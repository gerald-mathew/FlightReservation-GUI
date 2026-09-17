#include "flights.hpp"
#include "sqlite3db.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <regex>
#include <string>
#include <vector>
#include <memory>

// Time conversion utilities
std::chrono::system_clock::time_point Flight::parseDateTime(const std::string& dateTime) {
	// Expected format: YYYY-MM-DD HH:MM
	try {
		std::istringstream iss(dateTime);
		std::tm tm = {};
		iss >> std::get_time(&tm, "%Y-%m-%d %H:%M");
		if (iss.fail()) {
			std::cout << "Failed to parse datetime: " << dateTime << std::endl;
			return std::chrono::system_clock::now();
		}
		time_t time = std::mktime(&tm);
		return std::chrono::system_clock::from_time_t(time);
	}
	catch (const std::exception& e) {
		std::cout << "Error parsing datetime: " << e.what() << std::endl;
		return std::chrono::system_clock::now();
	}
}

std::string Flight::timePointToString(const std::chrono::system_clock::time_point& tp) {
	auto time = std::chrono::system_clock::to_time_t(tp);
	std::tm tm_info{};
#ifdef _WIN32
	localtime_s(&tm_info, &time);
#else
	if (const std::tm* local = std::localtime(&time)) {
		tm_info = *local;
	}
#endif
	char buffer[20];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm_info);
	return std::string(buffer);
}

std::unique_ptr<Seat> Flight::makeSeatForRow(int row, char letter) {
	// Factory method: Creates heap-allocated seat objects via unique_ptr (auto-cleanup)
	// Simple cabin split: rows 1-5 First Class, 6-15 Business, rest Economy.
	if (row <= 5) return std::make_unique<FirstClassSeat>(row, letter);
	if (row <= 15) return std::make_unique<BusinessSeat>(row, letter);
	return std::make_unique<EconomySeat>(row, letter);
}

void Flight::initializeSeats(int seatCount) {
	seats.clear();
	seats.reserve(static_cast<size_t>(seatCount));

	// Seat layout: 4 seats per row (A-D) eg. 1A, 1B, 1C, 1D.
	constexpr int colsPerRow = 4;
	const int rows = (seatCount + colsPerRow - 1) / colsPerRow;

	int created = 0;
	for (int row = 1; row <= rows && created < seatCount; ++row) {
		for (int col = 0; col < colsPerRow && created < seatCount; ++col) {
			const char letter = static_cast<char>('A' + col);
			seats.push_back(makeSeatForRow(row, letter));
			++created;
		}
	}
}

std::unique_ptr<Seat> Flight::seatFromJson(const nlohmann::json& js) {
	// Deserialize seat from JSON: defensive pattern uses contains() to check key existence
	// then get<T>() to extract typed values safely
	const std::string seatNumber = js.at("Seat Number").is_null() ? "" : js.at("Seat Number").get<std::string>();
	if (seatNumber.size() < 2) return {};

	const std::string rowPart = seatNumber.substr(0, seatNumber.size() - 1);
	const char letter = static_cast<char>(std::toupper(static_cast<unsigned char>(seatNumber.back())));
	const int row = std::max(1, std::stoi(rowPart));

	auto cabin = Seat::cabinFromString(js.at("Cabin").is_null() ? "Economy" : js.at("Cabin").get<std::string>());
	// Factory pattern: create appropriate seat subclass via unique_ptr for polymorphic handling
	std::unique_ptr<Seat> seat;
	switch (cabin) {
	case Seat::Cabin::FirstClass:
		seat = std::make_unique<FirstClassSeat>(row, letter);
		break;
	case Seat::Cabin::Business:
		seat = std::make_unique<BusinessSeat>(row, letter);
		break;
	default:
		seat = std::make_unique<EconomySeat>(row, letter);
		break;
	}

	const bool isBooked = js.at("Is Booked").is_null() ? false : js.at("Is Booked").get<bool>();
	if (isBooked) {
		seat->book(js.at("Booked By").is_null() ? "" : js.at("Booked By").get<std::string>());
		// Restore booked time if available
		if (js.contains("Booked Time") && !js["Booked Time"].is_null()) {
			std::string bookedTimeStr = js["Booked Time"].get<std::string>();
			auto restoredTime = parseDateTime(bookedTimeStr);
			// Need to set the bookedTime directly - we'll need a setter in Seat
			seat->setBookedTime(restoredTime);
		}
	}
	return seat;
}

std::string Flight::statusToString(Flight::Status s) {
	switch (s) {
	case Status::Scheduled: return "Scheduled";
	case Status::Boarding: return "Boarding";
	case Status::Departed: return "Departed";
	case Status::Delayed: return "Delayed";
	case Status::Cancelled: return "Cancelled";
	}
	return "Scheduled";
}

Flight::Status Flight::statusFromString(std::string v) {
	std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	if (v == "BOARDING") return Status::Boarding;
	if (v == "DEPARTED") return Status::Departed;
	if (v == "DELAYED") return Status::Delayed;
	if (v == "CANCELLED" || v == "CANCELED") return Status::Cancelled;
	return Status::Scheduled;
}

bool Flight::isValidAirportCode(const std::string& code) {
	std::string upperCase = code;
	std::transform(upperCase.begin(), upperCase.end(), upperCase.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

	return airports.contains(upperCase);
}

bool Flight::isValidDateTimeFormat(const std::string& dateTime) {
	// Accept YYYY-MM-DD HH:MM with optional zero padding on the month, day and
	// hour fields (e.g. both "2026-06-15 09:30" and "2026-6-15 9:30" are valid).
	const std::regex dateTimepattern(R"(^\d{4}-\d{1,2}-\d{1,2} \d{1,2}:\d{2}$)");
	return std::regex_match(dateTime, dateTimepattern);
}

Flight::Flight() : totalSeats(DefaultSeats), availableSeats(DefaultSeats) {
	initializeSeats(DefaultSeats);
}

Flight::Flight(const Flight& other)
	: id(other.id), airline(other.airline), origin(other.origin), destination(other.destination),
	departureTime(other.departureTime), arrivalTime(other.arrivalTime), terminal(other.terminal),
	gate(other.gate), totalSeats(other.totalSeats), availableSeats(other.availableSeats),
	ticketPrice(other.ticketPrice), status(other.status) {
	// Deep copy unique_ptr vector: clone() polymorphically creates subclass instances
	seats.reserve(other.seats.size());
	for (const auto& s : other.seats) {
		seats.push_back(s ? s->clone() : nullptr);
	}
}

Flight& Flight::operator=(const Flight& other) {
	if (this == &other) return *this;
	id = other.id;
	airline = other.airline;
	origin = other.origin;
	destination = other.destination;
	departureTime = other.departureTime;
	arrivalTime = other.arrivalTime;
	terminal = other.terminal;
	gate = other.gate;
	totalSeats = other.totalSeats;
	availableSeats = other.availableSeats;
	ticketPrice = other.ticketPrice;
	status = other.status;
	// Deep copy seats: clear and realloc unique_ptrs with polymorphic cloning
	seats.clear();
	seats.reserve(other.seats.size());
	for (const auto& s : other.seats) {
		seats.push_back(s ? s->clone() : nullptr);
	}
	return *this;
}

Flight::Flight(std::string aId, std::string anAirline, std::string anOrigin, std::string aDestination,
	std::chrono::system_clock::time_point aDepartureTime, std::chrono::system_clock::time_point anArrivalTime, 
	int aTotalSeats, double aTicketPrice)
	: id(std::move(aId)), airline(std::move(anAirline)), departureTime(aDepartureTime),
	arrivalTime(anArrivalTime), totalSeats(aTotalSeats), availableSeats(aTotalSeats),
	ticketPrice(aTicketPrice) {

	if (totalSeats <= 0) totalSeats = DefaultSeats;
	if (totalSeats > MaxSeats) totalSeats = MaxSeats;
	availableSeats = totalSeats;

	// Validate times
	auto now = std::chrono::system_clock::now();
	if (departureTime < now) {
		setStatus(Status::Departed); // Automatically mark as Departed if departure time is in the past
	}
	if (arrivalTime <= departureTime) {
		arrivalTime = departureTime + std::chrono::hours(3); // Default 3-hour flight
	}

	initializeSeats(totalSeats);
	setOrigin(std::move(anOrigin));
	setDestination(std::move(aDestination));
	updateStatus();
}

bool Flight::setId(std::string aId) {
	if (aId.empty()) return false;
	id = std::move(aId);
	return true;
}

bool Flight::setAirline(std::string anAirline) {
	if (anAirline.empty()) return false;
	airline = std::move(anAirline);
	return true;
}

bool Flight::setOrigin(std::string anOrigin) {
	std::transform(anOrigin.begin(), anOrigin.end(), anOrigin.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	if (!isValidAirportCode(anOrigin) || anOrigin == destination) return false;
	origin = std::move(anOrigin);
	return true;
}

bool Flight::setDestination(std::string aDestination) {
	std::transform(aDestination.begin(), aDestination.end(), aDestination.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	if (!isValidAirportCode(aDestination) || aDestination == origin) return false;
	destination = std::move(aDestination);
	return true;
}

bool Flight::setDepartureTime(std::chrono::system_clock::time_point aDepartureTime) {
	auto now = std::chrono::system_clock::now();
	if (aDepartureTime < now) {
		std::cout << "Departure time cannot be in the past! Current time: " << timePointToString(now) << std::endl;
		return false;
	}

	// Don't allow changing departure time if explicitly marked as Departed or Cancelled by admin
	if (status == Status::Departed || status == Status::Cancelled) {
		std::cout << "Cannot change departure time for a " << statusToString(status) << " flight!" << std::endl;
		return false;
	}

	departureTime = aDepartureTime;
	updateStatus();
	return true;
}

bool Flight::setDepartureTimeFromString(const std::string& dateTime) {
	if (!isValidDateTimeFormat(dateTime)) {
		std::cout << "Invalid departure time format. Please use YYYY-MM-DD HH:MM." << std::endl;
		return false;
	}
	return setDepartureTime(parseDateTime(dateTime));
}

bool Flight::setArrivalTime(std::chrono::system_clock::time_point anArrivalTime) {
	if (anArrivalTime <= departureTime) {
		std::cout << "Arrival time must be after departure time!" << std::endl;
		return false;
	}

	// Don't allow changing arrival time if explicitly marked as Departed or Cancelled
	if (status == Status::Departed || status == Status::Cancelled) {
		std::cout << "Cannot change arrival time for a " << statusToString(status) << " flight!" << std::endl;
		return false;
	}

	arrivalTime = anArrivalTime;
	return true;
}

bool Flight::setArrivalTimeFromString(const std::string& dateTime) {
	if (!isValidDateTimeFormat(dateTime)) {
		std::cout << "Invalid arrival time format. Please use YYYY-MM-DD HH:MM." << std::endl;
		return false;
	}
	return setArrivalTime(parseDateTime(dateTime));
}

bool Flight::setTotalSeats(int aTotalSeats) {
	if (aTotalSeats <= 0 || aTotalSeats > MaxSeats) {
		std::cout << "Total seats cannot be greater than " << MaxSeats << std::endl;
		return false;
	}

	// To keep seat numbers consistent, only allow resizing when no seats are booked.
	const int bookedSeats = getBookedSeats();
	if (bookedSeats != 0) {
		std::cout << "Seats cannot be resized when a seat is booked" << std::endl;
		return false;
	}
	totalSeats = aTotalSeats;
	availableSeats = totalSeats;
	initializeSeats(totalSeats);
	return true;
}

bool Flight::setTicketPrice(double aTicketPrice) {
	if (aTicketPrice < 0.0) return false;
	ticketPrice = aTicketPrice;
	return true;
}
void Flight::setTerminal(std::string aTerminal) { terminal = std::move(aTerminal); }
void Flight::setGate(std::string aGate) { gate = std::move(aGate); }
void Flight::setStatus(Status aStatus) { status = aStatus; }

std::string Flight::getId() const { return id; }
std::string Flight::getAirline() const { return airline; }
std::string Flight::getOrigin() const { return origin; }
std::string Flight::getDestination() const { return destination; }
std::string Flight::getDepartureTime() const { return timePointToString(departureTime); }
std::string Flight::getArrivalTime() const { return timePointToString(arrivalTime); }
std::chrono::system_clock::time_point Flight::getDepartureTimePoint() const { return departureTime; }
std::chrono::system_clock::time_point Flight::getArrivalTimePoint() const { return arrivalTime; }
std::string Flight::getTerminal() const { return terminal; }
std::string Flight::getGate() const { return gate; }
int Flight::getTotalSeats() const { return totalSeats; }
int Flight::getAvailableSeats() const { return availableSeats; }
double Flight::getTicketPrice() const { return ticketPrice; }
Flight::Status Flight::getStatus() const { return status; }

// Time-based methods
bool Flight::isDeparted() const {
	return status == Status::Departed || std::chrono::system_clock::now() >= departureTime;
}

bool Flight::isBoarding() const {
	auto now = std::chrono::system_clock::now();
	auto boardingTime = departureTime - std::chrono::minutes(30);
	return status != Status::Cancelled && status != Status::Departed && now >= boardingTime && now < departureTime;
}

int Flight::getMinutesUntilDeparture() const {
	auto now = std::chrono::system_clock::now();
	if (now >= departureTime) return 0;
	auto duration = std::chrono::duration_cast<std::chrono::minutes>(departureTime - now);
	return static_cast<int>(duration.count());
}

void Flight::updateStatus() {
	if (status == Status::Cancelled) return; // Don't update cancelled flights

	auto now = std::chrono::system_clock::now();

	if (now >= departureTime) {
		status = Status::Departed;
	} else if (now >= (departureTime - std::chrono::minutes(30))) {
		status = Status::Boarding;
	} else {
		status = Status::Scheduled;
	}
}

int Flight::getBookedSeats() const { return totalSeats - availableSeats; }

void Flight::recountAvailableSeats() {
	// Derive availableSeats/totalSeats from the actual seat objects. The DB load
	// path rebuilds the seat vector after construction, so the constructor's
	// optimistic availableSeats == totalSeats must be corrected here. If no seat
	// rows were restored, rebuild the cabin layout so the flight is never left
	// claiming free seats it cannot show.
	if (seats.empty() && totalSeats > 0) {
		initializeSeats(totalSeats);
	}
	if (!seats.empty()) {
		totalSeats = static_cast<int>(seats.size());
	}
	int avail = 0;
	for (const auto& s : seats) {
		if (s && !s->getIsBooked()) ++avail;
	}
	availableSeats = avail;
}
bool Flight::hasAvailableSeats(int seatsRequested) const { return seatsRequested > 0 && availableSeats >= seatsRequested; }

Seat* Flight::findSeat(const std::string& seatNumber) {
	std::string upperCase = seatNumber;
	std::transform(upperCase.begin(), upperCase.end(), upperCase.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	for (const auto& seat : seats) {
		if (seat && seat->getSeatNumber() == upperCase) return seat.get();
	}
	return nullptr;
}

const std::vector<std::unique_ptr<Seat>>& Flight::getSeats() const { return seats; }

nlohmann::json Flight::toJson() const {
	nlohmann::json js;
	js["Id"] = id;
	js["Airline"] = airline;
	js["Origin"] = origin;
	js["Destination"] = destination;
	js["Departure Time"] = timePointToString(departureTime);
	js["Arrival Time"] = timePointToString(arrivalTime);
	js["Terminal"] = terminal;
	js["Gate"] = gate;
	js["Ticket Price"] = ticketPrice;
	js["Status"] = statusToString(status);

	nlohmann::json seatArray = nlohmann::json::array();
	for (const auto& s : seats) {
		if (!s) continue;
		seatArray.push_back(s->toJson());
	}
	js["Seats"] = std::move(seatArray);
	js["Total Seats"] = static_cast<int>(js["Seats"].size());
	js["Available Seats"] = availableSeats;
	return js;
}

Flight Flight::fromJson(const nlohmann::json& js) {
	Flight f;
	// Defensive JSON deserialization: contains() check before extraction prevents exceptions
	f.id = js.contains("Id") ? js["Id"].get<std::string>() : "";
	f.airline = js.contains("Airline") ? js["Airline"].get<std::string>() : "";
	f.origin = js.contains("Origin") ? js["Origin"].get<std::string>() : "";
	f.destination = js.contains("Destination") ? js["Destination"].get<std::string>() : "";

	// Parse departure and arrival times
	std::string deptTimeStr = js.contains("Departure Time") ? js["Departure Time"].get<std::string>() : "";
	std::string arrvTimeStr = js.contains("Arrival Time") ? js["Arrival Time"].get<std::string>() : "";

	f.departureTime = deptTimeStr.empty() ? std::chrono::system_clock::now() : parseDateTime(deptTimeStr);
	f.arrivalTime = arrvTimeStr.empty() ? (f.departureTime + std::chrono::hours(3)) : parseDateTime(arrvTimeStr);

	f.terminal = js.contains("Terminal") ? js["Terminal"].get<std::string>() : "";
	f.gate = js.contains("Gate") ? js["Gate"].get<std::string>() : "";
	f.ticketPrice = js.contains("Ticket Price") ? js["Ticket Price"].get<double>() : 0.0;
	f.status = statusFromString(js.contains("Status") ? js["Status"].get<std::string>() : "Scheduled");

	// Deserialize seat collection: each seat is polymorphic (Economy/Business/FirstClass)
	f.seats.clear();
	if (js.contains("Seats") && js["Seats"].is_array()) {
		for (const auto& s : js["Seats"]) {
			auto seat = seatFromJson(s);
			if (seat) f.seats.push_back(std::move(seat));
		}
	}

	int seatCount = static_cast<int>(f.seats.size());
	if (seatCount == 0) {
		seatCount = js.contains("Total Seats") ? js["Total Seats"].get<int>() : DefaultSeats;
		if (seatCount <= 0) seatCount = DefaultSeats;
		if (seatCount > MaxSeats) seatCount = MaxSeats;
		f.initializeSeats(seatCount);
	}
	else {
		if (seatCount > MaxSeats) {
			seatCount = MaxSeats;
			f.seats.resize(static_cast<size_t>(seatCount));
		}
	}
	f.totalSeats = seatCount;
	f.availableSeats = 0;
	for (const auto& s : f.seats) {
		if (s && !s->getIsBooked()) ++f.availableSeats;
	}

	return f;
}

bool Flight::bookSeat(const std::string& seatNumber) {
	// Check if flight has already departed
	if (isDeparted()) {
		std::cout << "Cannot book seat on a departed flight!" << std::endl;
		return false;
	}

	Seat* seat = findSeat(seatNumber);
	if (!seat) return false;
	if (!seat->book()) return false;
	if (availableSeats > 0) {
		--availableSeats;
		return true;
	}
	seat->cancel();
	return false;
}

bool Flight::cancelSeat(const std::string& seatNumber, std::chrono::system_clock::time_point bookingTime) {
	Seat* seat = findSeat(seatNumber);
	if (!seat) {
		std::cout << "Seat not found!" << std::endl;
		return false;
	}

	if (!seat->getIsBooked()) {
		std::cout << "Seat is not booked!" << std::endl;
		return false;
	}

	// Check if cancellation is within 30 minutes of booking
	auto now = std::chrono::system_clock::now();
	auto minutesSinceBooking = std::chrono::duration_cast<std::chrono::minutes>(now - bookingTime);

	if (minutesSinceBooking.count() > CancellationMinutesThreshold) {
		std::cout << "Cancellation only available within " << CancellationMinutesThreshold 
			<< " minutes of booking. You booked " << minutesSinceBooking.count() 
			<< " minutes ago." << std::endl;
		return false;
	}

	if (!seat->cancel()) return false;
	if (availableSeats < totalSeats) ++availableSeats;
	return true;
}

bool Flight::bookSeats(int seatCount) {
	if (!hasAvailableSeats(seatCount)) return false;
	std::vector<Seat*> toBook;
	toBook.reserve(static_cast<size_t>(seatCount));
	for (const auto& s : seats) {
		if (!s || s->getIsBooked()) continue;
		toBook.push_back(s.get());
		if (static_cast<int>(toBook.size()) == seatCount) break;
	}
	if (static_cast<int>(toBook.size()) != seatCount) return false;
	for (Seat* s : toBook) {
		s->book();
	}
	availableSeats -= seatCount;
	return true;
}

bool Flight::cancelSeats(int seatCount) {
	if (seatCount <= 0 || seatCount > getBookedSeats()) return false;
	int cancelled = 0;
	for (auto it = seats.rbegin(); it != seats.rend() && cancelled < seatCount; ++it) {
		if (!(*it) || !(*it)->getIsBooked()) continue;
		(*it)->cancel();
		++cancelled;
	}
	availableSeats += cancelled;
	return cancelled == seatCount;
}

void saveFlights(const std::unordered_map<std::string, Flight>& flightCategory) {
	SQLite3DB* db = SQLite3DB::getInstance();

	// Ensure database is connected
	if (!db->isConnected()) {
		db->connect("data/flight_reservation.db");
	}

	// Save to SQLite database
	if (!db || !db->isConnected()) {
		std::cerr << "Database is not connected. Flights saved to JSON only.\n";
		return;
	}

	// Clear existing flights
	db->executeUpdate("DELETE FROM flights;");
	db->executeUpdate("DELETE FROM seats;");

	// Helper lambda to escape single quotes
	auto escapeQuotes = [](std::string s) {
		size_t pos = 0;
		while ((pos = s.find('\'', pos)) != std::string::npos) {
			s.replace(pos, 1, "''");
			pos += 2;
		}
		return s;
	};

	for (const auto& [id, flight] : flightCategory) {
		(void)id;

		std::ostringstream sql;
		sql << "INSERT INTO flights (id, airline, origin, destination, departure_time, arrival_time, "
			<< "terminal, gate, total_seats, available_seats, ticket_price, status) VALUES ("
			<< "'" << escapeQuotes(flight.getId()) << "', "
			<< "'" << escapeQuotes(flight.getAirline()) << "', "
			<< "'" << escapeQuotes(flight.getOrigin()) << "', "
			<< "'" << escapeQuotes(flight.getDestination()) << "', "
			<< "'" << escapeQuotes(flight.getDepartureTime()) << "', "
			<< "'" << escapeQuotes(flight.getArrivalTime()) << "', "
			<< "'" << escapeQuotes(flight.getTerminal()) << "', "
			<< "'" << escapeQuotes(flight.getGate()) << "', "
			<< flight.getTotalSeats() << ", "
			<< flight.getAvailableSeats() << ", "
			<< flight.getTicketPrice() << ", "
			<< "'" << Flight::statusToString(flight.getStatus()) << "'" << ");";

		if (!db->executeUpdate(sql.str())) {
			std::cerr << "Failed to insert flight: " << flight.getId() << "\n";
		}

		// Insert seats
		for (const auto& seat : flight.getSeats()) {
			if (!seat) continue;

			std::string bookedTimeStr;
			if (seat->getIsBooked()) {
				bookedTimeStr = Flight::timePointToString(seat->getBookedTime());
			}

			std::ostringstream seatSQL;
			seatSQL << "INSERT INTO seats (flight_id, row_number, letter, cabin, is_booked, booked_by, booked_time) VALUES ("
				<< "'" << escapeQuotes(flight.getId()) << "', "
				<< seat->getRow() << ", "
				<< "'" << seat->getLetter() << "', "
				<< "'" << Seat::cabinToString(seat->getCabin()) << "', "
				<< (seat->getIsBooked() ? 1 : 0) << ", "
				<< "'" << escapeQuotes(seat->getBookedBy()) << "', "
				<< (bookedTimeStr.empty() ? "NULL" : ("'" + escapeQuotes(bookedTimeStr) + "'")) << ");";

			db->executeUpdate(seatSQL.str());
		}
	}
}

void loadFlights(std::unordered_map<std::string, Flight>& flightCategory) {
	SQLite3DB* db = SQLite3DB::getInstance();

	// Ensure database is connected
	if (!db->isConnected()) {
		db->connect("data/flight_reservation.db");
	}

	flightCategory.clear();

	if (db && db->isConnected()) {
		std::vector<std::vector<std::string>> flightRows = db->executeQuery(
			"SELECT id, airline, origin, destination, departure_time, arrival_time, "
			"terminal, gate, total_seats, available_seats, ticket_price, status FROM flights;"
		);

		if (!flightRows.empty()) {
			for (const auto& row : flightRows) {
				if (row.size() < 12) continue;

				// Parse times from strings
				auto departureTime = Flight::parseDateTime(row[4]);
				auto arrivalTime = Flight::parseDateTime(row[5]);

				Flight f(row[0], row[1], row[2], row[3], departureTime, arrivalTime,
						 std::stoi(row[8]), std::stod(row[10]));

				f.setTerminal(row[6]);
				f.setGate(row[7]);
				f.setStatus(Flight::statusFromString(row[11]));
				f.updateStatus();

				// Load seats for this flight
				std::string seatQuery = "SELECT row_number, letter, cabin, is_booked, booked_by, booked_time FROM seats WHERE flight_id = '" + f.getId() + "' ORDER BY row_number, letter;";
				std::vector<std::vector<std::string>> seatRows = db->executeQuery(seatQuery);

				// Clear default seats and rebuild from database
				auto& seatsRef = const_cast<std::vector<std::unique_ptr<Seat>>&>(f.getSeats());
				seatsRef.clear();

				for (const auto& seatRow : seatRows) {
					if (seatRow.size() < 5) continue;

					int row = std::stoi(seatRow[0]);
					char letter = seatRow[1][0];
					std::string cabin = seatRow[2];
					bool isBooked = std::stoi(seatRow[3]) != 0;
					std::string bookedBy = seatRow[4];
					std::string bookedTimeStr = (seatRow.size() > 5 && !seatRow[5].empty()) ? seatRow[5] : "";

					std::unique_ptr<Seat> seat;
					auto cabinType = Seat::cabinFromString(cabin);

					switch (cabinType) {
					case Seat::Cabin::FirstClass:
						seat = std::make_unique<FirstClassSeat>(row, letter);
						break;
					case Seat::Cabin::Business:
						seat = std::make_unique<BusinessSeat>(row, letter);
						break;
					default:
						seat = std::make_unique<EconomySeat>(row, letter);
						break;
					}

					if (isBooked && seat) {
						seat->book(bookedBy);
						// Restore booked time if available
						if (!bookedTimeStr.empty()) {
							auto restoredTime = Flight::parseDateTime(bookedTimeStr);
							seat->setBookedTime(restoredTime);
						}
					}
					seatsRef.push_back(std::move(seat));
				}
				// availableSeats was set to totalSeats by the constructor; now
				// that the real seats are loaded, sync it to the actual booked
				// state (and rebuild seats if none were stored for this flight).
				f.recountAvailableSeats();
				if (!f.getId().empty()) {
					flightCategory.emplace(f.getId(), std::move(f));
				}
			}
		}
	}
}

void saveFlights() {
	saveFlights(flights);
}

void loadFlights() {
	loadFlights(flights);
}