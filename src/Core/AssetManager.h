#pragma once
#include "AssetHandle.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace lgt {

    enum class AssetState {
        Unloaded = 0,
        Loading,
        Loaded,
        Failed
    };

    class Asset {
    public:
        virtual ~Asset() = default;
        UUID Handle;
        AssetState State = AssetState::Unloaded;
    };

    class AssetManager {
    public:
        static void Init();
        static void Shutdown();

        template<typename T, typename... Args>
        static AssetHandle<T> CreateAsset(Args&&... args) {
            UUID id;
            std::shared_ptr<T> asset = std::make_shared<T>(std::forward<Args>(args)...);
            asset->Handle = id;
            asset->State = AssetState::Loaded;
            
            std::unique_lock<std::shared_mutex> lock(s_RegistryMutex);
            s_Assets[id] = asset;
            return AssetHandle<T>(id);
        }

        template<typename T>
        static std::shared_ptr<T> GetAsset(AssetHandle<T> handle) {
            std::shared_lock<std::shared_mutex> lock(s_RegistryMutex);
            auto it = s_Assets.find(handle.GetUUID());
            if (it != s_Assets.end()) {
                return std::static_pointer_cast<T>(it->second);
            }
            return nullptr;
        }

        template<typename T>
        static AssetState GetAssetState(AssetHandle<T> handle) {
            std::shared_lock<std::shared_mutex> lock(s_RegistryMutex);
            auto it = s_Assets.find(handle.GetUUID());
            if (it != s_Assets.end()) {
                return it->second->State;
            }
            return AssetState::Unloaded;
        }

    private:
        static std::unordered_map<UUID, std::shared_ptr<Asset>> s_Assets;
        static std::shared_mutex s_RegistryMutex;
    };

}
