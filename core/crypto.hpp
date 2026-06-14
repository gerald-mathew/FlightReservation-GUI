#pragma once

#include <string>
#include <cstddef>
#include "json.hpp"

// Argon2 password hashing utility
namespace Crypto {
	constexpr size_t HASH_LENGTH = 128;
	constexpr size_t SALT_LENGTH = 16;
	constexpr int ARGON2_TIME_COST = 2;
	constexpr int ARGON2_MEMORY_COST = 19;  // 2^19 KiB = ~512 MiB
	constexpr int ARGON2_PARALLELISM = 1;

	std::string generateSalt(); // Generate a random salt for password hashing
	nlohmann::json hashPassword(const std::string& password); // Returns JSON object with hash and salt
	bool verifyPassword(const std::string& password, const std::string& storedHash, const std::string& salt); // Verify a password against a stored Argon2 hash
}
