#pragma once
#include "BlueNoise.h"
#include "TemporalFramework.h"
#include "Vendor/glad.h"

namespace lgt {

struct GPUResources {
    BlueNoise blueNoise;
    TemporalFramework temporal;
    
    // Persistent AO output texture
    GLuint aoTexture = 0;
    int aoWidth = 0, aoHeight = 0;
    
    void Init(int w, int h) {
        blueNoise.Init();
        aoWidth = w;
        aoHeight = h;
        
        glGenTextures(1, &aoTexture);
        glBindTexture(GL_TEXTURE_2D, aoTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, w, h);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    void Resize(int w, int h) {
        if (aoTexture) {
            glDeleteTextures(1, &aoTexture);
        }
        aoWidth = w;
        aoHeight = h;
        
        glGenTextures(1, &aoTexture);
        glBindTexture(GL_TEXTURE_2D, aoTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, w, h);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        temporal.Resize(w, h);
    }
    
    void Shutdown() {
        blueNoise.Shutdown();
        temporal.Shutdown();
        if (aoTexture) {
            glDeleteTextures(1, &aoTexture);
            aoTexture = 0;
        }
    }
};

} // namespace lgt
