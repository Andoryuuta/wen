#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>

using namespace std::string_view_literals;

static const std::wstring_view LOGGER_PREFIX = L"[WenModLoader]: "sv;

// Injects a DLL into the target process.
void inject_dll(wil::unique_process_information& proccess,
                std::wstring_view dll_path) {
    // Allocate memory for the DLL path in the target process.
    // We explicitly do not use WIL RAII utilities here, as we
    // don't want to free this memory in the target process.
    void* dll_path_address =
        ::VirtualAllocEx(proccess.hProcess, 0, dll_path.size(),
                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    THROW_LAST_ERROR_IF_NULL(dll_path_address);

    // Write DLL path to target process memory.
    auto write_dll_path_result =
        ::WriteProcessMemory(proccess.hProcess, dll_path_address,
                             dll_path.data(), dll_path.size() * 2, nullptr);
    THROW_IF_WIN32_BOOL_FALSE(write_dll_path_result);

    // Find (system-wide) shared address of LoadLibraryW.
    // This is consistent as kernel32.dll is _ALWAYS_ loaded at the same address
    // across processes in Windows.

    auto kernel32_handle = ::GetModuleHandleW(L"kernel32.dll");
    THROW_LAST_ERROR_IF_NULL_MSG(kernel32_handle,
                                 "Failed to get kernel32.dll handle");

    auto pfn_load_library_w = ::GetProcAddress(kernel32_handle, "LoadLibraryW");
    THROW_LAST_ERROR_IF_NULL_MSG(
        pfn_load_library_w, "Failed to get kernel32.dll!LoadLibraryW address");

    // Spawn a thread at the LoadLibraryW thread in the remote process,
    // providing the DLL path as an argument.
    DWORD thread_id = 0;
    wil::unique_handle remote_thread_handle{::CreateRemoteThread(
        proccess.hProcess, NULL, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(pfn_load_library_w),
        dll_path_address, 0, &thread_id)};

    THROW_LAST_ERROR_IF_NULL_MSG(
        remote_thread_handle,
        "Failed to get kernel32.dll!LoadLibraryW address");

    // Wait until the thread finishes (DllMain should finish fast).
    auto wait_state =
        ::WaitForSingleObject(remote_thread_handle.get(), 60 * 1000);
    THROW_LAST_ERROR_IF(wait_state == WAIT_FAILED);

    if (wait_state == WAIT_TIMEOUT) {
        throw new std::runtime_error("Remote DllMain timeout!");
    }
}

// Starts the game client and injects the provided DLL before anything else
// loads/runs in the client code.
void start_game_with_dll(std::wstring_view executable_path,
                         std::wstring_view dll_path) {
    // Forward the current process startup info to the real game client.
    // This allows us to properly keep the desktop, window size/pos, etc
    // that Steam set when launching the game.
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    GetStartupInfoW(&si);

    wil::unique_process_information process;

    // Create the process in a suspended state.
    THROW_IF_WIN32_BOOL_FALSE_MSG(
        CreateProcessW(executable_path.data(), NULL, NULL, NULL, true,
                       CREATE_SUSPENDED, NULL, NULL, &si, &process),
        "Failed to create process %s in a suspended state!", executable_path);

    // Inject DLL and start thread....
    // ...
    inject_dll(process, dll_path);

    // Resume process.
    THROW_LAST_ERROR_IF(ResumeThread(process.hThread) == -1);
}

int main(int argc, char* argv[]) {
    std::wcout << LOGGER_PREFIX << "Loader started\n";

    try {
        start_game_with_dll(L"MonsterHunterWorld.exe", L"WMLCore.dll");
    } catch (const std::exception& e) {
        std::wcerr << LOGGER_PREFIX << e.what() << '\n';
    }

    std::wcout << LOGGER_PREFIX << "Injection done!\n";

    std::getchar();

    return 0;
}