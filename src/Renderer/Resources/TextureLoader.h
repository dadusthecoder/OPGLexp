#pragma once
#include <string>
#include <memory>
#include "../Core/Texture.h"

namespace lgt {
    class TextureLoader {
    public:
        // Load a 2D texture from file. sRGB=true for albedo/emissive, false for normal/metallic/roughness.
        static std::shared_ptr<Texture> LoadFromFile(const std::string& path, bool sRGB = true);
    };
}
