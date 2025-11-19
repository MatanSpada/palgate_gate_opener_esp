#pragma once
#include <stdint.h>
#include <string>
#include <array>

// tokenType: 0 = SMS, 1 = PRIMARY, 2 = SECONDARY
std::string generateToken(const uint8_t sessionToken[16], uint64_t phoneNumber, int tokenType, uint32_t timestampSecs = 0, int timestampOffset = 2);

// helper: parse hex string -> bytes (returns true on success)
bool hexStringToBytes(const std::string &hex, uint8_t *out, size_t outLen);
