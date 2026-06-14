#include <curl/curl.h>
#ifdef _WIN32
	#include <windows.h>
	#include <shellapi.h>
#endif

#include "payment.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <cmath>
#include <cstdint>

std::string loadSecretKey();
std::size_t WriteCallback(void* contents, std::size_t size, std::size_t nmemb, std::string* userp);
nlohmann::json makePaystackPostRequest(const std::string& endpoint, const nlohmann::json& payload);
nlohmann::json makePaystackGetRequest(const std::string& endpoint);

std::string loadSecretKey() {
	const std::array<std::string, 4> candidates{
		"config/paystack_secret.txt",        // when running from repo root
		"../config/paystack_secret.txt",     // when running from build/ or src/
		"../../config/paystack_secret.txt",  // when running from build/bin-like subfolders
		"./config/paystack_secret.txt"       // explicit current dir
	};
	for (const auto& path : candidates) {
		std::ifstream file(path);
		if (!file.is_open()) continue;
		std::string key;
		std::getline(file, key);
		if (!key.empty()) return key;
	}
	throw std::runtime_error(
		"Could not read Paystack secret key. Create 'config/paystack_secret.txt' in the project root and put your key on the first line.");
}

const std::string PAYSTACK_SECRET_KEY = loadSecretKey();
constexpr std::string_view PAYSTACK_BASE_URL = "https://api.paystack.co";

std::size_t WriteCallback(void* contents, std::size_t size, std::size_t nmemb, std::string* userp) {
	std::size_t realsize = size * nmemb;
	userp->append((char*)contents, realsize);
	return realsize;
}

double koboToNaira(long long kobo) {
	return static_cast<double>(kobo) / 100.0;
}

long long nairaTOKobo(double naira) {
	return static_cast<long long>(std::llround(naira * 100.0));
}

nlohmann::json makePaystackPostRequest(const std::string& endpoint, const nlohmann::json& payload) {
	// CURL POST wrapper: performs HTTP POST with Bearer token authentication
	// Returns parsed JSON response or error object
	CURL* curl = curl_easy_init();
	nlohmann::json result;
	if (!curl) {
		result["error"] = "Failed to initialize curl";
		return result;
	}
	std::string url = std::string(PAYSTACK_BASE_URL) + endpoint;
	std::string readBuffer;
	std::string jsonStr = payload.dump();
	// Set up the request headers and authentication
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	std::string authHeader = "Authorization: Bearer " + std::string(PAYSTACK_SECRET_KEY);
	headers = curl_slist_append(headers, authHeader.c_str());
	// Configure CURL options for SSL and timeout
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		result["error"] = "Curl request failed: " + std::string(curl_easy_strerror(res));
	}
	else {
		try {
			result = nlohmann::json::parse(readBuffer);
		}
		catch (const nlohmann::json::parse_error& e) {
			result["error"] = "Failed to parse JSON response: " + std::string(e.what());
		}
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return result;
}

nlohmann::json makePaystackGetRequest(const std::string& endpoint) {
	// CURL GET wrapper: performs HTTP GET with Bearer token authentication
	// Returns parsed JSON response or error object
	CURL* curl = curl_easy_init();
	nlohmann::json result;
	if (!curl) {
		result["error"] = "Failed to initialize curl";
		return result;
	}
	std::string url = std::string(PAYSTACK_BASE_URL) + endpoint;
	std::string readBuffer;
	// Set up the request headers and authentication
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	std::string authHeader = "Authorization: Bearer " + std::string(PAYSTACK_SECRET_KEY);
	headers = curl_slist_append(headers, authHeader.c_str());
	// Configure CURL options for SSL and timeout
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		result["error"] = "Curl request failed: " + std::string(curl_easy_strerror(res));
	}
	else {
		try {
			result = nlohmann::json::parse(readBuffer);
		}
		catch (const nlohmann::json::parse_error& e) {
			result["error"] = "Failed to parse JSON response: " + std::string(e.what());
		}
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return result;
}

PaystackInitResult paystackInitialize(const std::string& email, double nairaAmount, const std::string& metadataJson) {
	PaystackInitResult res{};
	long long koboAmount = nairaTOKobo(nairaAmount);

	// Prepare the payload
	nlohmann::json payload;
	payload["email"] = email;
	payload["amount"] = koboAmount;

	try {
		nlohmann::json metadata = nlohmann::json::parse(metadataJson);
		payload["metadata"] = metadata;
	}
	catch (...) {
		payload["metadata"] = nlohmann::json::object();
	}

	// Make the request
	nlohmann::json response = makePaystackPostRequest("/transaction/initialize", payload);

	// Check for errors in the response
	if (response.contains("error")) {
		res.ok = false;
		res.message = response["error"].get<std::string>();
		return res;
	}

	// Check if Paystack returned status: true

	if (!response.contains("status") || !response["status"].get<bool>()) {
		res.ok = false;
		res.message = response.contains("message") ? response["message"].get<std::string>() : "Unknown error from Paystack";
		return res;
	}

	// Extract the data
	nlohmann::json data = response.contains("data") ? response["data"] : nlohmann::json::object();
	res.ok = true;
	res.authorization_url = data.contains("authorization_url") ? data["authorization_url"] : "";
	res.reference = data.contains("reference") ? data["reference"] : "";
	res.amount = static_cast<double>(koboAmount);
	res.message = "Transaction initialized successfully";

	return res;
}

PaystackVerifyResult paystackVerify(const std::string& reference) {
	PaystackVerifyResult res{};

	std::string endpoint = "/transaction/verify/" + reference;
	nlohmann::json response = makePaystackGetRequest(endpoint);

	// Check for errors in the response
	if (response.contains("error")) {
		res.ok = false;
		res.message = response["error"].get<std::string>();
		return res;
	}

	// Check if Paystack returned status: true
	if (!response.contains("status") || !response["status"].get<bool>()) {
		res.ok = false;
		res.message = response.contains("message") ? response["message"].get<std::string>() : "Unknown error from Paystack";
		return res;
	}

	// Extract the data
	nlohmann::json data = response.contains("data") ? response["data"] : nlohmann::json::object();
	res.ok = true;
	res.data = data;
	res.status = data.contains("status") ? data["status"].get<std::string>() : "";
	res.isSuccess = (res.status == "success");
	// Safely extract amount as long long from JSON to preserve precision
	long long amountInKobo = data.contains("amount") ? data["amount"].get<long long>() : 0LL;
	res.amount = static_cast<double>(amountInKobo);
	res.message = data.contains("gateway_response") ? data["gateway_response"].get<std::string>() : "Transaction verified";

	return res;
}

namespace {
	bool openUrlInDefaultBrowser(const std::string& url) {
#ifdef _WIN32
		HINSTANCE result = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		return reinterpret_cast<intptr_t>(result) > 32;
#elif defined(__APPLE__)
		std::string command = "open \"" + url + "\"";
		return system(command.c_str()) == 0;
#elif defined(__linux__)
		std::string command = "xdg-open \"" + url + "\" &";
		return system(command.c_str()) == 0;
#else
		(void)url;
		return false;
#endif
	}
}

void displayPaystackAuthorizationUrl(const std::string& url) {
	std::cout << "\n" << std::string(60, '=') << std::endl;
	std::cout << "Complete Your Payment Authorization" << std::endl;
	std::cout << std::string(60, '=') << std::endl;
	if (openUrlInDefaultBrowser(url)) {
		std::cout << "\nOpening the payment link in your browser..." << std::endl;
	}
	else {
		std::cout << "\nPlease open this URL in your browser: " << url << std::endl;
	}
	std::cout << "\nAfter completing the payment, return to this application." << std::endl;
	std::cout << std::string(60, '=') << "\n" << std::endl;
}