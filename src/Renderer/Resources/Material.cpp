#include "Material.h"
#include "../../Vendor/glad.h"

namespace lgt {

    Material::Material(Shader* shader)
        : m_Shader(shader) {
    }

    void Material::Bind() const {
        if (!m_Shader) return;

        m_Shader->Bind();

        m_Shader->SetFloat3("u_Material.Albedo", Albedo);
        m_Shader->SetFloat("u_Material.Metallic", Metallic);
        m_Shader->SetFloat("u_Material.Roughness", Roughness);

        // Bind textures if they exist, otherwise bind default textures/colors
        // For Milestone 3 we just set booleans to tell shader if we have maps
        if (AlbedoMap) {
            AlbedoMap->Bind(0);
            m_Shader->SetInt("u_AlbedoMap", 0);
            m_Shader->SetInt("u_UseAlbedoMap", 1);
        } else {
            m_Shader->SetInt("u_UseAlbedoMap", 0);
        }

        if (NormalMap) {
            NormalMap->Bind(1);
            m_Shader->SetInt("u_NormalMap", 1);
            m_Shader->SetInt("u_UseNormalMap", 1);
        } else {
            m_Shader->SetInt("u_UseNormalMap", 0);
        }
        
        if (MetallicMap) {
            MetallicMap->Bind(2);
            m_Shader->SetInt("u_MetallicMap", 2);
            m_Shader->SetInt("u_UseMetallicMap", 1);
        } else {
            m_Shader->SetInt("u_UseMetallicMap", 0);
        }

        if (RoughnessMap) {
            RoughnessMap->Bind(3);
            m_Shader->SetInt("u_RoughnessMap", 3);
            m_Shader->SetInt("u_UseRoughnessMap", 1);
        } else {
            m_Shader->SetInt("u_UseRoughnessMap", 0);
        }
    }

}
