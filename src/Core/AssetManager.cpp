#include "AssetManager.h"

namespace lgt {

    std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::s_Assets;
    std::shared_mutex AssetManager::s_RegistryMutex;

    void AssetManager::Init() {
        // Initialization if needed
    }

    void AssetManager::Shutdown() {
        std::unique_lock<std::shared_mutex> lock(s_RegistryMutex);
        s_Assets.clear();
    }

}
