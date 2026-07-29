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

    static std::string ProcessIncludes(const std::string& source, const std::string& currentFileDir) {
        std::string result;
        std::istringstream stream(source);
        std::string line;
        while (std::getline(stream, line)) {
            size_t includePos = line.find("#include");
            if (includePos != std::string::npos) {
                size_t firstQuote = line.find('"', includePos);
                size_t lastQuote = line.find('"', firstQuote + 1);
                if (firstQuote != std::string::npos && lastQuote != std::string::npos) {
                    std::string includePath = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    std::string fullPath = currentFileDir + "/" + includePath;
                    std::ifstream in(fullPath);
                    if (in) {
                        std::string includeSource((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                        auto lastSlash = fullPath.find_last_of("/\\");
                        std::string nextDir = lastSlash == std::string::npos ? "." : fullPath.substr(0, lastSlash);
                        result += ProcessIncludes(includeSource, nextDir) + "\n";
                    } else {
                        CORE_ERROR("Could not open include file '{}'", fullPath);
                    }
                    continue;
                }
            }
            result += line + "\n";
        }
        return result;
    }

    OpenGLShader::OpenGLShader(const std::string& filepath) 
        : m_RendererID(0), m_Filepath(filepath) 
    {
        // Extract name from filepath
        auto lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind('.');
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);
        
        Reload();
    }

    void OpenGLShader::Reload() {
        std::ifstream in(m_Filepath, std::ios::in | std::ios::binary);
        std::string result;
        if (in) {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1) {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
                
                auto lastSlash = m_Filepath.find_last_of("/\\");
                std::string dir = lastSlash == std::string::npos ? "." : m_Filepath.substr(0, lastSlash);
                result = ProcessIncludes(result, dir);
            }
            else {
                CORE_ERROR("Could not read from file '{}'", m_Filepath);
                return;
            }
        }
        else {
            CORE_ERROR("Could not open file '{}'", m_Filepath);
            return;
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
            
            type.erase(0, type.find_first_not_of(" \t\r\n"));
            type.erase(type.find_last_not_of(" \t\r\n") + 1);

            size_t nextLinePos = result.find_first_not_of("\r\n", eol);
            if (nextLinePos == std::string::npos) break;
            
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
        bool compileFailed = false;
        
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
                    CORE_ERROR("Shader compilation failure in {}!", m_Filepath);
                    CORE_ERROR("{}", infoLog.data());
                } else {
                    CORE_ERROR("Shader compilation failure in {}! (No info log)", m_Filepath);
                }
                
                glDeleteShader(shader);
                compileFailed = true;
                break;
            }

            glAttachShader(program, shader);
            glShaderIDs.push_back(shader);
        }

        if (compileFailed) {
            glDeleteProgram(program);
            for (auto id : glShaderIDs) glDeleteShader(id);
            return;
        }

        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            if (maxLength > 0) {
                std::vector<GLchar> infoLog(maxLength);
                glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
                CORE_ERROR("Shader link failure in {}!", m_Filepath);
                CORE_ERROR("{}", infoLog.data());
            } else {
                CORE_ERROR("Shader link failure in {}! (No info log)", m_Filepath);
            }
            
            glDeleteProgram(program);
            for (auto id : glShaderIDs) glDeleteShader(id);
            return;
        }

        for (auto id : glShaderIDs) {
            glDetachShader(program, id);
            glDeleteShader(id);
        }

        // Only apply the new program if compilation and linking succeeded
        if (m_RendererID != 0) {
            glDeleteProgram(m_RendererID);
        }
        m_RendererID = program;
        m_UniformLocationCache.clear();
        CORE_INFO("Successfully reloaded shader: {}", m_Name);
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
        if (location == -1) {
            // Unused uniforms are naturally optimized out. Downgrade to TRACE/DEBUG or remove entirely.
            m_UniformLocationCache[name] = location;
            return location;
        }

        m_UniformLocationCache[name] = location;
        return location;
    }

    void OpenGLShader::SetInt(const std::string& name, int value) {
        glProgramUniform1i(m_RendererID, GetUniformLocation(name), value);
    }

    void OpenGLShader::SetInt3(const std::string& name, const glm::ivec3& value) {
        glProgramUniform3i(m_RendererID, GetUniformLocation(name), value.x, value.y, value.z);
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
