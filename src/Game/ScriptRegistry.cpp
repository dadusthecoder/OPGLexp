#include "ScriptRegistry.h"
#include "../Scene/NativeScript.h"
#include <iostream>

namespace lgt {

    std::unordered_map<std::string, ScriptFunctions> ScriptRegistry::s_Registry;

    void ScriptRegistry::RegisterScript(const std::string& name, ScriptInstantiateFn instantiate, ScriptDestroyFn destroy) {
        if (s_Registry.find(name) == s_Registry.end()) {
            s_Registry[name] = { instantiate, destroy };
        } else {
            std::cout << "Warning: Script '" << name << "' is already registered!" << std::endl;
        }
    }

    bool ScriptRegistry::GetScriptFunctions(const std::string& name, ScriptFunctions& outFunctions) {
        auto it = s_Registry.find(name);
        if (it != s_Registry.end()) {
            outFunctions = it->second;
            return true;
        }
        return false;
    }

    void ScriptRegistry::RegisterAllScripts() {
        // The Game Dev MCP Server will inject register calls here for new scripts.
        // E.g., REGISTER_SCRIPT(PlayerController);
    }

}
