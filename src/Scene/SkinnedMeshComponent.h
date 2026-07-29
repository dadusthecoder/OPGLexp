#pragma once
#include "../Core/AssetHandle.h"
#include "../Renderer/Resources/Skeleton.h"
#include "../Renderer/Resources/Mesh.h"

namespace lgt {

    struct SkinnedMeshComponent {
        AssetHandle<Skeleton> skeletonHandle;
        AssetHandle<Mesh> meshHandle;

        SkinnedMeshComponent() = default;
        SkinnedMeshComponent(const SkinnedMeshComponent&) = default;
    };

}
