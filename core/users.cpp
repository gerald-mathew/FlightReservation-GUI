#include "users.hpp"
#include "payment.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <random>
#include <cctype>
#include <ctime>
#include <limits>

#ifdef _WIN32
#include <conio.h>
#endif

// Magic constant for super admin username
constexpr std::string_view SHADOW_ADMIN_USERNAME = "Shadow as admin.";

std::string escapeSql(const std::string& str) {
	std::string escaped;
	for (char c : str) {
		if (c == '\'') {
			escaped += "''";  // SQLite escape: '' represents a single quote
		}
		else {
			escaped += c;
		}
	}
	return escaped;
}

std::chrono::year_month_day parseDate(const std::string& dateStr) {
	std::istringstream iss(dateStr);
	int y, m, d;
	char dash1, dash2;
	if (!(iss >> y >> dash1 >> m >> dash2 >> d && dash1 == '-' && dash2 == '-')) {
		throw std::invalid_argument("Invalid date format");
	}
	auto ymd = std::chrono::year_month_day{ std::chrono::year{y}, std::chrono::month{static_cast<unsigned>(m)}, std::chrono::day{static_cast<unsigned>(d)} };
	if (!ymd.ok()) throw std::invalid_argument("Invalid date");
	return ymd;
}

User::User() = default;
User::User(const std::string& aName, const std::string& aPassword, const std::string& aPhoneNumber, char aGender, const std::string& anAddress, const std::string& anEmail, const NextOfKin& aNextOfKin)
	: name(aName), phoneNumber(aPhoneNumber), gender(aGender), address(anAddress), email(anEmail), nextOfKin(aNextOfKin) {
	// Hash password on construction
	nlohmann::json hashResult = Crypto::hashPassword(aPassword);
	if (hashResult.contains("hash") && hashResult.contains("salt")) {
		passwordHash = hashResult["hash"].get<std::string>();
		passwordSalt = hashResult["salt"].get<std::string>();
	}
}

void User::setName(const std::string& aName) {
	name = aName;
}

bool User::setPhoneNumber(std::string aPhoneNumber) {
	if (aPhoneNumber.length() < 10 || aPhoneNumber.length() > 20)
		return false;
	for (char& c : aPhoneNumber) {
		if (!std::isdigit(c)) {
			std::cout << "Phone number must contain only digits." << std::endl;
			return false;
		}
	}
	phoneNumber = std::move(aPhoneNumber);
	return true;
}

bool User::setGender(char aGender) {
	aGender = static_cast<char>(std::toupper(static_cast<unsigned char>(aGender)));
	if (aGender != 'M' && aGender != 'F') {
		return false;
	}
	gender = aGender;
	return true;
}

void User::setAddress(const std::string& anAddress) {
	address = anAddress;
}

bool User::setPassword(const std::string& aPassword) {
	if (aPassword.length() < 8) {
		std::cout << "Password must be at least 8 characters long." << std::endl;
		return false;
	}
	// Hash password using Argon2 hashing
	nlohmann::json hashResult = Crypto::hashPassword(aPassword);
	if (hashResult.contains("hash") && hashResult.contains("salt")) {
		passwordHash = hashResult["hash"].get<std::string>();
		passwordSalt = hashResult["salt"].get<std::string>();
		return true;
	}
	return false;
}

bool User::setEmail(const std::string& anEmail) {
	// Email validation: must contain @ and a domain with at least one dot after @
	size_t atPos = anEmail.find("@");
	if (atPos == std::string::npos || atPos == 0 || atPos == anEmail.length() - 1) {
		return false;
	}
	size_t dotPos = anEmail.find('.', atPos);
	if (dotPos == std::string::npos || dotPos == anEmail.length() - 1) {
		return false;
	}
	email = anEmail;
	return true;
}

void User::setNextOfKin(NextOfKin aNextOfKin) {
	nextOfKin = aNextOfKin;
}

bool User::validatePassword(const std::string& aPassword) const { 
	return Crypto::verifyPassword(aPassword, passwordHash, passwordSalt);
}

std::string User::getName() const { return name; }
std::string User::getPhoneNumber() const { return phoneNumber; }
char User::getGender() const { return gender; }
std::string User::getAddress() const { return address; }
std::string User::getPasswordHash() const { return passwordHash; }
std::string User::getPasswordSalt() const { return passwordSalt; }
std::string User::getEmail() const { return email; }
NextOfKin User::getNextOfKin() const { return nextOfKin; }
User::~User() = default;


Admin::Admin() : User("", "") {
	department = "General Administration";
	adminLevel = "Level 1";
	hireDate = parseDate("2026-01-01");
}

Admin::Admin(const std::string& aName, const std::string& aPassword) : User(aName, aPassword) {
	department = "General Administration";
	adminLevel = "Level 1";
	hireDate = parseDate("2026-01-01");
	std::cout << "Welcome new admin, " << getName() << std::endl;
}

Admin::Admin(const std::string& aName, const std::string& aPassword, const std::string& aPhoneNumber, char aGender, const std::string& anAddress, const std::string& anEmail, NextOfKin aNextOfKin, const std::string& aDepartment, const std::string& anAdminLevel, const std::string& aHireDate)
	: User(aName, aPassword, aPhoneNumber, aGender, anAddress, anEmail, aNextOfKin), department(aDepartment), adminLevel(anAdminLevel), hireDate(parseDate(aHireDate)) {
}

Admin::Admin(const Admin& other) : User(other.getName(), "", other.getPhoneNumber(), other.getGender(), other.getAddress(), other.getEmail(), other.getNextOfKin()) {
	department = other.department;
	adminLevel = other.adminLevel;
	hireDate = other.hireDate;
	// Copy password hash and salt directly
	passwordHash = other.passwordHash;
	passwordSalt = other.passwordSalt;
}

void Admin::setDepartment(const std::string& aDepartment) {
	department = aDepartment;
}

void Admin::setAdminLevel(const std::string& anAdminLevel) {
	adminLevel = anAdminLevel;
}

bool Admin::setHireDate(const std::string& aHireDate) {
	try {
		hireDate = parseDate(aHireDate);
		return true;
	}
	catch (const std::invalid_argument&) {
		std::cout << "Invalid date format. Please use YYYY-MM-DD." << std::endl;
		return false;
	}
}

std::string Admin::getDepartment() const { return department; }
std::string Admin::getAdminLevel() const { return adminLevel; }
std::string Admin::getHireDate() const {
	std::ostringstream oss;
	oss << static_cast<int>(hireDate.year()) << '-'
		<< std::setw(2) << std::setfill('0') << static_cast<unsigned>(hireDate.month()) << '-'
		<< std::setw(2) << std::setfill('0') << static_cast<unsigned>(hireDate.day());
	return oss.str();
}


Customer::Customer() : User("", "") {
	totalBookings = 0;
	loyaltyPoints = 0.0;
	accountBalance = 0.0;
	accountCreationDate = getCurrentDate();
}

Customer::Customer(const std::string& aName, const std::string& aPassword) : User(aName, aPassword) {
	totalBookings = 0;
	loyaltyPoints = 0.0;
	accountBalance = 0.0;
	accountCreationDate = getCurrentDate();
}

Customer::Customer(const std::string& aName, const std::string& aPassword, const std::string& aPhoneNumber, char aGender, const std::string& anAddress, const std::string& anEmail, NextOfKin aNextOfKin, int aTotalBookings, double aLoyaltyPoints, const std::string& anAccountCreationDate, double anAccountBalance, std::vector<std::string> aTransactionHistory, std::vector <Flight> aBookedFlight)
	: User(aName, aPassword, aPhoneNumber, aGender, anAddress, anEmail, aNextOfKin), totalBookings(aTotalBookings), loyaltyPoints(aLoyaltyPoints), accountCreationDate(anAccountCreationDate), accountBalance(anAccountBalance), transactionHistory(aTransactionHistory), bookedFlights(aBookedFlight) {
}

Customer::Customer(const Customer& other) : User(other.getName(), "", other.getPhoneNumber(), other.getGender(), other.getAddress(), other.getEmail(), other.getNextOfKin()) {
	totalBookings = other.totalBookings;
	loyaltyPoints = other.loyaltyPoints;
	accountCreationDate = other.accountCreationDate;
	accountBalance = other.accountBalance;
	transactionHistory = other.transactionHistory;
	bookedFlights = other.bookedFlights;
	// Copy password hash and salt directly
	passwordHash = other.passwordHash;
	passwordSalt = other.passwordSalt;
}

void Customer::setTotalBookings(int aBookings) {
	totalBookings = aBookings;
}

void Customer::setLoyaltyPoints(double aPoints) {
	loyaltyPoints = aPoints;
}

void Customer::setAccountCreationDate(const std::string& aDate) {
	accountCreationDate = aDate;
}

void Customer::setAccountBalance(double aBalance) {
	accountBalance = aBalance;
}

int Customer::getTotalBookings() const { return totalBookings; }
double Customer::getLoyaltyPoints() const { return loyaltyPoints; }
std::string Customer::getAccountCreationDate() const { return accountCreationDate; }
double Customer::getAccountBalance() const { return accountBalance; }
std::vector <std::string> Customer::getTransactionHistory() const { return transactionHistory; }
std::vector <Flight> Customer::getBookedFlights() const { return bookedFlights; }

std::string Customer::getCurrentDate() const {
	auto now = std::chrono::system_clock::now();
	auto ymd = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(now));
	std::ostringstream oss;
	oss << static_cast<int>(ymd.year()) << '-'
		<< std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.month()) << '-'
		<< std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.day());
	return oss.str();
}

void Customer::addBooking(Seat::Cabin cabin) {
	switch (cabin) {
	case Seat::Cabin::FirstClass:
		loyaltyPoints += 5;
		break;
	case Seat::Cabin::Business:
		loyaltyPoints += 3;
		break;
	case Seat::Cabin::Economy:
	default:
		loyaltyPoints += 1;
		break;
	}
	totalBookings++;
}

bool Customer::addLoyaltyPoints(double aPoints) {
	if (aPoints < 0) {
		std::cout << "Cannot add negative loyalty points." << std::endl;
		return false;
	}
	loyaltyPoints += aPoints;
	return true;
}

bool Customer::deductBalance(double anAmount) {
	if (anAmount < 0) {
		std::cout << "Cannot deduct negative amount." << std::endl;
		return false;
	}
	if (accountBalance < anAmount) {
		std::cout << "Insufficient balance. Current balance: " << accountBalance << std::endl;
		return false;
	}
	accountBalance -= anAmount;
	return true;
}

bool Customer::creditBalance(double anAmount) {
	if (anAmount < 0) {
		std::cout << "Cannot credit negative amount." << std::endl;
		return false;
	}
	accountBalance += anAmount;
	return true;
}

void Customer::addTransaction(const std::string& aTransaction) {
	transactionHistory.push_back(aTransaction);
}

void Customer::addBookedFlight(const Flight& aFlight) {
	bookedFlights.push_back(aFlight);
}

bool Customer::removeBookedFlight(const std::string& flightId) {
	if (bookedFlights.empty()) {
		std::cout << "No booked flights to remove." << std::endl;
		return false;
	}
	if (flightId.empty()) return false;
	const auto oldSize = bookedFlights.size();
	bookedFlights.erase(
		std::remove_if(bookedFlights.begin(), bookedFlights.end(), [&](const Flight& f) { return f.getId() == flightId; }),
		bookedFlights.end());
	if (bookedFlights.size() == oldSize) {
		std::cout << "Specified flight not found in booked flights." << std::endl;
		return false;
	}
	return true;
}

bool Customer::removeBookedFlight(const Flight& aFlight) {
	return removeBookedFlight(aFlight.getId());
}

#if !defined(_WIN32)
#include <termios.h>
#include <unistd.h>
#endif

std::string readPassword() {
	std::string password;
#ifdef _WIN32
	// Windows: Use _getch() but filter out extended/special keys
	char ch;
	while ((ch = _getch()) != '\r') {  // Enter key = carriage return
		// Only accept backspace and printable ASCII characters (32-126)
		if (ch == '\b') {  // Backspace key
			if (!password.empty()) {
				password.pop_back();
				std::cout << "\b \b";  // Erase * from screen
			}
		}
		// Ignore arrow keys, function keys, and other extended characters
		// Extended keys on Windows start with 0 or 0xE0 as first byte
		else if (ch >= 32 && ch <= 126) {  // Printable ASCII only
			password += ch;
			std::cout << '*';  // Show * for each character
		}
		// Non-printable characters are silently ignored (no crash, no acceptance)
	}
	std::cout << std::endl;
#elif defined(__linux__)
	// Unix/Linux: Disable echo and read normally
	termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~ECHO;  // Turn off echo
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	std::string rawInput;
	std::getline(std::cin, rawInput);

	// Filter: only keep printable ASCII characters (32-126)
	for (char c : rawInput) {
		if (c >= 32 && c <= 126) {
			password += c;
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore terminal settings
	std::cout << std::endl;
#else
	std::getline(std::cin, password);
	std::cout << std::endl;
#endif
	return password;
}

template <typename U> std::string validateUser(const std::unordered_map<std::string, U>& userCategory) {
	std::cout << "Enter your username: ";
	std::string uname;
	std::getline(std::cin, uname);
	if (uname.empty()) {
		std::cout << "Username cannot be empty. Please try again." << std::endl;
		return "";
	}
	std::cout << "Enter your password: ";
	std::string pwd = readPassword();
	if (pwd.empty()) {
		std::cout << "Password cannot be empty. Please try again." << std::endl;
		return "";
	}
	if (!userCategory.contains(uname)) {
		std::cout << "Username does not exist. Please try again." << std::endl;
		return "";
	}
	if (!userCategory.at(uname).validatePassword(pwd)) {
		std::cout << "Incorrect username or password." << std::endl;
		return "";
	}
	std::cout << "" << std::endl;
	return uname;
}

bool createAdminUser(const std::string& username, const std::string& password) {
	if (!adminUsers.contains(username) && username != std::string(SHADOW_ADMIN_USERNAME)) {
		if (password.empty()) {
			std::cout << "Password cannot be empty." << std::endl;
			return false;
		}
		if (password.length() < 8) {
			std::cout << "Password must be at least 8 characters long." << std::endl;
			return false;
		}
		adminUsers.emplace(username, Admin(username, password));
		return true;
	}
	else {
		std::cout << "Admin user with username '" << username << "' already exists." << std::endl;
		return false;
	}
}

bool createCustomerUser(const std::string& username, const std::string& password) {
	if (!customerUsers.contains(username) && username != std::string(SHADOW_ADMIN_USERNAME)) {
		if (password.empty()) {
			std::cout << "Password cannot be empty." << std::endl;
			return false;
		}
		if (password.length() < 8) {
			std::cout << "Password must be at least 8 characters long." << std::endl;
			return false;
		}
		customerUsers.emplace(username, Customer(username, password));
		return true;
	}
	else {
		std::cout << "Customer user with username '" << username << "' already exists." << std::endl;
		return false;
	}
}

template <typename U> void saveAccounts(const std::unordered_map<std::string, U>& userCategory) {
	SQLite3DB* db = SQLite3DB::getInstance();
	if (!db->isConnected()) {
		std::cout << "Database is not connected. Cannot save accounts." << std::endl;
		return;
	}

	if constexpr (std::is_same_v<U, Admin>) {
		// Save Admin users to admin_users table
		for (const auto& [username, acc] : userCategory) {
			if (username == "Shadow as admin.") continue;

			const NextOfKin nok = acc.getNextOfKin();

			// Check if record exists
			std::string checkSql = "SELECT 1 FROM admin_users WHERE username = '" + escapeSql(username) + "'";
			auto results = db->executeQuery(checkSql);
			bool exists = !results.empty();

			std::ostringstream sql;
			if (exists) {
				std::string genderVal;
				char g = acc.getGender();
				if (g == '\0') {
					genderVal = "NULL";
				}
				else {
					genderVal = std::string("'") + g + "'";
				}

				sql << "UPDATE admin_users SET "
					<< "account_name = '" << escapeSql(acc.getName()) << "', "
					<< "password_hash = '" << escapeSql(acc.getPasswordHash()) << "', "
					<< "password_salt = '" << escapeSql(acc.getPasswordSalt()) << "', "
					<< "phone_number = '" << escapeSql(acc.getPhoneNumber()) << "', "
					<< "gender = " << genderVal << ", "
					<< "address = '" << escapeSql(acc.getAddress()) << "', "
					<< "email = '" << escapeSql(acc.getEmail()) << "', "
					<< "department = '" << escapeSql(acc.getDepartment()) << "', "
					<< "admin_level = '" << escapeSql(acc.getAdminLevel()) << "', "
					<< "hire_date = '" << acc.getHireDate() << "', "
					<< "nok_name = '" << escapeSql(nok.name) << "', "
					<< "nok_relationship = '" << escapeSql(nok.relationship) << "', "
					<< "nok_phone = '" << escapeSql(nok.phoneNumber) << "', "
					<< "nok_address = '" << escapeSql(nok.address) << "' "
					<< "WHERE username = '" << escapeSql(username) << "'";
			}
			else {
				std::string genderVal;
				char g = acc.getGender();
				if (g == '\0') {
					genderVal = "NULL";
				}
				else {
					genderVal = std::string("'") + g + "'";
				}

				sql << "INSERT INTO admin_users (username, account_name, password_hash, password_salt, phone_number, gender, address, email, department, admin_level, hire_date, nok_name, nok_relationship, nok_phone, nok_address) "
					<< "VALUES ('" << escapeSql(username) << "', '" << escapeSql(acc.getName()) << "', '" << escapeSql(acc.getPasswordHash()) << "', '" << escapeSql(acc.getPasswordSalt()) << "', "
					<< "'" << escapeSql(acc.getPhoneNumber()) << "', " << genderVal << ", '" << escapeSql(acc.getAddress()) << "', '" << escapeSql(acc.getEmail()) << "', "
					<< "'" << escapeSql(acc.getDepartment()) << "', '" << escapeSql(acc.getAdminLevel()) << "', '" << acc.getHireDate() << "', "
					<< "'" << escapeSql(nok.name) << "', '" << escapeSql(nok.relationship) << "', '" << escapeSql(nok.phoneNumber) << "', '" << escapeSql(nok.address) << "')";
			}

			if (!db->executeUpdate(sql.str())) {
				std::cout << "Failed to save admin user: " << username << std::endl;
			}
		}
	}
	else if constexpr (std::is_same_v<U, Customer>) {
		// Save Customer users to customer_users table
		for (const auto& [username, acc] : userCategory) {
			if (username == "Shadow as admin.") continue;

			const NextOfKin nok = acc.getNextOfKin();

			// Check if record exists
			std::string checkSql = "SELECT 1 FROM customer_users WHERE username = '" + escapeSql(username) + "'";
			auto results = db->executeQuery(checkSql);
			bool exists = !results.empty();

			std::string genderVal;
			char g = acc.getGender();
			if (g == '\0') {
				genderVal = "NULL";
			}
			else {
				genderVal = std::string("'") + g + "'";
			}

			std::ostringstream sql;
			if (exists) {
				sql << "UPDATE customer_users SET "
					<< "account_name = '" << escapeSql(acc.getName()) << "', "
					<< "password_hash = '" << escapeSql(acc.getPasswordHash()) << "', "
					<< "password_salt = '" << escapeSql(acc.getPasswordSalt()) << "', "
					<< "phone_number = '" << escapeSql(acc.getPhoneNumber()) << "', "
					<< "gender = " << genderVal << ", "
					<< "address = '" << escapeSql(acc.getAddress()) << "', "
					<< "email = '" << escapeSql(acc.getEmail()) << "', "
					<< "total_bookings = " << acc.getTotalBookings() << ", "
					<< "loyalty_points = " << acc.getLoyaltyPoints() << ", "
					<< "account_balance = " << acc.getAccountBalance() << ", "
					<< "nok_name = '" << escapeSql(nok.name) << "', "
					<< "nok_relationship = '" << escapeSql(nok.relationship) << "', "
					<< "nok_phone = '" << escapeSql(nok.phoneNumber) << "', "
					<< "nok_address = '" << escapeSql(nok.address) << "' "
					<< "WHERE username = '" << escapeSql(username) << "'";
			}
			else {
				sql << "INSERT INTO customer_users (username, account_name, password_hash, password_salt, phone_number, gender, address, email, total_bookings, loyalty_points, account_creation_date, account_balance, nok_name, nok_relationship, nok_phone, nok_address) "
					<< "VALUES ('" << escapeSql(username) << "', '" << escapeSql(acc.getName()) << "', '" << escapeSql(acc.getPasswordHash()) << "', '" << escapeSql(acc.getPasswordSalt()) << "', "
					<< "'" << escapeSql(acc.getPhoneNumber()) << "', " << genderVal << ", '" << escapeSql(acc.getAddress()) << "', '" << escapeSql(acc.getEmail()) << "', "
					<< acc.getTotalBookings() << ", " << acc.getLoyaltyPoints() << ", "
					<< "'" << acc.getAccountCreationDate() << "', " << acc.getAccountBalance() << ", "
					<< "'" << escapeSql(nok.name) << "', '" << escapeSql(nok.relationship) << "', '" << escapeSql(nok.phoneNumber) << "', '" << escapeSql(nok.address) << "')";
			}

			if (!db->executeUpdate(sql.str())) {
				std::cout << "Failed to save customer user: " << username << std::endl;
			}

			// Save transaction history
			std::ostringstream deleteTxSql;
			deleteTxSql << "DELETE FROM customer_transaction_history WHERE username = '" << escapeSql(username) << "'";
			db->executeUpdate(deleteTxSql.str());

			for (const auto& tx : acc.getTransactionHistory()) {
				std::ostringstream txSql;
				txSql << "INSERT INTO customer_transaction_history (username, transaction_text) VALUES ('" << escapeSql(username) << "', '" << escapeSql(tx) << "')";
				db->executeUpdate(txSql.str());
			}

			// Save booked flights
			std::ostringstream deleteFlightsSql;
			deleteFlightsSql << "DELETE FROM customer_booked_flights WHERE username = '" << escapeSql(username) << "'";
			db->executeUpdate(deleteFlightsSql.str());

			for (const auto& flight : acc.getBookedFlights()) {
				std::ostringstream flightSql;
				flightSql << "INSERT INTO customer_booked_flights (username, flight_id) VALUES ('" << escapeSql(username) << "', '" << escapeSql(flight.getId()) << "')";
				db->executeUpdate(flightSql.str());
			}
		}
	}
}

template <typename U> void loadAccounts(std::unordered_map<std::string, U>& userCategory) {
	SQLite3DB* db = SQLite3DB::getInstance();
	if (!db->isConnected()) {
		std::cout << "Database is not connected. Cannot load accounts." << std::endl;
		return;
	}

	userCategory.clear();

	if constexpr (std::is_same_v<U, Admin>) {
		// Load Admin users from admin_users table
		std::string sql = "SELECT username, account_name, password_hash, password_salt, phone_number, gender, address, email, department, admin_level, hire_date, nok_name, nok_relationship, nok_phone, nok_address FROM admin_users WHERE username != 'Shadow as admin.'";
		auto results = db->executeQuery(sql);

		for (const auto& row : results) {
			if (row.size() < 15) continue;

			const std::string username = row[0];
			const std::string aName = row[1];
			const std::string aPasswordHash = row[2];
			const std::string aPasswordSalt = row[3];
			const std::string aPhoneNumber = row[4];
			const char aGender = !row[5].empty() ? row[5][0] : '\0';
			const std::string aAddress = row[6];
			const std::string aEmail = row[7];
			const std::string aDepartment = row[8];
			const std::string aAdminLevel = row[9];
			const std::string aHireDate = row[10];
			NextOfKin nok{ row[11], row[12], row[13], row[14] };

			Admin admin(aName, "", aPhoneNumber, aGender, aAddress, aEmail, nok, aDepartment, aAdminLevel, aHireDate);
			admin.setPasswordHashAndSalt(aPasswordHash, aPasswordSalt);
			userCategory.emplace(username, admin);
		}
	}
	else if constexpr (std::is_same_v<U, Customer>) {
		// Load Customer users from customer_users table
		std::string sql = "SELECT username, account_name, password_hash, password_salt, phone_number, gender, address, email, total_bookings, loyalty_points, account_creation_date, account_balance, nok_name, nok_relationship, nok_phone, nok_address FROM customer_users WHERE username != 'Shadow as admin.'";
		auto results = db->executeQuery(sql);

		for (const auto& row : results) {
			if (row.size() < 16) continue;

			const std::string username = row[0];
			const std::string aName = row[1];
			const std::string aPasswordHash = row[2];
			const std::string aPasswordSalt = row[3];
			const std::string aPhoneNumber = row[4];
			const char aGender = !row[5].empty() ? row[5][0] : '\0';
			const std::string aAddress = row[6];
			const std::string aEmail = row[7];
			const int aTotalBookings = std::stoi(row[8]);
			const double aLoyaltyPoints = std::stod(row[9]);
			const std::string aAccountCreationDate = row[10];
			const double aAccountBalance = std::stod(row[11]);
			NextOfKin nok{ row[12], row[13], row[14], row[15] };

			// Load transaction history
			std::ostringstream txSql;
			txSql << "SELECT transaction_text FROM customer_transaction_history WHERE username = '" << escapeSql(username) << "' ORDER BY ROWID";
			auto txResults = db->executeQuery(txSql.str());
			std::vector<std::string> aTransactionHistory;
			for (const auto& txRow : txResults) {
				if (!txRow.empty()) {
					aTransactionHistory.push_back(txRow[0]);
				}
			}

			// Load booked flights
			std::ostringstream flightSql;
			flightSql << "SELECT flight_id FROM customer_booked_flights WHERE username = '" << escapeSql(username) << "' ORDER BY ROWID";
			auto flightResults = db->executeQuery(flightSql.str());
			std::vector<Flight> aBookedFlights;
			aBookedFlights.reserve(flightResults.size());
			for (const auto& flightRow : flightResults) {
				if (!flightRow.empty()) {
					auto it = flights.find(flightRow[0]);
					if (it != flights.end()) {
						aBookedFlights.push_back(it->second);
					}
				}
			}

			Customer customer(aName, "", aPhoneNumber, aGender, aAddress, aEmail, nok, aTotalBookings, aLoyaltyPoints, aAccountCreationDate, aAccountBalance, aTransactionHistory, aBookedFlights);
			customer.setPasswordHashAndSalt(aPasswordHash, aPasswordSalt);
			userCategory.emplace(username, customer);
		}
	}
}

void initializeDatabase() {
	SQLite3DB* db = SQLite3DB::getInstance();
	if (!db->connect("data/flight_reservation.db")) {
		std::cout << "Failed to connect to SQLite3 database." << std::endl;
		return;
	}

	// Create tables if they don't exist
	std::vector<std::string> createTableSqls = {
		// Admin users table
		"CREATE TABLE IF NOT EXISTS admin_users ("
		"username TEXT PRIMARY KEY,"
		"account_name TEXT NOT NULL,"
		"password_hash TEXT NOT NULL,"
		"password_salt TEXT NOT NULL,"
		"phone_number TEXT,"
		"gender CHAR(1) DEFAULT NULL,"
		"address TEXT,"
		"email TEXT,"
		"department TEXT,"
		"admin_level TEXT,"
		"hire_date TEXT,"
		"nok_name TEXT,"
		"nok_relationship TEXT,"
		"nok_phone TEXT,"
		"nok_address TEXT"
		");",

		// Customer users table
		"CREATE TABLE IF NOT EXISTS customer_users ("
		"username TEXT PRIMARY KEY,"
		"account_name TEXT NOT NULL,"
		"password_hash TEXT NOT NULL,"
		"password_salt TEXT NOT NULL,"
		"phone_number TEXT,"
		"gender CHAR(1) DEFAULT NULL,"
		"address TEXT,"
		"email TEXT,"
		"total_bookings INTEGER DEFAULT 0,"
		"loyalty_points REAL DEFAULT 0.0,"
		"account_creation_date TEXT,"
		"account_balance REAL DEFAULT 0.0,"
		"nok_name TEXT,"
		"nok_relationship TEXT,"
		"nok_phone TEXT,"
		"nok_address TEXT"
		");"

		// Customer transaction history table
		"CREATE TABLE IF NOT EXISTS customer_transaction_history ("
		"username TEXT NOT NULL,"
		"transaction_text TEXT NOT NULL,"
		"FOREIGN KEY (username) REFERENCES customer_users(username) ON DELETE CASCADE"
		");",

		// Customer booked flights table
		"CREATE TABLE IF NOT EXISTS customer_booked_flights ("
		"username TEXT NOT NULL,"
		"flight_id TEXT NOT NULL,"
		"FOREIGN KEY (username) REFERENCES customer_users(username) ON DELETE CASCADE"
		");",

		// Payment references tracking table
		"CREATE TABLE IF NOT EXISTS used_payment_references ("
		"reference TEXT PRIMARY KEY,"
		"used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
		");"
	};

	for (const auto& sql : createTableSqls) {
		if (!db->executeUpdate(sql)) {
			std::cout << "Failed to create table." << std::endl;
		}
	}

	std::cout << "Database initialization complete." << std::endl;
}

void loadAccounts() {
	SQLite3DB* db = SQLite3DB::getInstance();

	// Ensure database is connected
	if (!db->isConnected()) {
		db->connect("data/flight_reservation.db");
	}

	// Ensure the used_payment_references table exists
	std::string createTableSql = "CREATE TABLE IF NOT EXISTS used_payment_references ("
		"reference TEXT PRIMARY KEY,"
		"used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
		")";
	db->executeUpdate(createTableSql);

	loadAccounts(customerUsers);
	loadAccounts(adminUsers);

	// Try to find super admin password from config file
	const std::array<std::string, 4> configPaths{
		"config/super_admin.txt",        // when running from repo root
		"../config/super_admin.txt",     // when running from build/ or src/
		"../../config/super_admin.txt",  // when running from build/bin-like subfolders
		"./config/super_admin.txt"       // explicit current dir
	};
	std::string adminPassword;
	bool adminConfigFound = false;

	for (const auto& configPath : configPaths) {
		std::ifstream configFile(configPath);
		if (configFile.is_open()) {
			std::getline(configFile, adminPassword);
			configFile.close();
			if (!adminPassword.empty()) {
				adminConfigFound = true;
				break;
			}
		}
	}

	// If no config file found or empty, use default password
	if (!adminConfigFound || adminPassword.empty()) {
		adminPassword = "Kamar-Taj";
	}

	// Create super admin account
	adminUsers.emplace(std::string(SHADOW_ADMIN_USERNAME), 
		Admin("Gerald Mathew", adminPassword, "2348122997160", 'M', "Federal University of Technology, Owerri", "mathewgerald.14@gmail.com", NextOfKin{}, "Super Admin", "MAX", "2026-01-01"));
}

void saveAccounts() {
	SQLite3DB* db = SQLite3DB::getInstance();

	// Ensure database is connected
	if (!db->isConnected()) {
		db->connect("data/flight_reservation.db");
	}

	saveAccounts(customerUsers);
	saveAccounts(adminUsers);
}

template <typename U> void saveCurrentUser(const std::string& username, const U& user) {
	SQLite3DB* db = SQLite3DB::getInstance();
	if (!db->isConnected()) {
		std::cout << "Database is not connected. Cannot save account." << std::endl;
		return;
	}

	if constexpr (std::is_same_v<U, Admin>) {
		// Save a single Admin user
		const NextOfKin nok = user.getNextOfKin();

		// Check if record exists
		std::string checkSql = "SELECT 1 FROM admin_users WHERE username = '" + escapeSql(username) + "'";
		auto results = db->executeQuery(checkSql);
		bool exists = !results.empty();

		std::ostringstream sql;
		if (exists) {
			sql << "UPDATE admin_users SET "
				<< "account_name = '" << escapeSql(user.getName()) << "', "
				<< "password_hash = '" << escapeSql(user.getPasswordHash()) << "', "
				<< "password_salt = '" << escapeSql(user.getPasswordSalt()) << "', "
				<< "phone_number = '" << escapeSql(user.getPhoneNumber()) << "', "
				<< "gender = '" << user.getGender() << "', "
				<< "address = '" << escapeSql(user.getAddress()) << "', "
				<< "email = '" << escapeSql(user.getEmail()) << "', "
				<< "department = '" << escapeSql(user.getDepartment()) << "', "
				<< "admin_level = '" << escapeSql(user.getAdminLevel()) << "', "
				<< "hire_date = '" << user.getHireDate() << "', "
				<< "nok_name = '" << escapeSql(nok.name) << "', "
				<< "nok_relationship = '" << escapeSql(nok.relationship) << "', "
				<< "nok_phone = '" << escapeSql(nok.phoneNumber) << "', "
				<< "nok_address = '" << escapeSql(nok.address) << "' "
				<< "WHERE username = '" << escapeSql(username) << "'";
		}
		else {
			sql << "INSERT INTO admin_users (username, account_name, password_hash, password_salt, phone_number, gender, address, email, department, admin_level, hire_date, nok_name, nok_relationship, nok_phone, nok_address) "
				<< "VALUES ('" << escapeSql(username) << "', '" << escapeSql(user.getName()) << "', '" << escapeSql(user.getPasswordHash()) << "', '" << escapeSql(user.getPasswordSalt()) << "', "
				<< "'" << escapeSql(user.getPhoneNumber()) << "', '" << user.getGender() << "', '" << escapeSql(user.getAddress()) << "', '" << escapeSql(user.getEmail()) << "', "
				<< "'" << escapeSql(user.getDepartment()) << "', '" << escapeSql(user.getAdminLevel()) << "', '" << user.getHireDate() << "', "
				<< "'" << escapeSql(nok.name) << "', '" << escapeSql(nok.relationship) << "', '" << escapeSql(nok.phoneNumber) << "', '" << escapeSql(nok.address) << "')";
		}

		if (!db->executeUpdate(sql.str())) {
			std::cout << "Failed to save admin user: " << username << std::endl;
		}
	}
	else if constexpr (std::is_same_v<U, Customer>) {
		// Save a single Customer user
		const NextOfKin nok = user.getNextOfKin();

		// Check if record exists
		std::string checkSql = "SELECT 1 FROM customer_users WHERE username = '" + escapeSql(username) + "'";
		auto results = db->executeQuery(checkSql);
		bool exists = !results.empty();

		std::ostringstream sql;
		if (exists) {
			sql << "UPDATE customer_users SET "
				<< "account_name = '" << escapeSql(user.getName()) << "', "
				<< "password_hash = '" << escapeSql(user.getPasswordHash()) << "', "
				<< "password_salt = '" << escapeSql(user.getPasswordSalt()) << "', "
				<< "phone_number = '" << escapeSql(user.getPhoneNumber()) << "', "
				<< "gender = '" << user.getGender() << "', "
				<< "address = '" << escapeSql(user.getAddress()) << "', "
				<< "email = '" << escapeSql(user.getEmail()) << "', "
				<< "total_bookings = " << user.getTotalBookings() << ", "
				<< "loyalty_points = " << user.getLoyaltyPoints() << ", "
				<< "account_balance = " << user.getAccountBalance() << ", "
				<< "nok_name = '" << escapeSql(nok.name) << "', "
				<< "nok_relationship = '" << escapeSql(nok.relationship) << "', "
				<< "nok_phone = '" << escapeSql(nok.phoneNumber) << "', "
				<< "nok_address = '" << escapeSql(nok.address) << "' "
				<< "WHERE username = '" << escapeSql(username) << "'";
		}
		else {
			sql << "INSERT INTO customer_users (username, account_name, password_hash, password_salt, phone_number, gender, address, email, total_bookings, loyalty_points, account_creation_date, account_balance, nok_name, nok_relationship, nok_phone, nok_address) "
				<< "VALUES ('" << escapeSql(username) << "', '" << escapeSql(user.getName()) << "', '" << escapeSql(user.getPasswordHash()) << "', '" << escapeSql(user.getPasswordSalt()) << "', "
				<< "'" << escapeSql(user.getPhoneNumber()) << "', '" << user.getGender() << "', '" << escapeSql(user.getAddress()) << "', '" << escapeSql(user.getEmail()) << "', "
				<< user.getTotalBookings() << ", " << user.getLoyaltyPoints() << ", "
				<< "'" << user.getAccountCreationDate() << "', " << user.getAccountBalance() << ", "
				<< "'" << escapeSql(nok.name) << "', '" << escapeSql(nok.relationship) << "', '" << escapeSql(nok.phoneNumber) << "', '" << escapeSql(nok.address) << "')";
		}

		if (!db->executeUpdate(sql.str())) {
			std::cout << "Failed to save customer user: " << username << std::endl;
		}

		// Save transaction history
		std::ostringstream deleteTxSql;
		deleteTxSql << "DELETE FROM customer_transaction_history WHERE username = '" << escapeSql(username) << "'";
		db->executeUpdate(deleteTxSql.str());

		for (const auto& tx : user.getTransactionHistory()) {
			std::ostringstream txSql;
			txSql << "INSERT INTO customer_transaction_history (username, transaction_text) VALUES ('" << escapeSql(username) << "', '" << escapeSql(tx) << "')";
			db->executeUpdate(txSql.str());
		}

		// Save booked flights
		std::ostringstream deleteFlightsSql;
		deleteFlightsSql << "DELETE FROM customer_booked_flights WHERE username = '" << escapeSql(username) << "'";
		db->executeUpdate(deleteFlightsSql.str());

		for (const auto& flight : user.getBookedFlights()) {
			std::ostringstream flightSql;
			flightSql << "INSERT INTO customer_booked_flights (username, flight_id) VALUES ('" << escapeSql(username) << "', '" << escapeSql(flight.getId()) << "')";
			db->executeUpdate(flightSql.str());
		}
	}
}

void saveCurrentUserAccount(const std::string& username, const std::string& userType) {
	if (userType == "admin") {
		if (adminUsers.contains(username)) {
			saveCurrentUser(username, adminUsers.at(username));
		}
		else {
			std::cout << "Admin user '" << username << "' not found." << std::endl;
		}
	}
	else if (userType == "customer") {
		if (customerUsers.contains(username)) {
			saveCurrentUser(username, customerUsers.at(username));
		}
		else {
			std::cout << "Customer user '" << username << "' not found." << std::endl;
		}
	}
	else {
		std::cout << "Invalid user type. Use 'admin' or 'customer'." << std::endl;
	}
}

// Explicit template instantiations for Admin and Customer
// These instantiate the template functions for both user types to satisfy linker
template std::string validateUser<Admin>(const std::unordered_map<std::string, Admin>& userCategory);
template std::string validateUser<Customer>(const std::unordered_map<std::string, Customer>& userCategory);

template bool deleteUser<Admin>(const std::string& username, std::unordered_map<std::string, Admin>& userCategory);
template bool deleteUser<Customer>(const std::string& username, std::unordered_map<std::string, Customer>& userCategory);

template void saveAccounts<Admin>(const std::unordered_map<std::string, Admin>& userCategory);
template void saveAccounts<Customer>(const std::unordered_map<std::string, Customer>& userCategory);

template void saveCurrentUser<Admin>(const std::string& username, const Admin& user);
template void saveCurrentUser<Customer>(const std::string& username, const Customer& user);

template void loadAccounts<Admin>(std::unordered_map<std::string, Admin>& userCategory);
template void loadAccounts<Customer>(std::unordered_map<std::string, Customer>& userCategory);

bool isPaymentReferenceUsed(const std::string& reference) {
	SQLite3DB* db = SQLite3DB::getInstance();
	if (!db->isConnected()) {
		return false;
	}

	std::ostringstream sql;
	sql << "SELECT 1 FROM used_payment_references WHERE reference = '" << escapeSql(reference) << "'";
	auto results = db->executeQuery(sql.str());
	return !results.empty();
}

bool markPaymentReferenceAsUsed(const std::string& reference) {
	SQLite3DB* db = SQLite3DB::getInstance();
	if (!db->isConnected()) {
		std::cout << "Database is not connected. Cannot mark reference as used." << std::endl;
		return false;
	}

	std::ostringstream sql;
	sql << "INSERT INTO used_payment_references (reference) VALUES ('" << escapeSql(reference) << "')";
	return db->executeUpdate(sql.str());
}