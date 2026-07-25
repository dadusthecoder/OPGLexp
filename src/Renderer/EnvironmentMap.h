#pragma once
#include "Vendor/glad.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "Shader.h"
#include "Camera.h"

namespace lgt {

class EnvironmentMap {
public:
    EnvironmentMap();
    ~EnvironmentMap();

    bool LoadHDR(const std::string& filepath);
    void GenerateMaps(); // Generates irradiance, prefilter, and BRDF LUT

    // Getters
    GLuint GetEnvironmentCubemap() const { return m_envCubemap; }
    GLuint GetIrradianceMap() const      { return m_irradianceMap; }
    GLuint GetPrefilterMap() const       { return m_prefilterMap; }
    GLuint GetBrdfLUT() const            { return m_brdfLUT; }

    bool IsLoaded() const { return m_isLoaded; }
    const std::string& GetFilepath() const { return m_filepath; }

private:
    std::string m_filepath;
    bool        m_isLoaded = false;

    // Generated textures
    GLuint m_hdrTexture    = 0;
    GLuint m_envCubemap    = 0;
    GLuint m_irradianceMap = 0;
    GLuint m_prefilterMap  = 0;
    GLuint m_brdfLUT       = 0;

    // Helper cube rendering
    GLuint m_cubeVAO = 0;
    GLuint m_cubeVBO = 0;
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    void renderCube();
    void renderQuad();
    
    void setupCube();
    void setupQuad();
};

} // namespace lgt
