cmake_minimum_required (VERSION 3.8)

project(Wen VERSION "4.0.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(extern)

# # Old/unused exe proxy loader
# # This was scrapped due to steams [re]loading mechanisms
# add_executable(loader_exec
#     "src/loader_exec/main.cpp"
# )
# target_compile_features(loader_exec PUBLIC cxx_std_20)
# target_include_directories(loader_exec PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern/wil/include)

# DLL proxy loader (winmm.dll stub)
add_library(proxy_loader SHARED
    "src/proxy_loader/proxy_winmm.cpp"
)
target_compile_features(proxy_loader PUBLIC cxx_std_20)
target_include_directories(proxy_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern/wil/include) 

# Core library
add_library(core SHARED
	"src/wml/entry.cc"
	"src/wml/log.h"
	"src/wml/log.cc"
)
target_compile_features(core PUBLIC cxx_std_20)
target_include_directories (core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories (core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern)
target_link_libraries(core PUBLIC spdlog::spdlog)
target_link_libraries(core PUBLIC minhook)

# (OPTIONALLY) copy output directly to game folder for rapid testing/development.
option(RAPID_DEV_INSTALL "Automatically copy the build target(s) to the MHW installation folder" ON)
if(RAPID_DEV_INSTALL)
    # Get game directory via Steam install registry key.
    get_filename_component(STEAM_INSTALL_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam;InstallPath]" ABSOLUTE CACHE)
    set(STEAM_INSTALL_PATH "${STEAM_INSTALL_PATH}/steamapps/common/Monster Hunter World")
    MESSAGE("STEAM_INSTALL_PATH = ${STEAM_INSTALL_PATH}")

    # Kill game/WML processes which might be holding handles to the installed build artifacts.
    execute_process(COMMAND "cmd /c taskkill -f -im WMLLoader.exe || 1" OUTPUT_QUIET)
    execute_process(COMMAND "cmd /c taskkill -f -im MonsterHunterWorld.exe || 1" OUTPUT_QUIET)

    # Copy target output
    # add_custom_command(TARGET loader_exec POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:loader_exec> ${STEAM_INSTALL_PATH}/WMLLoader.exe
    #     COMMENT "Created ${STEAM_INSTALL_PATH}/WMLLoader.exe"
    # )

    add_custom_command(TARGET proxy_loader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:proxy_loader> ${STEAM_INSTALL_PATH}/winmm.dll
        COMMENT "Created ${STEAM_INSTALL_PATH}/winmm.dll"
    )
    

    add_custom_command(TARGET core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:core> ${STEAM_INSTALL_PATH}/wml/WMLCore.dll
        COMMENT "Created ${STEAM_INSTALL_PATH}/wml/WMLCore.dll"
    )


	# target_compile_definitions(LoaderCore PRIVATE LOADER_NEXUS_CHECK=LOADER_NEXUS_CHECK)
endif()




# add_subdirectory(extern)

# option(LOADER_NEXUS_CHECK "A check only for the nexus version to future-proof against collection misuse" OFF)

# add_library(dinput8 SHARED
# 	"src/dinput_proxy/dinput8_proxy.cpp"
# )

# add_library(LoaderCore SHARED
# 	"src/loader/entry.cpp"
# 	"src/loader/SigScan.cpp"
# 	"src/loader/SigScan.hpp"
# 	"src/loader/Log.hpp"
# 	"src/loader/Log.cpp"
# 	"src/loader/PluginManager.cpp"
# 	"src/loader/PluginManager.hpp"
# 	"src/loader/Plugin.cpp"
# 	"src/loader/Plugin.hpp"
# 	"src/loader/PluginConfig.hpp"
# 	"src/loader/PluginConfig.cpp"
# 	#"src/loader/Win32Internals.hpp"
# 	#"src/loader/Win32Internals.cpp"
# )
# target_include_directories (LoaderCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# target_include_directories (LoaderCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern)
# target_link_libraries(LoaderCore PUBLIC spdlog::spdlog)
# target_link_libraries(LoaderCore PUBLIC minhook)

# if(LOADER_NEXUS_CHECK)
# 	message("Nexus check enabled")
# 	target_compile_definitions(LoaderCore PRIVATE LOADER_NEXUS_CHECK=LOADER_NEXUS_CHECK)
# endif()