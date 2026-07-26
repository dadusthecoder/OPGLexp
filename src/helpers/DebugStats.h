#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace lgt {

    struct DebugStatEntry {
        std::string name;
        std::string value;
    };

    class DebugStats {
    public:
        // Report a new statistic or update an existing one. Compiles out in LGT_DIST.
#ifndef LGT_DIST
        template<typename T>
        static void Report(const std::string& name, const T& value) {
            std::lock_guard<std::mutex> lock(s_Mutex);
            s_Stats[name] = std::to_string(value);
        }

        // Specialization for strings
        static void Report(const std::string& name, const std::string& value) {
            std::lock_guard<std::mutex> lock(s_Mutex);
            s_Stats[name] = value;
        }

        static void Report(const std::string& name, const char* value) {
            std::lock_guard<std::mutex> lock(s_Mutex);
            s_Stats[name] = std::string(value);
        }

        // Retrieve all current stats for rendering
        static std::vector<DebugStatEntry> GetStats() {
            std::lock_guard<std::mutex> lock(s_Mutex);
            std::vector<DebugStatEntry> entries;
            entries.reserve(s_Stats.size());
            for (const auto& [name, value] : s_Stats) {
                entries.push_back({name, value});
            }
            return entries;
        }
#else
        template<typename T>
        static void Report(const std::string& name, const T& value) {}
        static void Report(const std::string& name, const std::string& value) {}
        static void Report(const std::string& name, const char* value) {}
        
        static std::vector<DebugStatEntry> GetStats() { return {}; }
#endif

    private:
        static std::unordered_map<std::string, std::string> s_Stats;
        static std::mutex s_Mutex;
    };

}
