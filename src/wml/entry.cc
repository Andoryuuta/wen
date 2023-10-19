#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>

#include <Windows.h>
#include <intrin.h>
#include <psapi.h>

#include "MinHook.h"
#include "spdlog/async.h"
#include "spdlog/spdlog.h"

#include "game_address.h"
#include "log.h"

#pragma intrinsic(_ReturnAddress)

#define VERSION_MESSAGE "WenLoader v1.0.0"

typedef void (*GetSystemTimeAsFileTime_t)(LPFILETIME lpSystemTimeAsFileTime);
GetSystemTimeAsFileTime_t fpGetSystemTimeAsFileTime = NULL;

typedef int (*main_t)(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nShowCmd);
main_t fpWinMain = NULL;

typedef int64_t (*SCRTCommonMain_t)();
SCRTCommonMain_t fpSCRTCommonMain = NULL;

// This is hooks the __scrt_common_main_seh MSVC function.
// This runs before all of the CRT initalization, static
// initalizers, and before WinMain.
int64_t hookedSCRTCommonMain() {
    // Initialize the console and logger.
    Log::OpenConsole();
    Log::InitializeLogger();
    auto logger = spdlog::get("Loader");
    logger->info(VERSION_MESSAGE);
    logger->info("Loading mods from main thread (pre-main)");

    return fpSCRTCommonMain();
}

int __stdcall hookedWinMain(HINSTANCE hInstance,
                            HINSTANCE hPrevInstance,
                            LPSTR lpCmdLine,
                            int nShowCmd) {
    auto logger = spdlog::get("Loader");
    logger->info("Loading mods from main thread (WinMain)");
    return fpWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

int EnableCoreHooks() {
    auto logger = spdlog::get("PreLoader");

    if (MH_CreateHook(
            reinterpret_cast<void*>(WML::Game::Address::SCRT_COMMON_MAIN_SEH),
            reinterpret_cast<void*>(hookedSCRTCommonMain),
            reinterpret_cast<void**>(&fpSCRTCommonMain)) != MH_OK) {
        logger->error("Failed to create SCRTCommonMain hook");
        return 1;
    }
    if (MH_EnableHook(reinterpret_cast<void*>(
            WML::Game::Address::SCRT_COMMON_MAIN_SEH)) != MH_OK) {
        logger->error("Failed to enable SCRTCommonMain hook");
        return 1;
    }

    if (MH_CreateHook(reinterpret_cast<void*>(WML::Game::Address::WINMAIN),
                      reinterpret_cast<void*>(&hookedWinMain),
                      reinterpret_cast<void**>(&fpWinMain)) != MH_OK) {
        logger->error("Failed to create main hook");
        return 1;
    }

    if (MH_EnableHook(reinterpret_cast<void*>(WML::Game::Address::WINMAIN)) !=
        MH_OK) {
        logger->error("Failed to enable main hook");
        return 1;
    }

    return 0;
}

// The hooked GetSystemTimeAsFileTime function.
// This function is called in many places, one of them being the
// `__security_init_cookie` function that is used to setup the security token(s)
// before SCRT_COMMON_MAIN_SEH is called.
void hookedGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime) {
    auto logger = spdlog::get("PreLoader");
    uint64_t ret_address = (uint64_t)_ReturnAddress();

    if (ret_address == WML::Game::Address::SECURITY_COOKIE_INIT_GETTIME_RET) {
        // Enable our hooks and, if successful, disable the
        // GetSystemTimeAsFileTime as we will not need it anymore.
        if (EnableCoreHooks()) {
            logger->error("Failed to enable core hooks!");
            goto EXIT_HOOK;
        } else {
            logger->info("Enabled core hooks");

            if (MH_DisableHook(reinterpret_cast<void*>(
                    &GetSystemTimeAsFileTime)) != MH_OK) {
                logger->error("Failed to disable GetSystemTimeAsFileTime hook");
                goto EXIT_HOOK;
            }
            logger->info("Disabled GetSystemTimeAsFileTime hook");
        }
    }

EXIT_HOOK:
    fpGetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}

// This function is called from the loader-locked DllMain,
// does the bare-minimum to get control flow in the main thread
// by hooking a function called in the CRT startup (GetSystemTimeAsFileTime).
//
// This allows us to work with both the SteamDRM and Steamless unpacked
// binaries by detecting the first call to the hooked function _after_ the
// executable is unpacked in memory.
void LoaderLockedInitialize() {
    Log::InitializePreLogger();
    auto logger = spdlog::get("PreLoader");

    logger->info("Begin enabling post-unpack hooks on thread: {0}",
                 GetCurrentThreadId());

    // Override the process security token that that the singleton instanciation
    // happens, causing GetSystemTimeAsFileTime to be called pre-CRT init.
    *(uint64_t*)WML::Game::Address::PROCESS_SECURITY_COOKIE = 0x2B992DDFA232L;

    if (MH_Initialize() != MH_OK) {
        logger->error("Failed to initialize minhook!");
        return;
    }

    // Hook GetSystemTimeAsFileTime in order to get control of execution after
    // the .exe is unpacked in memory.
    if (MH_CreateHook(reinterpret_cast<void*>(&GetSystemTimeAsFileTime),
                      reinterpret_cast<void*>(&hookedGetSystemTimeAsFileTime),
                      reinterpret_cast<void**>(&fpGetSystemTimeAsFileTime)) !=
        MH_OK) {
        logger->error("Failed to create GetSystemTimeAsFileTime hook");
        return;
    }
    if (MH_EnableHook(reinterpret_cast<void*>(&GetSystemTimeAsFileTime)) !=
        MH_OK) {
        logger->error("Failed to enable GetSystemTimeAsFileTime hook");
        return;
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        MessageBoxA(NULL, "WenLoader", "WenLoader", MB_OK);
        LoaderLockedInitialize();
    }

    return TRUE;
}