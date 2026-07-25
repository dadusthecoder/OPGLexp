#pragma once
#include "Vendor/glad.h"
#include "Helpers/Logger.h"
#include <vector>
#include <random>
#include <cmath>

namespace lgt {

class BlueNoise {
public:
    void Init() {
        CORE_INFO("Initializing Blue Noise textures");
        
        std::vector<uint8_t> scalarData;
        GenerateBlueNoise(128, 1, scalarData);
        
        glGenTextures(1, &m_scalarNoise);
        glBindTexture(GL_TEXTURE_2D, m_scalarNoise);
        // Using glTexImage2D since we're generating data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, scalarData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        std::vector<uint8_t> vec2Data;
        GenerateBlueNoise(128, 2, vec2Data);
        
        glGenTextures(1, &m_vec2Noise);
        glBindTexture(GL_TEXTURE_2D, m_vec2Noise);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, 128, 128, 0, GL_RG, GL_UNSIGNED_BYTE, vec2Data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    
    void Shutdown() {
        if (m_scalarNoise) glDeleteTextures(1, &m_scalarNoise);
        if (m_vec2Noise) glDeleteTextures(1, &m_vec2Noise);
        m_scalarNoise = 0;
        m_vec2Noise = 0;
    }
    
    GLuint GetScalarNoiseTexture() const { return m_scalarNoise; }
    GLuint GetVec2NoiseTexture() const { return m_vec2Noise; }
    
    void Bind(int scalarUnit, int vec2Unit) const {
        glActiveTexture(GL_TEXTURE0 + scalarUnit);
        glBindTexture(GL_TEXTURE_2D, m_scalarNoise);
        glActiveTexture(GL_TEXTURE0 + vec2Unit);
        glBindTexture(GL_TEXTURE_2D, m_vec2Noise);
    }
    
private:
    GLuint m_scalarNoise = 0;  // 128x128 R8 tiling blue noise
    GLuint m_vec2Noise   = 0;  // 128x128 RG8 tiling blue noise
    
    void GenerateBlueNoise(int size, int channels, std::vector<uint8_t>& output) {
        // Fallback to white noise for now
        output.resize(size * size * channels);
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<uint16_t> dist(0, 255);
        for (size_t i = 0; i < output.size(); ++i) {
            output[i] = static_cast<uint8_t>(dist(rng));
        }
    }
};

} // namespace lgt
