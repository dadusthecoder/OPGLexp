#include "GPUTimer.h"
#include "../Vendor/glad.h"
#include "Logger.h"

namespace lgt {

    std::unordered_map<std::string, ScopedGPUTimer::TimerData> ScopedGPUTimer::s_Timers;
    std::unordered_map<std::string, double> ScopedGPUTimer::s_LatestTimes;

    ScopedGPUTimer::ScopedGPUTimer(const std::string& name) : m_Name(name) {
        if (s_Timers.find(name) == s_Timers.end()) {
            glGenQueries(1, &s_Timers[name].QueryID);
        }
        
        m_QueryID = s_Timers[name].QueryID;
        
        // Don't start a new query if we're still waiting for the previous frame's result
        if (!s_Timers[name].WaitingForResult) {
            glBeginQuery(GL_TIME_ELAPSED, m_QueryID);
            m_Started = true;
        }
    }

    ScopedGPUTimer::~ScopedGPUTimer() {
        if (m_Started) {
            glEndQuery(GL_TIME_ELAPSED);
            s_Timers[m_Name].WaitingForResult = true;
        }
    }

    double ScopedGPUTimer::GetTimeMS(const std::string& name) {
        if (s_LatestTimes.find(name) != s_LatestTimes.end()) {
            return s_LatestTimes[name];
        }
        return 0.0;
    }

    const std::unordered_map<std::string, double>& ScopedGPUTimer::GetAllTimes() {
        return s_LatestTimes;
    }

    void ScopedGPUTimer::RetrieveResults() {
        for (auto& [name, data] : s_Timers) {
            if (data.WaitingForResult) {
                GLuint available = 0;
                glGetQueryObjectuiv(data.QueryID, GL_QUERY_RESULT_AVAILABLE, &available);
                if (available) {
                    GLuint64 timeElapsed = 0;
                    glGetQueryObjectui64v(data.QueryID, GL_QUERY_RESULT, &timeElapsed);
                    data.TimeMS = (double)timeElapsed / 1000000.0; // ns to ms
                    s_LatestTimes[name] = data.TimeMS;
                    data.WaitingForResult = false;
                }
            }
        }
    }

}
