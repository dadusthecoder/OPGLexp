#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "../Helpers/Logger.h"
#include <typeinfo>

namespace lgt {
    // Singleton resource manager that caches loaded assets by file path.
    // Usage: auto shader = ResourceManager::Load<Shader>("res/shaders/test.glsl");
    class ResourceManager {
    public:
        template<typename T>
        static std::shared_ptr<T> Load(const std::string& path) {
            std::string key = MakeKey<T>(path);
            if (s_Cache.find(key) != s_Cache.end()) {
                return std::static_pointer_cast<T>(s_Cache[key]);
            }

            std::shared_ptr<T> resource = std::shared_ptr<T>(T::Create(path));
            if (resource) {
                s_Cache[key] = resource;
                return resource;
            }

            CORE_ERROR("Failed to load resource: {}", path);
            return nullptr;
        }
        
        template<typename T>
        static std::shared_ptr<T> Get(const std::string& path) {
            std::string key = MakeKey<T>(path);
            if (s_Cache.find(key) != s_Cache.end()) {
                return std::static_pointer_cast<T>(s_Cache[key]);
            }
            return nullptr;
        }
        
        template<typename T>
        static bool Has(const std::string& path) {
            std::string key = MakeKey<T>(path);
            return s_Cache.find(key) != s_Cache.end();
        }
        
        static void Clear() {
            s_Cache.clear();
        }
        
    private:
        // We store void* shared_ptrs keyed by (type_hash, path)
        inline static std::unordered_map<std::string, std::shared_ptr<void>> s_Cache;
        
        template<typename T>
        static std::string MakeKey(const std::string& path) {
            return std::string(typeid(T).name()) + "::" + path;
        }
    };
}
