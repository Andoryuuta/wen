#include "log.h"
#include <Windows.h>
#include <cstdio>
#include <mutex>

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

using proxy_dist_sink_mt = spdlog::sinks::dist_sink<std::mutex>;

std::mutex g_prelogger_initalized_mutex;
bool g_prelogger_initalized;
std::mutex g_logger_initalized_mutex;
bool g_logger_initalized;
std::shared_ptr<proxy_dist_sink_mt> console_proxy = nullptr;
std::shared_ptr<proxy_dist_sink_mt> file_proxy = nullptr;

const static char* PRELOADER_LOG_PATH = "wml/logs/PreLoaderLog.txt";
const static char* LOADER_LOG_PATH = "wml/logs/LoaderLog.txt";

// Open new console for c(in/out/err)
void Log::OpenConsole() {
    AllocConsole();
    FILE* cinStream;
    FILE* coutStream;
    FILE* cerrStream;
    freopen_s(&cinStream, "CONIN$", "r", stdin);
    freopen_s(&coutStream, "CONOUT$", "w", stdout);
    freopen_s(&cerrStream, "CONOUT$", "w", stderr);

    // From: https://stackoverflow.com/a/45622802 to deal with UTF8 CP:
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);
}

int Log::InitializePreLogger() {
    std::lock_guard<std::mutex> guard(g_prelogger_initalized_mutex);
    if (g_prelogger_initalized == true)
        return 1;

    // Setup async logger.
    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto rotating_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        PRELOADER_LOG_PATH, true);
    std::vector<spdlog::sink_ptr> sinks{stdout_sink, rotating_sink};
    auto logger = std::make_shared<spdlog::async_logger>(
        "PreLoader", sinks.begin(), sinks.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    logger->flush_on(spdlog::level::debug);
    spdlog::register_logger(logger);

    g_prelogger_initalized = true;
    return 0;
}

int Log::InitializeLogger() {
    std::lock_guard<std::mutex> guard(g_logger_initalized_mutex);
    if (g_logger_initalized == true)
        return 1;

    // Setup async logger.
    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        LOADER_LOG_PATH, true);

    console_proxy = std::make_shared<proxy_dist_sink_mt>();
    console_proxy->add_sink(stdout_sink);

    file_proxy = std::make_shared<proxy_dist_sink_mt>();
    file_proxy->add_sink(file_sink);

    spdlog::sinks_init_list loaderLoggerSinkList = {console_proxy, file_proxy};
    auto loaderLogger = std::make_shared<spdlog::async_logger>(
        "Loader", loaderLoggerSinkList.begin(), loaderLoggerSinkList.end(),
        spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    loaderLogger->flush_on(spdlog::level::debug);
    spdlog::register_logger(loaderLogger);

    g_logger_initalized = true;
    return 0;
}