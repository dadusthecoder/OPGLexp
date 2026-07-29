#include "Material.h"
#include "../../Vendor/glad.h"
#include <algorithm>

namespace lgt {

    Material::Material(Shader* shader)
        : m_Shader(shader) {
    }

    void Material::Bind() const {
        if (!m_Shader) return;

        m_Shader->Bind();

        m_Shader->SetFloat3("u_Albedo", Albedo);
        m_Shader->SetFloat("u_Metallic", Metallic);
        m_Shader->SetFloat("u_Roughness", Roughness);
        
        float maxEmissive = std::max(Emissive.r, std::max(Emissive.g, Emissive.b));
        m_Shader->SetFloat("u_EmissiveStrength", maxEmissive);

        // Bind textures if they exist, otherwise bind default textures/colors
        if (AlbedoMap) {
            AlbedoMap->Bind(0);
            m_Shader->SetInt("u_AlbedoTex", 0);
            m_Shader->SetInt("u_HasAlbedoTex", 1);
        } else {
            m_Shader->SetInt("u_HasAlbedoTex", 0);
        }

        if (NormalMap) {
            NormalMap->Bind(1);
            m_Shader->SetInt("u_NormalTex", 1);
            m_Shader->SetInt("u_HasNormalTex", 1);
        } else {
            m_Shader->SetInt("u_HasNormalTex", 0);
        }
        
        // geometry.glsl expects a combined metallic/roughness texture, but we have separate. 
        // For now, if either exist, we just pass one of them to slot 2 and hope they are packed, 
        // or just rely on scalar metallic/roughness if they don't exist.
        if (MetallicMap || RoughnessMap) {
            if (MetallicMap) MetallicMap->Bind(2);
            else if (RoughnessMap) RoughnessMap->Bind(2);
            m_Shader->SetInt("u_MetallicRoughnessTex", 2);
            m_Shader->SetInt("u_HasMetallicRoughnessTex", 1);
        } else {
            m_Shader->SetInt("u_HasMetallicRoughnessTex", 0);
        }
    }

}
