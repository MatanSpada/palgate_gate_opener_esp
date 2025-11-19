#pragma once

// ============================================================
// PalGate Credentials Configuration File (Template)
// ------------------------------------------------------------
// Rename this file to config.h and replace the values below 
// with your phone number, session token, and token type obtained 
// during the QR-code device-linking process.
//
// If you are unsure how to generate these values, refer to the 
// EXTRACT_SESSION_TOKEN guide in the documentation directory.
// ============================================================

#include <stdint.h>

// IMPORTANT:
// The phone number MUST be a full international number WITHOUT "+".
// Use ULL suffix to ensure it's treated as uint64_t.

// Example:
// uint64_t phone = 972501234567ULL;

static const uint64_t PALGATE_PHONE_NUMBER = 000000000000ULL; // <-- REPLACE HERE

// The session token is HEX, so keep it as a C-string.
static const char* PALGATE_SESSION_TOKEN = "REPLACE_ME";

// Token type returned by pylgate:
// 1 = Primary
// 2 = Secondary
static const int PALGATE_TOKEN_TYPE = 1; // <-- REPLACE ME (1 or 2)


