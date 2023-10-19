#pragma once
#include <cstdint>

// As this game updates so infrequently (e.g. 2 year gap between 15.11.01 -> 15.20.00),
// we hard-code the addresses in this file instead of attempting to dynamically locate
// them at runtime based on AOBs or other patterns.

namespace WML::Game::Address {
// MHW 15.20.00:
const uint64_t IMAGE_BASE = 0x140000000;
const uint64_t PROCESS_OEP = IMAGE_BASE + 0x2741668;
const uint64_t PROCESS_SECURITY_COOKIE = IMAGE_BASE + 0x4bf4be8;
const uint64_t SECURITY_COOKIE_INIT_GETTIME_RET = IMAGE_BASE + 0x27422e2;
const uint64_t SCRT_COMMON_MAIN_SEH = IMAGE_BASE + 0x27414f4;
const uint64_t WINMAIN = IMAGE_BASE + 0x13a4c00;

}  // namespace WML::Game::Address