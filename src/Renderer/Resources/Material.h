#pragma once

#include <glm/glm.hpp>
#include "../Core/Shader.h"
#include "../Core/Texture.h"
#include <memory>

namespace lgt {

    class Material {
    public:
        Material(Shader* shader);
        ~Material() = default;

        void Bind() const;

        // PBR parameters
        glm::vec3 Albedo = { 1.0f, 1.0f, 1.0f };
        float Metallic = 0.0f;
        float Roughness = 0.5f;

        // Base textures (optional)
        std::shared_ptr<Texture> AlbedoMap = nullptr;
        std::shared_ptr<Texture> NormalMap = nullptr;
        std::shared_ptr<Texture> MetallicMap = nullptr;
        std::shared_ptr<Texture> RoughnessMap = nullptr;

        Shader* GetShader() const { return m_Shader; }

    private:
        Shader* m_Shader = nullptr;
    };

}
