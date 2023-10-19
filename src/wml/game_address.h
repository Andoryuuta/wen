#pragma once
#include <cstdint>

// All of the addresses in this file are hard-coded to 15.11.01 version of the
// game client The game hasn't been updated in 2+ years at the time of writing
// this, so there doesn't seem to be any benefit to trying to find the addresses
// dynamically.

namespace WML::Game::Address {
const uint64_t IMAGE_BASE = 0x140000000;
const uint64_t PROCESS_OEP = IMAGE_BASE + 0x279a918;
const uint64_t PROCESS_SECURITY_COOKIE = IMAGE_BASE + 0x4c56b88;
const uint64_t SECURITY_COOKIE_INIT_GETTIME_RET = IMAGE_BASE + 0x279b592;
const uint64_t SCRT_COMMON_MAIN_SEH = IMAGE_BASE + 0x279a7a4;
const uint64_t WINMAIN = IMAGE_BASE + 0x13b5110;
}  // namespace WML::Game::Address