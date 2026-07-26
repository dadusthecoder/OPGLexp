#include "OpenGLShader.h"
#include "../../Vendor/glad.h"
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "../../Helpers/Logger.h"

#include <algorithm>

namespace lgt {

    static GLenum ShaderTypeFromString(std::string type) {
        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c){ return std::tolower(c); });
        if (type == "vertex") return GL_VERTEX_SHADER;
        if (type == "fragment" || type == "pixel") return GL_FRAGMENT_SHADER;
        if (type == "compute") return GL_COMPUTE_SHADER;
        return 0;
    }

    OpenGLShader::OpenGLShader(const std::string& filepath) {
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        std::string result;
        if (in) {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1) {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
            }
            else {
                CORE_ERROR("Could not read from file '{0}'", filepath);
            }
        }
        else {
            CORE_ERROR("Could not open file '{0}'", filepath);
        }

        std::unordered_map<GLenum, std::string> shaderSources;
        size_t pos = 0;
        while (true) {
            size_t typePos = result.find("#type", pos);
            size_t shaderPos = result.find("#shader", pos);
            
            size_t foundPos = std::min(typePos, shaderPos);
            if (foundPos == std::string::npos) break;
            
            size_t tokenLen = (foundPos == typePos) ? 5 : 7;
            size_t eol = result.find_first_of("\r\n", foundPos);
            size_t begin = foundPos + tokenLen + 1;
            std::string type = result.substr(begin, eol - begin);
            
            // Trim whitespace
            type.erase(0, type.find_first_not_of(" \t\r\n"));
            type.erase(type.find_last_not_of(" \t\r\n") + 1);

            size_t nextLinePos = result.find_first_not_of("\r\n", eol);
            if (nextLinePos == std::string::npos) break; // empty source
            
            size_t nextTypePos = result.find("#type", nextLinePos);
            size_t nextShaderPos = result.find("#shader", nextLinePos);
            size_t nextFoundPos = std::min(nextTypePos, nextShaderPos);
            
            shaderSources[ShaderTypeFromString(type)] = (nextFoundPos == std::string::npos) 
                ? result.substr(nextLinePos) 
                : result.substr(nextLinePos, nextFoundPos - nextLinePos);
                
            pos = nextFoundPos;
        }

        GLuint program = glCreateProgram();
        std::vector<GLenum> glShaderIDs;
        
        for (auto& kv : shaderSources) {
            GLenum type = kv.first;
            const std::string& source = kv.second;

            GLuint shader = glCreateShader(type);
            const GLchar* sourceCStr = source.c_str();
            glShaderSource(shader, 1, &sourceCStr, 0);
            glCompileShader(shader);

            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE) {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
                if (maxLength > 0) {
                    std::vector<GLchar> infoLog(maxLength);
                    glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
                    CORE_ERROR("Shader compilation failure!");
                    CORE_ERROR("{0}", infoLog.data());
                } else {
                    CORE_ERROR("Shader compilation failure! (No info log)");
                }
                
                glDeleteShader(shader);
                break;
            }

            glAttachShader(program, shader);
            glShaderIDs.push_back(shader);
        }

        m_RendererID = program;
        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            if (maxLength > 0) {
                std::vector<GLchar> infoLog(maxLength);
                glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
                CORE_ERROR("Shader link failure!");
                CORE_ERROR("{0}", infoLog.data());
            } else {
                CORE_ERROR("Shader link failure! (No info log)");
            }
            
            glDeleteProgram(program);
            for (auto id : glShaderIDs) glDeleteShader(id);

            return;
        }

        for (auto id : glShaderIDs) {
            glDetachShader(program, id);
            glDeleteShader(id);
        }

        // Extract name from filepath
        auto lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind('.');
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);
    }

    OpenGLShader::~OpenGLShader() {
        glDeleteProgram(m_RendererID);
    }

    void OpenGLShader::Bind() const {
        glUseProgram(m_RendererID);
    }

    void OpenGLShader::Unbind() const {
        glUseProgram(0);
    }

    void OpenGLShader::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) {
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    }

    int OpenGLShader::GetUniformLocation(const std::string& name) {
        if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
            return m_UniformLocationCache[name];

        int location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location == -1)
            CORE_WARN("Warning: uniform '{0}' doesn't exist!", name);

        m_UniformLocationCache[name] = location;
        return location;
    }

    void OpenGLShader::SetInt(const std::string& name, int value) {
        glProgramUniform1i(m_RendererID, GetUniformLocation(name), value);
    }

    void OpenGLShader::SetUInt(const std::string& name, uint32_t value) {
        glProgramUniform1ui(m_RendererID, GetUniformLocation(name), value);
    }

    void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count) {
        glProgramUniform1iv(m_RendererID, GetUniformLocation(name), count, values);
    }

    void OpenGLShader::SetFloat(const std::string& name, float value) {
        glProgramUniform1f(m_RendererID, GetUniformLocation(name), value);
    }

    void OpenGLShader::SetFloat2(const std::string& name, const glm::vec2& value) {
        glProgramUniform2f(m_RendererID, GetUniformLocation(name), value.x, value.y);
    }

    void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value) {
        glProgramUniform3f(m_RendererID, GetUniformLocation(name), value.x, value.y, value.z);
    }

    void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value) {
        glProgramUniform4f(m_RendererID, GetUniformLocation(name), value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value) {
        glProgramUniformMatrix4fv(m_RendererID, GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }
}
