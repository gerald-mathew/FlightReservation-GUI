#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include "seats.hpp"
#include "json.hpp"

inline const std::unordered_map<std::string, std::string> airports = {
	// Airport codes and their corresponding names
	{"LOS", "Murtala Muhammed International Airport, Lagos"},
	{"ABV", "Nnamdi Azikiwe International Airport, Abuja"},
	{"PHC", "Port Harcourt International Airport"},
	{"KAN", "Mallam Aminu Kano International Airport"},
	{"ENU", "Akanu Ibiam International Airport, Enugu"},
	{"ILR", "Ilorin International Airport"},
	{"JOS", "Yakubu Gowon Airport, Jos"},
	{"CBQ", "Margaret Ekpo International Airport, Calabar"},
	{"BEN", "Benin Airport"},
	{"QOW", "Sam Mbakwe Airport, Owerri"},
	{"AKR", "Akure Airport"},
	{"YOL", "Yola Airport"},
	{"IBA", "Ibadan Airport"},
	{"MDI", "Makurdi Airport"},
	{"SKO", "Sadiq Abubakar III International Airport, Sokoto"},
	{"MIU", "Maiduguri International Airport"},
	{"ZAR", "Zaria Airport"},
	{"GMO", "Gombe Lawanti International Airport"},
	{"DUT", "Dutse International Airport"},
	{"KAD", "Kaduna Airport"},
	{"ABB", "Asaba International Airport"}
};

class Flight {
public:
	enum class Status {
		Scheduled,
		Boarding,
		Departed,
		Delayed,
		Cancelled
	};

	static constexpr int DefaultSeats = 200;
	static constexpr int MaxSeats = 500;
	static constexpr int CancellationMinutesThreshold = 30;

private:
	std::string id{};
	std::string airline{};
	std::string origin{};
	std::string destination{};
	std::chrono::system_clock::time_point departureTime{};
	std::chrono::system_clock::time_point arrivalTime{};
	std::string terminal{};
	std::string gate{};
	int totalSeats{};
	int availableSeats{};
	double ticketPrice{};
	Status status{ Status::Scheduled };
	std::vector<std::unique_ptr<Seat>> seats{};

	static std::unique_ptr<Seat> makeSeatForRow(int row, char letter);
	void initializeSeats(int seatCount);
	static std::unique_ptr<Seat> seatFromJson(const nlohmann::json& js);
	static bool isValidAirportCode(const std::string& code);
	static bool isValidDateTimeFormat(const std::string& dateTime);

public:
	static std::chrono::system_clock::time_point parseDateTime(const std::string& dateTime);
	static std::string timePointToString(const std::chrono::system_clock::time_point& tp);
	static std::string statusToString(Status s);
	static Status statusFromString(std::string v);
	Flight();

	Flight(const Flight& other);
	Flight& operator=(const Flight& other);

	Flight(std::string aId, std::string anAirline, std::string anOrigin, std::string aDestination,
		   std::chrono::system_clock::time_point aDepartureTime, std::chrono::system_clock::time_point anArrivalTime,
		   int aTotalSeats, double aTicketPrice);

	// Setters
	bool setId(std::string aId);
	bool setAirline(std::string anAirline);
	bool setOrigin(std::string anOrigin);
	bool setDestination(std::string aDestination);
	bool setDepartureTime(std::chrono::system_clock::time_point aDepartureTime);
	bool setArrivalTime(std::chrono::system_clock::time_point anArrivalTime);
	bool setTotalSeats(int aTotalSeats);
	bool setTicketPrice(double aTicketPrice);

	void setTerminal(std::string aTerminal);
	void setGate(std::string aGate);
	void setStatus(Status aStatus);

	// Overloaded setters for string input
	bool setDepartureTimeFromString(const std::string& dateTime);
	bool setArrivalTimeFromString(const std::string& dateTime);

	// Getters
	std::string getId() const;
	std::string getAirline() const;
	std::string getOrigin() const;
	std::string getDestination() const;
	std::string getDepartureTime() const;
	std::string getArrivalTime() const;
	std::chrono::system_clock::time_point getDepartureTimePoint() const;
	std::chrono::system_clock::time_point getArrivalTimePoint() const;
	std::string getTerminal() const;
	std::string getGate() const;
	int getTotalSeats() const;
	int getAvailableSeats() const;
	double getTicketPrice() const;
	Status getStatus() const;

	// Time-based methods
	bool isDeparted() const;
	bool isBoarding() const;
	int getMinutesUntilDeparture() const;
	void updateStatus(); // Auto-update status based on current time

	int getBookedSeats() const;
	void recountAvailableSeats(); // Re-derive available/total seats from actual seat state
	bool hasAvailableSeats(int seatsRequested = 1) const;
	Seat* findSeat(const std::string& seatNumber);
	const std::vector<std::unique_ptr<Seat>>& getSeats() const;
	nlohmann::json toJson() const;
	static Flight fromJson(const nlohmann::json& js);
	bool bookSeat(const std::string& seatNumber);
	bool cancelSeat(const std::string& seatNumber, std::chrono::system_clock::time_point bookingTime);
	bool bookSeats(int seatCount = 1);
	bool cancelSeats(int seatCount = 1);
};

inline std::unordered_map<std::string, Flight> flights;

void saveFlights(const std::unordered_map<std::string, Flight>& flightCategory);
void loadFlights(std::unordered_map<std::string, Flight>& flightCategory);
void saveFlights();
void loadFlights();
