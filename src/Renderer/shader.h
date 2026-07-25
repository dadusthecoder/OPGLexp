#pragma once
#include "Vendor/glad.h"
#include <cstdlib>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace lgt {
class Camera;
struct MaterialGPU;

enum ShaderType {
    DEPTHSHADER,
    COLORSHADER,
    COMPUTESHADER
};

struct shadersource {
    std::string vertexSource;
    std::string fragmentSource;
    std::string computeSource;
};

class Pipeline {
private:
    std::string                                  m_filepath;
    long long                                    m_lastWriteTime = 0;
    GLuint                                       m_RenderID;
    ShaderType                                   m_type = ShaderType::COLORSHADER;
    mutable std::unordered_map<std::string, int> m_uniformLocationCache;

    // Helper methods
    shadersource parseShader(const std::string& filepath);
    unsigned int compileShader(unsigned int type, const std::string& source);
    unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader);
    unsigned int createComputeShader(const std::string& computeShader);
    void         cacheUniformLocations();
    int          getUniformLocation(const std::string& name) const;

public:
    // Constructor and destructor
    explicit Pipeline(const std::string& filepath);
    explicit Pipeline(const std::string& filepath, ShaderType type);

    ~Pipeline();

    // Delete copy constructor and assignment operator to prevent issues
    Pipeline(const Pipeline&)            = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    // Move constructor and assignment operator
    Pipeline(Pipeline&& other) noexcept
        : m_filepath(std::move(other.m_filepath)),
          m_RenderID(other.m_RenderID),
          m_uniformLocationCache(std::move(other.m_uniformLocationCache)) {
        other.m_RenderID = 0;
    }

    Pipeline& operator=(Pipeline&& other) noexcept {
        if (this != &other) {
            if (m_RenderID != 0) {
                glDeleteProgram(m_RenderID);
            }

            m_filepath             = std::move(other.m_filepath);
            m_RenderID             = other.m_RenderID;
            m_uniformLocationCache = std::move(other.m_uniformLocationCache);

            other.m_RenderID = 0;
        }
        return *this;
    }

    // Modern shader binding methods
    void use() const;
    void useWithCamera(Camera& Camera);
    void unuse() const;
    void dispatch(unsigned int x, unsigned int y, unsigned int z) const;

    // Modern uniform setting methods
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setIVec3(const std::string& name, const glm::ivec3& value) const;
    void setIVec3(const std::string& name, int x, int y, int z) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    // High-level uniform setting methods
    void setMaterial(uint32_t index) const;

    // Utility methods
    GLuint             getID() const;
    ShaderType         getType() const { return m_type; };
    const std::string& getPath() const;
    bool               isValid() const;
    void               reload();
    void               reloadIfModified();
    void               printActiveUniforms() const;
};
} // namespace lgt
