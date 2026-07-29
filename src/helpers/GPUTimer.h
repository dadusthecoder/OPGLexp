#pragma once
#include <string>
#include <unordered_map>

namespace lgt {

    class ScopedGPUTimer {
    public:
        ScopedGPUTimer(const std::string& name);
        ~ScopedGPUTimer();

        static double GetTimeMS(const std::string& name);
        static void RetrieveResults();
        static const std::unordered_map<std::string, double>& GetAllTimes();

    private:
        std::string m_Name;
        unsigned int m_QueryID = 0;
        bool m_Started = false;

        struct TimerData {
            unsigned int QueryID = 0;
            double TimeMS = 0.0;
            bool WaitingForResult = false;
        };

        static std::unordered_map<std::string, TimerData> s_Timers;
        static std::unordered_map<std::string, double> s_LatestTimes;
    };

}
