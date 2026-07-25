#pragma once
#include "Vendor/glad.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include "Helpers/Logger.h"

namespace lgt {

struct TemporalResource {
    GLuint textures[2] = {0, 0};
    int    currentIdx  = 0;
    int    frameCount  = 0;
    bool   valid       = false;
    int    width = 0, height = 0;
    GLenum format = GL_R16F;
    
    GLuint Current()  const { return textures[currentIdx]; }
    GLuint Previous() const { return textures[1 - currentIdx]; }
    void   Swap()           { currentIdx = 1 - currentIdx; frameCount++; valid = true; }
    void   Invalidate()     { valid = false; frameCount = 0; }
};

class TemporalFramework {
public:
    TemporalResource& Register(const std::string& name, int w, int h, GLenum format) {
        TemporalResource res;
        res.width = w;
        res.height = h;
        res.format = format;
        
        glGenTextures(2, res.textures);
        for (int i = 0; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, res.textures[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, format, w, h);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        
        m_resources[name] = res;
        return m_resources[name];
    }
    
    void BeginFrame(const glm::mat4& view, const glm::mat4& proj) {
        if (!m_firstFrame) {
            // Check camera cut
            // Basic translation check:
            glm::mat4 invPrevView = glm::inverse(m_prevView);
            glm::mat4 invView = glm::inverse(view);
            glm::vec3 prevPos(invPrevView[3]);
            glm::vec3 currPos(invView[3]);
            
            float dist = glm::length(currPos - prevPos);
            
            // Extract forward vector
            glm::vec3 prevFwd(-m_prevView[0][2], -m_prevView[1][2], -m_prevView[2][2]);
            glm::vec3 currFwd(-view[0][2], -view[1][2], -view[2][2]);
            
            float dotFwd = glm::dot(glm::normalize(prevFwd), glm::normalize(currFwd));
            // dotFwd = cos(angle), threshold > 45 deg -> cos(45) ~ 0.707
            
            if (dist > 5.0f || dotFwd < 0.707f) {
                CORE_WARN("Camera cut detected, invalidating temporal history");
                for (auto& pair : m_resources) {
                    pair.second.Invalidate();
                }
            }
        }
        
        m_prevView = view;
        m_prevProj = proj;
        m_firstFrame = false;
    }
    
    TemporalResource& Get(const std::string& name) {
        return m_resources.at(name);
    }
    
    bool Has(const std::string& name) const {
        return m_resources.find(name) != m_resources.end();
    }
    
    void Resize(int w, int h) {
        for (auto& pair : m_resources) {
            TemporalResource& res = pair.second;
            if (res.width == w && res.height == h) continue;
            
            glDeleteTextures(2, res.textures);
            
            res.width = w;
            res.height = h;
            
            glGenTextures(2, res.textures);
            for (int i = 0; i < 2; ++i) {
                glBindTexture(GL_TEXTURE_2D, res.textures[i]);
                glTexStorage2D(GL_TEXTURE_2D, 1, res.format, w, h);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            res.Invalidate();
        }
    }
    
    void Shutdown() {
        for (auto& pair : m_resources) {
            glDeleteTextures(2, pair.second.textures);
        }
        m_resources.clear();
    }
    
private:
    std::unordered_map<std::string, TemporalResource> m_resources;
    glm::mat4 m_prevView = glm::mat4(1.0f);
    glm::mat4 m_prevProj = glm::mat4(1.0f);
    bool m_firstFrame = true;
};

} // namespace lgt
