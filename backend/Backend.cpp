#include "Backend.h"

#include <QStringList>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QUrl>

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "users.hpp"
#include "flights.hpp"
#include "seats.hpp"
#include "sqlite3db.hpp"

namespace {

double cabinMultiplier(Seat::Cabin c) {
    switch (c) {
        case Seat::Cabin::FirstClass: return 2.0;
        case Seat::Cabin::Business:   return 1.5;
        default:                      return 1.0;
    }
}

QString cabinName(Seat::Cabin c) {
    return QString::fromStdString(Seat::cabinToString(c));
}

QString money(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return QString::fromStdString(oss.str());
}

FlightRow toRow(const Flight& f) {
    FlightRow r;
    r.id = QString::fromStdString(f.getId());
    r.airline = QString::fromStdString(f.getAirline());
    r.origin = QString::fromStdString(f.getOrigin());
    r.destination = QString::fromStdString(f.getDestination());
    auto oit = airports.find(f.getOrigin());
    auto dit = airports.find(f.getDestination());
    r.originName = oit != airports.end() ? QString::fromStdString(oit->second) : r.origin;
    r.destinationName = dit != airports.end() ? QString::fromStdString(dit->second) : r.destination;
    r.departure = QString::fromStdString(f.getDepartureTime());
    r.arrival = QString::fromStdString(f.getArrivalTime());
    r.price = f.getTicketPrice();
    r.availableSeats = f.getAvailableSeats();
    r.totalSeats = f.getTotalSeats();
    r.status = QString::fromStdString(Flight::statusToString(f.getStatus()));
    return r;
}

} // namespace

Backend::Backend(QObject* parent) : QObject(parent) {
    SQLite3DB* db = SQLite3DB::getInstance();
    if (!db->isConnected())
        db->connect("data/flight_reservation.db");
    loadFlights();
    loadAccounts();
    refreshFlights();

    connect(&m_paystack, &PaystackClient::initialized, this,
            [this](const QString& reference, const QString& url) {
        m_paystackReference = reference;
        m_paystackAuthUrl = url;
        setPaystackBusy(false);
        emit paystackInitialized();
        QDesktopServices::openUrl(QUrl(url));
        setMessage("Payment page opened in your browser. Complete it, then verify.");
    });

    connect(&m_paystack, &PaystackClient::verified, this,
            [this](const QString& reference, bool success, double naira, const QString& status) {
        setPaystackBusy(false);
        if (m_verifyMode == VerifyMode::AdminConfirm) {
            const QString detail = "Reference " + reference + " — " + status +
                                   (success ? (" · NGN" + money(naira)) : QString());
            setMessage(detail);
            emit paystackVerifyResult(success, detail);
            return;
        }

        // Deposit verification: credit the customer once per reference.
        if (!success) {
            const QString detail = "Payment not successful yet (status: " + status + ").";
            setMessage(detail);
            emit paystackVerifyResult(false, detail);
            return;
        }
        if (isPaymentReferenceUsed(reference.toStdString())) {
            const QString detail = "This payment reference has already been used.";
            setMessage(detail);
            emit paystackVerifyResult(false, detail);
            return;
        }
        auto cit = customerUsers.find(m_currentUser.toStdString());
        if (cit == customerUsers.end()) {
            emit paystackVerifyResult(false, "Session expired.");
            return;
        }
        Customer& customer = cit->second;
        customer.creditBalance(naira);
        {
            std::ostringstream oss;
            oss << "Deposit (Paystack): NGN" << std::fixed << std::setprecision(2) << naira
                << " | Ref: " << reference.toStdString();
            customer.addTransaction(oss.str());
        }
        markPaymentReferenceAsUsed(reference.toStdString());
        saveAccounts(customerUsers);
        syncProfileFromCustomer();
        const QString detail = "Deposit successful. Credited NGN" + money(naira) + ".";
        setMessage(detail);
        emit paystackVerifyResult(true, detail);
    });

    connect(&m_paystack, &PaystackClient::error, this, [this](const QString& message) {
        setPaystackBusy(false);
        setMessage(message);
        emit paystackVerifyResult(false, message);
    });
}

Backend::~Backend() = default;

void Backend::setMessage(const QString& m) {
    m_lastMessage = m;
    emit messageChanged();
}

void Backend::setPaystackBusy(bool busy) {
    if (m_paystackBusy == busy) return;
    m_paystackBusy = busy;
    emit paystackBusyChanged();
}

void Backend::clearSession() {
    m_loggedIn = false;
    m_isAdmin = false;
    m_currentUser.clear();
    m_displayName.clear();
    m_balance = 0.0;
    m_totalBookings = 0;
    m_loyaltyPoints = 0.0;
    m_email.clear();
    m_phone.clear();
    m_gender.clear();
    m_address.clear();
    m_department.clear();
    m_adminLevel.clear();
    m_seatsModel.clear();
    m_bookingsModel.setRows({});
    emit sessionChanged();
    emit profileChanged();
}

void Backend::syncProfileFromCustomer() {
    auto it = customerUsers.find(m_currentUser.toStdString());
    if (it == customerUsers.end()) return;
    Customer& c = it->second;
    m_displayName = QString::fromStdString(c.getName());
    m_balance = c.getAccountBalance();
    m_totalBookings = c.getTotalBookings();
    m_loyaltyPoints = c.getLoyaltyPoints();
    m_email = QString::fromStdString(c.getEmail());
    m_phone = QString::fromStdString(c.getPhoneNumber());
    const char g = c.getGender();
    m_gender = g ? QString(QChar(g)) : QString();
    m_address = QString::fromStdString(c.getAddress());
    emit profileChanged();
}

void Backend::syncProfileFromAdmin() {
    auto it = adminUsers.find(m_currentUser.toStdString());
    if (it == adminUsers.end()) return;
    Admin& a = it->second;
    m_displayName = QString::fromStdString(a.getName());
    m_email = QString::fromStdString(a.getEmail());
    m_phone = QString::fromStdString(a.getPhoneNumber());
    const char g = a.getGender();
    m_gender = g ? QString(QChar(g)) : QString();
    m_address = QString::fromStdString(a.getAddress());
    m_department = QString::fromStdString(a.getDepartment());
    m_adminLevel = QString::fromStdString(a.getAdminLevel());
    emit profileChanged();
}

bool Backend::registerCustomer(const QString& username, const QString& password) {
    const std::string u = username.trimmed().toStdString();
    if (u.empty()) { setMessage("Username cannot be empty."); return false; }
    if (password.size() < 8) { setMessage("Password must be at least 8 characters long."); return false; }
    if (customerUsers.contains(u)) { setMessage("That username is already taken."); return false; }
    if (!createCustomerUser(u, password.toStdString())) {
        setMessage("Could not create account.");
        return false;
    }
    saveAccounts();
    setMessage("Account created. You can now log in.");
    return true;
}

bool Backend::login(const QString& username, const QString& password, bool asAdmin) {
    const std::string u = username.trimmed().toStdString();
    const std::string p = password.toStdString();
    if (u.empty() || p.empty()) { setMessage("Enter your username and password."); return false; }

    if (asAdmin) {
        auto it = adminUsers.find(u);
        if (it == adminUsers.end() || !it->second.validatePassword(p)) {
            setMessage("Incorrect username or password.");
            return false;
        }
        m_isAdmin = true;
        m_loggedIn = true;
        m_currentUser = username.trimmed();
        syncProfileFromAdmin();
        setMessage("Welcome, " + m_displayName + ".");
        emit sessionChanged();
        refreshFlights();
        return true;
    }

    auto it = customerUsers.find(u);
    if (it == customerUsers.end() || !it->second.validatePassword(p)) {
        setMessage("Incorrect username or password.");
        return false;
    }
    m_isAdmin = false;
    m_loggedIn = true;
    m_currentUser = username.trimmed();
    syncProfileFromCustomer();
    setMessage("Welcome aboard, " + m_displayName + ".");
    emit sessionChanged();
    refreshFlights();
    refreshBookings();
    return true;
}

void Backend::logout() {
    clearSession();
    setMessage("Logged out.");
}

void Backend::refreshFlights() {
    QList<FlightRow> bookable;
    QList<FlightRow> inactive;
    bookable.reserve(static_cast<int>(flights.size()));
    for (auto& [id, f] : flights) {
        // Re-evaluate the time-based status live so a flight that crosses its
        // boarding/departure time while the app is open moves out of the
        // bookable list instead of lingering there with stale availability.
        f.updateStatus();
        FlightRow r = toRow(f);
        if (f.getStatus() == Flight::Status::Scheduled)
            bookable.push_back(r);
        else
            inactive.push_back(r);
    }
    auto byId = [](const FlightRow& a, const FlightRow& b) { return a.id < b.id; };
    std::sort(bookable.begin(), bookable.end(), byId);
    std::sort(inactive.begin(), inactive.end(), byId);
    m_flightsModel.setRows(bookable);
    m_departedModel.setRows(inactive);
    emit flightsChanged();
}

void Backend::refreshBookings() {
    QList<FlightRow> rows;
    auto it = customerUsers.find(m_currentUser.toStdString());
    if (it != customerUsers.end()) {
        for (const auto& f : it->second.getBookedFlights())
            rows.push_back(toRow(f));
    }
    m_bookingsModel.setRows(rows);
}

QStringList Backend::airportCodes() const {
    QStringList codes;
    for (const auto& [code, name] : airports)
        codes << QString::fromStdString(code);
    codes.sort();
    return codes;
}

void Backend::loadSeats(const QString& flightId) {
    QList<SeatRow> rows;
    auto fit = flights.find(flightId.toStdString());
    if (fit != flights.end()) {
        const Flight& f = fit->second;
        const double base = f.getTicketPrice();
        for (const auto& seat : f.getSeats()) {
            if (!seat) continue;
            SeatRow s;
            s.seatNumber = QString::fromStdString(seat->getSeatNumber());
            s.row = seat->getRow();
            s.letter = QString(QChar(seat->getLetter()));
            s.cabin = cabinName(seat->getCabin());
            s.booked = seat->getIsBooked();
            s.price = base * cabinMultiplier(seat->getCabin());
            rows.push_back(s);
        }
        std::sort(rows.begin(), rows.end(), [](const SeatRow& a, const SeatRow& b) {
            if (a.row != b.row) return a.row < b.row;
            return a.letter < b.letter;
        });
    }
    m_seatsModel.setRows(rows);
}

bool Backend::bookSeat(const QString& flightId, const QString& seatNumber) {
    auto fit = flights.find(flightId.toStdString());
    if (fit == flights.end()) { setMessage("Flight not found."); return false; }
    Flight& f = fit->second;

    // Block booking once a flight is boarding/departed (or otherwise not open),
    // even if the on-screen list was stale when the seat dialog was opened.
    f.updateStatus();
    if (f.getStatus() != Flight::Status::Scheduled) {
        setMessage("Flight " + flightId + " is no longer open for booking (" +
                   QString::fromStdString(Flight::statusToString(f.getStatus())) + ").");
        refreshFlights();
        return false;
    }

    auto cit = customerUsers.find(m_currentUser.toStdString());
    if (cit == customerUsers.end()) { setMessage("Session expired. Please log in again."); return false; }
    Customer& customer = cit->second;

    for (const auto& bf : customer.getBookedFlights()) {
        if (bf.getId() == f.getId()) {
            setMessage("You have already booked a seat on flight " + flightId + ".");
            return false;
        }
    }

    Seat* seat = f.findSeat(seatNumber.toStdString());
    if (!seat) { setMessage("Seat " + seatNumber + " does not exist on this flight."); return false; }
    if (seat->getIsBooked()) { setMessage("Seat " + seatNumber + " is already booked."); return false; }

    const double price = f.getTicketPrice() * cabinMultiplier(seat->getCabin());
    if (customer.getAccountBalance() < price) {
        setMessage("Insufficient balance. This seat costs NGN" + money(price) +
                   " but your balance is NGN" + money(customer.getAccountBalance()) + ".");
        return false;
    }

    if (!f.bookSeat(seat->getSeatNumber())) {
        setMessage("Failed to book seat " + seatNumber + ".");
        return false;
    }

    customer.deductBalance(price);
    customer.addBookedFlight(f);
    customer.addBooking(seat->getCabin());
    {
        std::ostringstream oss;
        oss << "Flight booking: " << f.getId() << " seat " << seat->getSeatNumber()
            << " (NGN" << std::fixed << std::setprecision(2) << price << ")";
        customer.addTransaction(oss.str());
    }

    saveFlights();
    saveAccounts(customerUsers);

    setMessage("Booked seat " + seatNumber + " on flight " + flightId + ".");
    syncProfileFromCustomer();
    refreshFlights();
    refreshBookings();
    loadSeats(flightId);
    return true;
}

bool Backend::cancelSeat(const QString& flightId, const QString& seatNumber) {
    auto fit = flights.find(flightId.toStdString());
    if (fit == flights.end()) { setMessage("Flight not found."); return false; }
    Flight& f = fit->second;

    auto cit = customerUsers.find(m_currentUser.toStdString());
    if (cit == customerUsers.end()) { setMessage("Session expired. Please log in again."); return false; }
    Customer& customer = cit->second;

    Seat* seat = f.findSeat(seatNumber.toStdString());
    if (!seat) { setMessage("Seat " + seatNumber + " does not exist on this flight."); return false; }

    const double refund = f.getTicketPrice() * cabinMultiplier(seat->getCabin());

    if (!f.cancelSeat(seat->getSeatNumber(), seat->getBookedTime())) {
        setMessage("Could not cancel seat " + seatNumber + ". Make sure it is currently booked.");
        return false;
    }

    customer.creditBalance(refund);
    customer.removeBookedFlight(f.getId());
    {
        std::ostringstream oss;
        oss << "Booking cancelled: " << f.getId() << " seat " << seat->getSeatNumber()
            << " (Refund NGN" << std::fixed << std::setprecision(2) << refund << ")";
        customer.addTransaction(oss.str());
    }

    saveFlights();
    saveAccounts(customerUsers);

    setMessage("Cancelled seat " + seatNumber + ". Refunded NGN" + money(refund) + ".");
    syncProfileFromCustomer();
    refreshFlights();
    refreshBookings();
    loadSeats(flightId);
    return true;
}

void Backend::paystackDeposit(double amount, const QString& email) {
    if (m_paystackBusy) { setMessage("A payment is already in progress."); return; }
    if (amount <= 0) { setMessage("Enter a valid amount."); return; }
    auto cit = customerUsers.find(m_currentUser.toStdString());
    if (cit == customerUsers.end()) { setMessage("Session expired."); return; }
    Customer& customer = cit->second;

    QString useEmail = email.trimmed();
    if (useEmail.isEmpty()) useEmail = m_email;
    if (useEmail.isEmpty()) { setMessage("Enter an email to receive the Paystack receipt."); return; }
    // Persist the email if it is new/valid so future deposits prefill it.
    if (useEmail != m_email && customer.setEmail(useEmail.toStdString())) {
        saveAccounts(customerUsers);
        syncProfileFromCustomer();
    }

    m_paystackReference.clear();
    m_paystackAuthUrl.clear();
    m_verifyMode = VerifyMode::Deposit;
    setPaystackBusy(true);
    setMessage("Contacting Paystack…");
    m_paystack.initialize(useEmail, amount);
}

void Backend::paystackVerifyDeposit(const QString& reference) {
    const QString ref = reference.trimmed();
    if (ref.isEmpty()) { setMessage("Enter a payment reference to verify."); return; }
    if (m_paystackBusy) { setMessage("A payment is already in progress."); return; }
    m_verifyMode = VerifyMode::Deposit;
    setPaystackBusy(true);
    setMessage("Verifying payment…");
    m_paystack.verify(ref);
}

void Backend::adminVerifyPayment(const QString& reference) {
    const QString ref = reference.trimmed();
    if (ref.isEmpty()) { setMessage("Enter a payment reference to verify."); return; }
    if (m_paystackBusy) { setMessage("A verification is already in progress."); return; }
    m_verifyMode = VerifyMode::AdminConfirm;
    setPaystackBusy(true);
    setMessage("Verifying payment…");
    m_paystack.verify(ref);
}

bool Backend::withdraw(double amount) {
    if (amount <= 0) { setMessage("Enter a valid amount."); return false; }
    auto cit = customerUsers.find(m_currentUser.toStdString());
    if (cit == customerUsers.end()) { setMessage("Session expired."); return false; }
    Customer& customer = cit->second;
    if (amount > customer.getAccountBalance()) { setMessage("Insufficient balance."); return false; }
    customer.deductBalance(amount);
    {
        std::ostringstream oss;
        oss << "Withdrawal: - NGN" << std::fixed << std::setprecision(2) << amount;
        customer.addTransaction(oss.str());
    }
    saveAccounts(customerUsers);
    setMessage("Withdrew NGN" + money(amount) + ".");
    syncProfileFromCustomer();
    return true;
}

bool Backend::transfer(const QString& toUser, double amount) {
    const std::string to = toUser.trimmed().toStdString();
    if (to.empty()) { setMessage("Enter a recipient username."); return false; }
    if (to == m_currentUser.toStdString()) { setMessage("You cannot transfer to yourself."); return false; }
    if (!customerUsers.contains(to)) { setMessage("Recipient does not exist."); return false; }
    if (amount <= 0) { setMessage("Enter a valid amount."); return false; }

    Customer& sender = customerUsers.at(m_currentUser.toStdString());
    if (amount > sender.getAccountBalance()) { setMessage("Insufficient balance."); return false; }
    Customer& recipient = customerUsers.at(to);

    if (sender.deductBalance(amount)) recipient.creditBalance(amount);
    {
        std::ostringstream oss;
        oss << "Transfer to " << to << ": - NGN" << std::fixed << std::setprecision(2) << amount;
        sender.addTransaction(oss.str());
    }
    {
        std::ostringstream oss;
        oss << "Transfer from " << m_currentUser.toStdString() << ": + NGN"
            << std::fixed << std::setprecision(2) << amount;
        recipient.addTransaction(oss.str());
    }
    saveAccounts(customerUsers);
    setMessage("Transferred NGN" + money(amount) + " to " + toUser + ".");
    syncProfileFromCustomer();
    return true;
}

QStringList Backend::transactionHistory() const {
    QStringList out;
    auto it = customerUsers.find(m_currentUser.toStdString());
    if (it != customerUsers.end()) {
        const auto& hist = it->second.getTransactionHistory();
        for (auto i = hist.rbegin(); i != hist.rend(); ++i)
            out << QString::fromStdString(*i);
    }
    return out;
}

bool Backend::updateProfile(const QString& name, const QString& email,
                            const QString& phone, const QString& gender,
                            const QString& address) {
    auto it = customerUsers.find(m_currentUser.toStdString());
    if (it == customerUsers.end()) { setMessage("Session expired."); return false; }
    Customer& c = it->second;
    if (!name.isEmpty()) {
        if (name.length() > 100) { setMessage("Name is too long (max 100 characters)."); return false; }
        c.setName(name.toStdString());
    }
    if (!email.isEmpty() && !c.setEmail(email.toStdString())) { setMessage("Invalid email (must contain '@')."); return false; }
    if (!phone.isEmpty() && !c.setPhoneNumber(phone.toStdString())) { setMessage("Invalid phone (10-20 digits)."); return false; }
    if (!gender.isEmpty() && !c.setGender(gender.at(0).toLatin1())) { setMessage("Invalid gender (use M or F)."); return false; }
    if (!address.isEmpty()) c.setAddress(address.toStdString());
    saveAccounts(customerUsers);
    setMessage("Profile updated.");
    syncProfileFromCustomer();
    return true;
}

bool Backend::changePassword(const QString& current, const QString& next) {
    auto it = customerUsers.find(m_currentUser.toStdString());
    if (it == customerUsers.end()) { setMessage("Session expired."); return false; }
    Customer& c = it->second;
    if (!c.validatePassword(current.toStdString())) { setMessage("Current password is incorrect."); return false; }
    if (!c.setPassword(next.toStdString())) { setMessage("New password must be at least 8 characters."); return false; }
    saveAccounts(customerUsers);
    setMessage("Password changed.");
    return true;
}

QStringList Backend::nextOfKin() const {
    auto it = customerUsers.find(m_currentUser.toStdString());
    if (it == customerUsers.end()) return {QString(), QString(), QString(), QString()};
    const NextOfKin nok = it->second.getNextOfKin();
    return {QString::fromStdString(nok.name), QString::fromStdString(nok.relationship),
            QString::fromStdString(nok.phoneNumber), QString::fromStdString(nok.address)};
}

bool Backend::updateNextOfKin(const QString& name, const QString& relationship,
                              const QString& phone, const QString& address) {
    auto it = customerUsers.find(m_currentUser.toStdString());
    if (it == customerUsers.end()) { setMessage("Session expired."); return false; }
    if (name.trimmed().isEmpty() || relationship.trimmed().isEmpty()) {
        setMessage("Next of kin name and relationship are required.");
        return false;
    }
    NextOfKin nok{ name.toStdString(), relationship.toStdString(),
                   phone.toStdString(), address.toStdString() };
    it->second.setNextOfKin(nok);
    saveAccounts(customerUsers);
    setMessage("Next of kin updated.");
    return true;
}

bool Backend::adminUpdateProfile(const QString& name, const QString& email,
                                 const QString& phone, const QString& gender,
                                 const QString& address, const QString& department) {
    auto it = adminUsers.find(m_currentUser.toStdString());
    if (it == adminUsers.end()) { setMessage("Session expired."); return false; }
    Admin& a = it->second;
    if (!name.isEmpty()) a.setName(name.toStdString());
    if (!email.isEmpty() && !a.setEmail(email.toStdString())) { setMessage("Invalid email (must contain '@')."); return false; }
    if (!phone.isEmpty() && !a.setPhoneNumber(phone.toStdString())) { setMessage("Invalid phone (10-20 digits)."); return false; }
    if (!gender.isEmpty() && !a.setGender(gender.at(0).toLatin1())) { setMessage("Invalid gender (use M or F)."); return false; }
    if (!address.isEmpty()) a.setAddress(address.toStdString());
    if (!department.isEmpty()) a.setDepartment(department.toStdString());
    saveAccounts(adminUsers);
    setMessage("Profile updated.");
    syncProfileFromAdmin();
    return true;
}

bool Backend::adminChangePassword(const QString& current, const QString& next) {
    auto it = adminUsers.find(m_currentUser.toStdString());
    if (it == adminUsers.end()) { setMessage("Session expired."); return false; }
    Admin& a = it->second;
    if (!a.validatePassword(current.toStdString())) { setMessage("Current password is incorrect."); return false; }
    if (!a.setPassword(next.toStdString())) { setMessage("New password must be at least 8 characters."); return false; }
    saveAccounts(adminUsers);
    setMessage("Password changed.");
    return true;
}

bool Backend::adminSaveFlight(const QString& id, const QString& airline,
                              const QString& origin, const QString& destination,
                              const QString& departure, const QString& arrival,
                              int totalSeats, double price) {
    std::string fid = id.trimmed().toStdString();
    std::transform(fid.begin(), fid.end(), fid.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (fid.empty()) { setMessage("Flight ID cannot be empty."); return false; }

    const bool exists = flights.contains(fid);
    Flight tmp;
    Flight& f = exists ? flights.at(fid) : tmp;
    if (!exists) f.setId(fid);

    if (!f.setAirline(airline.toStdString())) { setMessage("Invalid airline name."); return false; }
    if (!f.setOrigin(origin.toStdString())) { setMessage("Invalid origin airport code."); return false; }
    if (!f.setDestination(destination.toStdString())) { setMessage("Invalid destination (must differ from origin)."); return false; }

    // Validate the date/time fields ourselves so the message tells the admin
    // exactly what is wrong: a malformed string, a past departure, or an arrival
    // that is not after departure (previously all three said "format").
    static const QRegularExpression dtFormat(
        QStringLiteral("^\\d{4}-\\d{1,2}-\\d{1,2} \\d{1,2}:\\d{2}$"));
    const QString dep = departure.trimmed();
    const QString arr = arrival.trimmed();
    if (!dtFormat.match(dep).hasMatch()) {
        setMessage("Invalid departure format. Use YYYY-MM-DD HH:MM (e.g. 2026-06-15 14:30).");
        return false;
    }
    if (!dtFormat.match(arr).hasMatch()) {
        setMessage("Invalid arrival format. Use YYYY-MM-DD HH:MM (e.g. 2026-06-15 17:45).");
        return false;
    }
    if (!f.setDepartureTimeFromString(dep.toStdString())) {
        setMessage("Departure time must be in the future (and the flight not already departed).");
        return false;
    }
    if (!f.setArrivalTimeFromString(arr.toStdString())) {
        setMessage("Arrival time must be after the departure time.");
        return false;
    }
    if (!f.setTotalSeats(totalSeats)) { setMessage("Invalid seat count (1-500, cannot resize if booked)."); return false; }
    if (!f.setTicketPrice(price)) { setMessage("Invalid ticket price."); return false; }

    if (!exists) flights[fid] = tmp;
    saveFlights();
    setMessage(exists ? ("Flight " + QString::fromStdString(fid) + " updated.")
                      : ("Flight " + QString::fromStdString(fid) + " created."));
    refreshFlights();
    return true;
}

bool Backend::adminSetPrice(const QString& flightId, double price) {
    std::string fid = flightId.trimmed().toStdString();
    std::transform(fid.begin(), fid.end(), fid.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (!flights.contains(fid)) { setMessage("Flight not found."); return false; }
    if (price <= 0) { setMessage("Price must be greater than 0."); return false; }
    flights.at(fid).setTicketPrice(price);
    saveFlights();
    setMessage("Price updated for " + QString::fromStdString(fid) + ".");
    refreshFlights();
    return true;
}

QStringList Backend::adminAllBookings() const {
    QStringList out;
    for (const auto& [flightId, flight] : flights) {
        for (const auto& seat : flight.getSeats()) {
            if (seat && seat->getIsBooked()) {
                out << QString::fromStdString(flightId) + "  "
                       + QString::fromStdString(seat->getSeatNumber())
                       + " (" + QString::fromStdString(Seat::cabinToString(seat->getCabin())) + ")";
            }
        }
    }
    out.sort();
    return out;
}

QStringList Backend::adminCustomers() const {
    QStringList out;
    for (const auto& [uname, cust] : customerUsers) {
        out << QString::fromStdString(uname)
               + "  |  " + QString::fromStdString(cust.getName())
               + "  |  NGN" + money(cust.getAccountBalance())
               + "  |  " + QString::number(cust.getTotalBookings()) + " bookings";
    }
    out.sort();
    return out;
}

QString Backend::seatSummary(const QString& flightId) const {
    std::string fid = flightId.trimmed().toStdString();
    std::transform(fid.begin(), fid.end(), fid.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    auto it = flights.find(fid);
    if (it == flights.end()) return "Flight not found.";
    const Flight& f = it->second;

    int econ = 0, biz = 0, first = 0;
    int econB = 0, bizB = 0, firstB = 0;
    for (const auto& seat : f.getSeats()) {
        if (!seat) continue;
        switch (seat->getCabin()) {
            case Seat::Cabin::Economy:    ++econ;  if (seat->getIsBooked()) ++econB;  break;
            case Seat::Cabin::Business:   ++biz;   if (seat->getIsBooked()) ++bizB;   break;
            case Seat::Cabin::FirstClass: ++first; if (seat->getIsBooked()) ++firstB; break;
        }
    }
    return QString::fromStdString(fid) + "  ·  "
         + QString::number(f.getAvailableSeats()) + " of " + QString::number(f.getTotalSeats()) + " seats free\n"
         + "Booked: " + QString::number(f.getBookedSeats()) + "\n"
         + "Economy: " + QString::number(econB) + " / " + QString::number(econ) + "\n"
         + "Business: " + QString::number(bizB) + " / " + QString::number(biz) + "\n"
         + "First Class: " + QString::number(firstB) + " / " + QString::number(first);
}

bool Backend::createAdmin(const QString& username, const QString& password) {
    if (!isMaxAdmin()) { setMessage("Only MAX-level admins can do this."); return false; }
    const std::string u = username.trimmed().toStdString();
    if (u.empty()) { setMessage("Username cannot be empty."); return false; }
    if (password.size() < 8) { setMessage("Password must be at least 8 characters long."); return false; }
    if (adminUsers.contains(u)) { setMessage("That admin username already exists."); return false; }
    if (!createAdminUser(u, password.toStdString())) { setMessage("Could not create admin account."); return false; }
    saveAccounts(adminUsers);
    setMessage("Admin '" + username.trimmed() + "' created (Level 1).");
    return true;
}

bool Backend::deleteCustomerAccount(const QString& username) {
    if (!isMaxAdmin()) { setMessage("Only MAX-level admins can do this."); return false; }
    const std::string u = username.trimmed().toStdString();
    if (u.empty()) { setMessage("Username cannot be empty."); return false; }
    if (!customerUsers.contains(u)) { setMessage("Customer '" + username.trimmed() + "' not found."); return false; }
    deleteUser(u, customerUsers);
    saveAccounts(customerUsers);
    setMessage("Customer '" + username.trimmed() + "' deleted.");
    return true;
}

bool Backend::deleteAdminAccount(const QString& username) {
    if (!isMaxAdmin()) { setMessage("Only MAX-level admins can do this."); return false; }
    const QString name = username.trimmed();
    const std::string u = name.toStdString();
    if (u.empty()) { setMessage("Username cannot be empty."); return false; }
    if (name == m_currentUser) { setMessage("You cannot delete your own admin account."); return false; }
    if (u == "Shadow as admin.") { setMessage("This is a reserved account and cannot be deleted."); return false; }
    if (!adminUsers.contains(u)) { setMessage("Admin '" + name + "' not found."); return false; }
    deleteUser(u, adminUsers);
    saveAccounts(adminUsers);
    setMessage("Admin '" + name + "' deleted.");
    return true;
}

bool Backend::setAdminLevelFor(const QString& target, const QString& level) {
    if (!isMaxAdmin()) { setMessage("Only MAX-level admins can do this."); return false; }
    const std::string t = target.trimmed().toStdString();
    const std::string lvl = level.trimmed().toStdString();
    if (lvl.empty()) { setMessage("Admin level cannot be empty."); return false; }
    auto it = adminUsers.find(t);
    if (it == adminUsers.end()) { setMessage("Admin '" + target.trimmed() + "' not found."); return false; }
    it->second.setAdminLevel(lvl);
    saveAccounts(adminUsers);
    if (target.trimmed() == m_currentUser) syncProfileFromAdmin();
    setMessage("Updated '" + target.trimmed() + "' level to '" + level.trimmed() + "'.");
    return true;
}

QStringList Backend::adminList() const {
    QStringList out;
    for (const auto& [uname, adm] : adminUsers) {
        out << QString::fromStdString(uname)
               + "  |  " + QString::fromStdString(adm.getName())
               + "  |  Level: " + QString::fromStdString(adm.getAdminLevel())
               + "  |  Dept: " + QString::fromStdString(adm.getDepartment());
    }
    out.sort();
    return out;
}

QStringList Backend::customerTransactions(const QString& username) const {
    QStringList out;
    const std::string u = username.trimmed().toStdString();
    if (!u.empty()) {
        auto it = customerUsers.find(u);
        if (it == customerUsers.end()) return {"Customer '" + username.trimmed() + "' not found."};
        for (const auto& tx : it->second.getTransactionHistory())
            out << QString::fromStdString(tx);
    } else {
        for (const auto& [uname, cust] : customerUsers) {
            for (const auto& tx : cust.getTransactionHistory())
                out << QString::fromStdString(uname) + ": " + QString::fromStdString(tx);
        }
    }
    return out;
}
