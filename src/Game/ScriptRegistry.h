#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "../Scene/Entity.h"

namespace lgt {

    class NativeScript;

    using ScriptInstantiateFn = std::function<NativeScript*()>;
    using ScriptDestroyFn = std::function<void(NativeScript*)>;

    struct ScriptFunctions {
        ScriptInstantiateFn Instantiate;
        ScriptDestroyFn Destroy;
    };

    class ScriptRegistry {
    public:
        static void RegisterScript(const std::string& name, ScriptInstantiateFn instantiate, ScriptDestroyFn destroy);
        static bool GetScriptFunctions(const std::string& name, ScriptFunctions& outFunctions);
        
        // This function will be generated/updated automatically by the MCP server
        static void RegisterAllScripts();

    private:
        static std::unordered_map<std::string, ScriptFunctions> s_Registry;
    };

    // Helper macro to register scripts automatically or manually
    #define REGISTER_SCRIPT(ScriptClass) \
        lgt::ScriptRegistry::RegisterScript(#ScriptClass, \
            []() { return static_cast<lgt::NativeScript*>(new ScriptClass()); }, \
            [](lgt::NativeScript* s) { delete s; })

}
