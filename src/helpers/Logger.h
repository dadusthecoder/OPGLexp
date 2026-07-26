#pragma once
#ifdef _WIN32
#undef APIENTRY
#endif
#include <spdlog/async.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

struct ConsoleLogMessage {
    spdlog::level::level_enum level;
    std::string text;
};

class ImGuiConsoleSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    ImGuiConsoleSink() = default;

    void clear() {
        std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
        messages.clear();
    }

    std::vector<ConsoleLogMessage> get_messages() {
        std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
        return messages;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);
        std::string text = fmt::to_string(formatted);
        
        messages.push_back({msg.level, std::move(text)});
        
        if (messages.size() > 1000) {
            messages.erase(messages.begin(), messages.begin() + 500);
        }
    }

    void flush_() override {}

private:
    std::vector<ConsoleLogMessage> messages;
};

class Log {
public:
    static void                             Init();
    static void                             Shutdown();
    static std::shared_ptr<spdlog::logger>& Core();
    static void                             LogStatus(const std::string& msg);
    static std::shared_ptr<ImGuiConsoleSink> GetConsoleSink();

private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<ImGuiConsoleSink> s_ConsoleSink;
};

#ifndef LGT_DIST
    #define CORE_LOG_INIT()         Log::Init();
    #define CORE_TRACE(msg, ...)    Log::Core()->trace("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
    #define CORE_INFO(msg, ...)     Log::Core()->info("[{}] " msg, __func__, ##__VA_ARGS__)
    #define CORE_WARN(msg, ...)     Log::Core()->warn("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
    #define CORE_ERROR(msg, ...)    Log::Core()->error("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
    #define CORE_CRITICAL(msg, ...) Log::Core()->critical("[{}:{}] " msg, __func__, __LINE__, ##__VA_ARGS__)
    #define ENABLE_ASSERTIONS 1
#else
    #define CORE_LOG_INIT()
    #define CORE_TRACE(msg, ...)
    #define CORE_INFO(msg, ...)
    #define CORE_WARN(msg, ...)
    #define CORE_ERROR(msg, ...)
    #define CORE_CRITICAL(msg, ...)
    // Disable assertions in dist build
    #undef ENABLE_ASSERTIONS
#endif

#if defined(__clang__) || defined(__GNUC__)
#define DEBUGBREAK() __builtin_trap()
#elif defined(_MSC_VER)
#define DEBUGBREAK() __debugbreak()
#else
#include <cstdlib>
#define DEBUGBREAK() std::abort()
#endif

#ifdef ENABLE_ASSERTIONS
#define ASSERT(expr)                                                                                                             \
    {                                                                                                                            \
        if (!(expr)) {                                                                                                           \
            spdlog::error("Assertion Failed! Expr: {}", #expr);                                                                  \
            DEBUGBREAK();                                                                                                        \
        }                                                                                                                        \
    }

#define ASSERT_MSG(expr, msg, ...)                                                                                               \
    {                                                                                                                            \
        if (!(expr)) {                                                                                                           \
            spdlog::error("Assertion Failed: {} | Expr: "                                                                        \
                          "{}",                                                                                                  \
                          fmt::format(msg, ##__VA_ARGS__),                                                                       \
                          #expr);                                                                                                \
            DEBUGBREAK();                                                                                                        \
        }                                                                                                                        \
    }
#else
#define ASSERT(expr) (void)0
#define ASSERT_MSG(expr, msg, ...) (void)0
#endif
