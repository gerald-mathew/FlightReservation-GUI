#pragma once

#include <string>
#include "json.hpp"

struct PaystackInitResult {
	bool ok{};

	std::string reference{};
	std::string authorization_url{};
	std::string message{};
	double amount{}; // in kobo
};

struct PaystackVerifyResult {
	bool ok{};
	bool isSuccess{}; // true if status == "success"
	std::string status{};
	std::string message{};
	nlohmann::json data{}; // full transaction data
	double amount{}; // amount in kobo from Paystack
};

double koboToNaira(long long kobo);

long long nairaTOKobo(double naira);

// Paystack Initialize - Creates a payment transaction
PaystackInitResult paystackInitialize(const std::string& email, double nairaAmount, const std::string& metadataJson = "{}");

PaystackVerifyResult paystackVerify(const std::string& reference);

void displayPaystackAuthorizationUrl(const std::string& url);
