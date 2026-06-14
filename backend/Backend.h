#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration>

#include "FlightListModel.h"
#include "SeatGridModel.h"
#include "PaystackClient.h"

class Backend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY sessionChanged)
    Q_PROPERTY(bool isAdmin READ isAdmin NOTIFY sessionChanged)
    Q_PROPERTY(bool isMaxAdmin READ isMaxAdmin NOTIFY sessionChanged)
    Q_PROPERTY(QString currentUser READ currentUser NOTIFY sessionChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY profileChanged)
    Q_PROPERTY(double balance READ balance NOTIFY profileChanged)
    Q_PROPERTY(int totalBookings READ totalBookings NOTIFY profileChanged)
    Q_PROPERTY(double loyaltyPoints READ loyaltyPoints NOTIFY profileChanged)
    Q_PROPERTY(QString email READ email NOTIFY profileChanged)
    Q_PROPERTY(QString phone READ phone NOTIFY profileChanged)
    Q_PROPERTY(QString gender READ gender NOTIFY profileChanged)
    Q_PROPERTY(QString address READ address NOTIFY profileChanged)
    Q_PROPERTY(QString department READ department NOTIFY profileChanged)
    Q_PROPERTY(QString adminLevel READ adminLevel NOTIFY sessionChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY messageChanged)

    Q_PROPERTY(bool paystackBusy READ paystackBusy NOTIFY paystackBusyChanged)
    Q_PROPERTY(QString paystackReference READ paystackReference NOTIFY paystackInitialized)
    Q_PROPERTY(QString paystackAuthUrl READ paystackAuthUrl NOTIFY paystackInitialized)

    Q_PROPERTY(FlightListModel* flightsModel READ flightsModel CONSTANT)
    Q_PROPERTY(FlightListModel* departedFlightsModel READ departedFlightsModel CONSTANT)
    Q_PROPERTY(SeatGridModel* seatsModel READ seatsModel CONSTANT)
    Q_PROPERTY(FlightListModel* bookingsModel READ bookingsModel CONSTANT)

public:
    explicit Backend(QObject* parent = nullptr);
    ~Backend() override;

    bool loggedIn() const { return m_loggedIn; }
    bool isAdmin() const { return m_isAdmin; }
    bool isMaxAdmin() const { return m_isAdmin && m_adminLevel == QStringLiteral("MAX"); }
    QString currentUser() const { return m_currentUser; }
    QString displayName() const { return m_displayName; }
    double balance() const { return m_balance; }
    int totalBookings() const { return m_totalBookings; }
    double loyaltyPoints() const { return m_loyaltyPoints; }
    QString email() const { return m_email; }
    QString phone() const { return m_phone; }
    QString gender() const { return m_gender; }
    QString address() const { return m_address; }
    QString department() const { return m_department; }
    QString adminLevel() const { return m_adminLevel; }
    QString lastMessage() const { return m_lastMessage; }

    bool paystackBusy() const { return m_paystackBusy; }
    QString paystackReference() const { return m_paystackReference; }
    QString paystackAuthUrl() const { return m_paystackAuthUrl; }

    FlightListModel* flightsModel() { return &m_flightsModel; }
    FlightListModel* departedFlightsModel() { return &m_departedModel; }
    SeatGridModel* seatsModel() { return &m_seatsModel; }
    FlightListModel* bookingsModel() { return &m_bookingsModel; }

    // Auth
    Q_INVOKABLE bool registerCustomer(const QString& username, const QString& password);
    Q_INVOKABLE bool login(const QString& username, const QString& password, bool asAdmin);
    Q_INVOKABLE void logout();

    // Data refresh
    Q_INVOKABLE void refreshFlights();
    Q_INVOKABLE void refreshBookings();
    Q_INVOKABLE void loadSeats(const QString& flightId);
    Q_INVOKABLE QStringList airportCodes() const;

    // Customer actions
    Q_INVOKABLE bool bookSeat(const QString& flightId, const QString& seatNumber);
    Q_INVOKABLE bool cancelSeat(const QString& flightId, const QString& seatNumber);
    Q_INVOKABLE bool withdraw(double amount);
    Q_INVOKABLE bool transfer(const QString& toUser, double amount);
    Q_INVOKABLE QStringList transactionHistory() const;

    // Paystack deposits (asynchronous)
    Q_INVOKABLE void paystackDeposit(double amount, const QString& email);
    Q_INVOKABLE void paystackVerifyDeposit(const QString& reference);
    Q_INVOKABLE void adminVerifyPayment(const QString& reference);

    // Profile (customer)
    Q_INVOKABLE bool updateProfile(const QString& name, const QString& email,
                                   const QString& phone, const QString& gender,
                                   const QString& address);
    Q_INVOKABLE bool changePassword(const QString& current, const QString& next);
    Q_INVOKABLE QStringList nextOfKin() const;
    Q_INVOKABLE bool updateNextOfKin(const QString& name, const QString& relationship,
                                     const QString& phone, const QString& address);

    // Profile (admin)
    Q_INVOKABLE bool adminUpdateProfile(const QString& name, const QString& email,
                                        const QString& phone, const QString& gender,
                                        const QString& address, const QString& department);
    Q_INVOKABLE bool adminChangePassword(const QString& current, const QString& next);

    // Admin actions
    Q_INVOKABLE bool adminSaveFlight(const QString& id, const QString& airline,
                                     const QString& origin, const QString& destination,
                                     const QString& departure, const QString& arrival,
                                     int totalSeats, double price);
    Q_INVOKABLE bool adminSetPrice(const QString& flightId, double price);
    Q_INVOKABLE QStringList adminAllBookings() const;
    Q_INVOKABLE QStringList adminCustomers() const;
    Q_INVOKABLE QString seatSummary(const QString& flightId) const;

    // Super-admin (MAX) actions
    Q_INVOKABLE bool createAdmin(const QString& username, const QString& password);
    Q_INVOKABLE bool deleteCustomerAccount(const QString& username);
    Q_INVOKABLE bool deleteAdminAccount(const QString& username);
    Q_INVOKABLE bool setAdminLevelFor(const QString& target, const QString& level);
    Q_INVOKABLE QStringList adminList() const;
    Q_INVOKABLE QStringList customerTransactions(const QString& username) const;

signals:
    void sessionChanged();
    void profileChanged();
    void messageChanged();
    void flightsChanged();
    void paystackBusyChanged();
    void paystackInitialized();
    void paystackVerifyResult(bool success, const QString& detail);

private:
    enum class VerifyMode { Deposit, AdminConfirm };

    void setMessage(const QString& m);
    void setPaystackBusy(bool busy);
    void syncProfileFromCustomer();
    void syncProfileFromAdmin();
    void clearSession();

    bool m_loggedIn = false;
    bool m_isAdmin = false;
    QString m_currentUser;
    QString m_displayName;
    double m_balance = 0.0;
    int m_totalBookings = 0;
    double m_loyaltyPoints = 0.0;
    QString m_email;
    QString m_phone;
    QString m_gender;
    QString m_address;
    QString m_department;
    QString m_adminLevel;
    QString m_lastMessage;

    PaystackClient m_paystack;
    bool m_paystackBusy = false;
    QString m_paystackReference;
    QString m_paystackAuthUrl;
    VerifyMode m_verifyMode = VerifyMode::Deposit;

    FlightListModel m_flightsModel;
    FlightListModel m_departedModel;
    FlightListModel m_bookingsModel;
    SeatGridModel m_seatsModel;
};
