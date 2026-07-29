#pragma once
#include "UUID.h"
#include <type_traits>

namespace lgt {

    template<typename T>
    class AssetHandle {
    public:
        AssetHandle() : m_Valid(false) {}
        AssetHandle(UUID uuid) : m_UUID(uuid), m_Valid(true) {}

        bool IsValid() const { return m_Valid; }
        UUID GetUUID() const { return m_UUID; }

        bool operator==(const AssetHandle<T>& other) const { return m_UUID == other.m_UUID; }
        bool operator!=(const AssetHandle<T>& other) const { return !(*this == other); }
        operator bool() const { return IsValid(); }

    private:
        UUID m_UUID;
        bool m_Valid = false;
    };

}

namespace std {
    template<typename T>
    struct hash<lgt::AssetHandle<T>> {
        std::size_t operator()(const lgt::AssetHandle<T>& handle) const {
            return hash<lgt::UUID>()(handle.GetUUID());
        }
    };
}
