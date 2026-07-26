#include "DebugStats.h"

namespace lgt {

    std::unordered_map<std::string, std::string> DebugStats::s_Stats;
    std::mutex DebugStats::s_Mutex;

}
