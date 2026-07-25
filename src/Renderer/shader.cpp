#include "Shader.h"
#include "Camera.h"

#include "Renderer.h"
#include "Helpers/Logger.h"
#include <filesystem>

namespace lgt {
Pipeline::Pipeline(const std::string& filepath)
    : m_filepath(filepath),
      m_RenderID(0) {
    try {
        if (std::filesystem::exists(filepath)) {
            m_lastWriteTime = std::filesystem::last_write_time(filepath).time_since_epoch().count();
        }
    } catch (...) {}

    shadersource source = parseShader(filepath);
    if (!source.computeSource.empty()) {
        m_type = ShaderType::COMPUTESHADER;
        m_RenderID = createComputeShader(source.computeSource);
    } else {
        m_RenderID = createShader(source.vertexSource, source.fragmentSource);
    }

    cacheUniformLocations();
    CORE_INFO("Shader loaded from: {} | ID: {}", filepath, m_RenderID);
}

Pipeline::Pipeline(const std::string& filepath, ShaderType type)
    : m_filepath(filepath),
      m_RenderID(0),
      m_type(type) {
    try {
        if (std::filesystem::exists(filepath)) {
            m_lastWriteTime = std::filesystem::last_write_time(filepath).time_since_epoch().count();
        }
    } catch (...) {}

    shadersource source = parseShader(filepath);
    if (type == ShaderType::COMPUTESHADER || !source.computeSource.empty()) {
        m_type = ShaderType::COMPUTESHADER;
        m_RenderID = createComputeShader(source.computeSource);
    } else {
        m_RenderID = createShader(source.vertexSource, source.fragmentSource);
    }

    cacheUniformLocations();
    CORE_INFO("Shader loaded from: {} | ID: {}", filepath, m_RenderID);
}

Pipeline::~Pipeline() {
    if (m_RenderID != 0) {
        glDeleteProgram(m_RenderID);
        CORE_INFO("Shader deleted | ID: {}", m_RenderID);
    }
}

void Pipeline::use() const {
    glUseProgram(m_RenderID);
}

void Pipeline::useWithCamera(Camera& camera) {
    glUseProgram(m_RenderID);
    setMat4("u_View", camera.GetViewMatrix());
    setMat4("u_Projection", camera.GetProjectionMatrix());
    setVec3("u_CameraPos", camera.GetCameraPos());
}

void Pipeline::unuse() const {
    glUseProgram(0);
}

shadersource Pipeline::parseShader(const std::string& filepath) {
    m_filepath = filepath;

    std::stringstream ss[3];

    enum class InternalShaderType {
        NONE     = -1,
        VERTEX   = 0,
        FRAGMENT = 1,
        COMPUTE  = 2
    };
    InternalShaderType type = InternalShaderType::NONE;

    auto processFile = [&](const std::string& currentFilepath, InternalShaderType& currentType, auto& processFileRef) -> void {
        std::ifstream stream(currentFilepath);
        if (!stream.is_open()) {
            CORE_ERROR("Failed to open shader file: {}", currentFilepath);
            return;
        }

        std::string line;
        while (getline(stream, line)) {
            if (line.find("#shader") != std::string::npos) {
                if (line.find("Vertex") != std::string::npos)
                    currentType = InternalShaderType::VERTEX;
                else if (line.find("Fragment") != std::string::npos)
                    currentType = InternalShaderType::FRAGMENT;
                else if (line.find("Compute") != std::string::npos)
                    currentType = InternalShaderType::COMPUTE;
            } else if (line.find("#include") != std::string::npos) {
                size_t firstQuote = line.find_first_of("\"<");
                size_t lastQuote = line.find_last_of("\">");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote < lastQuote) {
                    std::string incFilename = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    std::filesystem::path p(currentFilepath);
                    std::filesystem::path incPath = p.parent_path() / incFilename;
                    if (!std::filesystem::exists(incPath)) {
                        incPath = std::filesystem::path("res/shaders") / incFilename;
                    }
                    processFileRef(incPath.string(), currentType, processFileRef);
                }
            } else if (currentType != InternalShaderType::NONE) {
                ss[static_cast<int>(currentType)] << line << "\n";
            }
        }
    };

    processFile(filepath, type, processFile);

    return {ss[0].str(), ss[1].str(), ss[2].str()};
}

unsigned int Pipeline::compileShader(unsigned int type, const std::string& source) {
    unsigned int id  = glCreateShader(type);
    const char*  src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    std::string shaderTypeStr;
    if (type == GL_VERTEX_SHADER) shaderTypeStr = "Vertex";
    else if (type == GL_FRAGMENT_SHADER) shaderTypeStr = "Fragment";
    else if (type == GL_COMPUTE_SHADER) shaderTypeStr = "Compute";

    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> errorLog(length);
        glGetShaderInfoLog(id, length, &length, errorLog.data());

        CORE_ERROR("{} shader compilation error: {}", shaderTypeStr, errorLog.data());
        glDeleteShader(id);
        return 0;
    }

    CORE_TRACE("{} shader compiled successfully.", shaderTypeStr);
    return id;
}

unsigned int Pipeline::createShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs      = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs      = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    if (vs == 0 || fs == 0) {
        CORE_ERROR("Shader compilation failed, cannot create program");
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        int length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> errorLog(length);
        glGetProgramInfoLog(program, length, &length, errorLog.data());

        CORE_ERROR("Shader program linking error: {}", errorLog.data());

        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(program);
        return 0;
    }

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        CORE_WARN("Shader program validation failed for Program ID: {}", program);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    CORE_INFO("Shader program linked successfully | Program ID: {}", program);
    return program;
}

unsigned int Pipeline::createComputeShader(const std::string& computeShader) {
    unsigned int program = glCreateProgram();
    unsigned int cs      = compileShader(GL_COMPUTE_SHADER, computeShader);

    if (cs == 0) {
        CORE_ERROR("Compute Shader compilation failed, cannot create program");
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, cs);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        int length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> errorLog(length);
        glGetProgramInfoLog(program, length, &length, errorLog.data());

        CORE_ERROR("Compute Shader program linking error: {}", errorLog.data());

        glDeleteShader(cs);
        glDeleteProgram(program);
        return 0;
    }

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        CORE_WARN("Compute Shader program validation failed for Program ID: {}", program);
    }

    glDeleteShader(cs);

    CORE_INFO("Compute Shader program linked successfully | Program ID: {}", program);
    return program;
}

void Pipeline::dispatch(unsigned int x, unsigned int y, unsigned int z) const {
    if (m_type == ShaderType::COMPUTESHADER && m_RenderID != 0) {
        glUseProgram(m_RenderID);
        glDispatchCompute(x, y, z);
    } else {
        CORE_WARN("Attempted to dispatch a non-compute shader!");
    }
}

void Pipeline::cacheUniformLocations() {
    if (m_RenderID == 0)
        return;

    // Common uniforms that we'll cache for performance
    std::vector<std::string> commonUniforms = {"u_model", "u_view", "u_projection", "u_viewPos", "u_useColor", "u_color"};

    for (const auto& uniform : commonUniforms) {
        int location = glGetUniformLocation(m_RenderID, uniform.c_str());
        if (location != -1) {
            m_uniformLocationCache[uniform] = location;
        }
    }
}

int Pipeline::getUniformLocation(const std::string& name) const {
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    int location = glGetUniformLocation(m_RenderID, name.c_str());
    if (location != -1) {
        m_uniformLocationCache[name] = location;
    }
    return location;
}

void Pipeline::setBool(const std::string& name, bool value) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform1i(loc, static_cast<int>(value));
}

void Pipeline::setInt(const std::string& name, int value) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform1i(loc, value);
}

void Pipeline::setFloat(const std::string& name, float value) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform1f(loc, value);
}

void Pipeline::setVec2(const std::string& name, const glm::vec2& value) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform2fv(loc, 1, &value[0]);
}

void Pipeline::setVec2(const std::string& name, float x, float y) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform2f(loc, x, y);
}

void Pipeline::setVec3(const std::string& name, const glm::vec3& value) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform3fv(loc, 1, &value[0]);
}

void Pipeline::setVec3(const std::string& name, float x, float y, float z) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform3f(loc, x, y, z);
}

void Pipeline::setVec4(const std::string& name, const glm::vec4& value) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform4fv(loc, 1, &value[0]);
}

void Pipeline::setVec4(const std::string& name, float x, float y, float z, float w) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniform4f(loc, x, y, z, w);
}

void Pipeline::setMat2(const std::string& name, const glm::mat2& mat) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniformMatrix2fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Pipeline::setMat3(const std::string& name, const glm::mat3& mat) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniformMatrix3fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Pipeline::setMat4(const std::string& name, const glm::mat4& mat) const {
    int loc = getUniformLocation(name);
    if (loc != -1)
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Pipeline::setMaterial(uint32_t index) const {
    setInt("u_MaterialIndex", index);
}

GLuint Pipeline::getID() const {
    return m_RenderID;
}

const std::string& Pipeline::getPath() const {
    return m_filepath;
}

bool Pipeline::isValid() const {
    return m_RenderID != 0;
}

// Removed old reload function
void Pipeline::printActiveUniforms() const {
    if (m_RenderID == 0)
        return;

    GLint numUniforms;
    glGetProgramiv(m_RenderID, GL_ACTIVE_UNIFORMS, &numUniforms);
    CORE_INFO("Active uniforms {}:", numUniforms);

    for (GLint i = 0; i < numUniforms; ++i) {
        char    name[256];
        GLsizei length;
        GLint   size;
        GLenum  type;

        glGetActiveUniform(m_RenderID, i, sizeof(name), &length, &size, &type, name);
        int location = glGetUniformLocation(m_RenderID, name);
        CORE_INFO("  [{}] {}", location, name);
    }
}

void Pipeline::reload() {
    if (m_filepath.empty()) return;
    
    CORE_INFO("Reloading shader: {}", m_filepath);
    shadersource source = parseShader(m_filepath);
    
    GLuint newRenderID = 0;
    if (m_type == ShaderType::COMPUTESHADER || !source.computeSource.empty()) {
        newRenderID = createComputeShader(source.computeSource);
    } else {
        newRenderID = createShader(source.vertexSource, source.fragmentSource);
    }
    
    // If compilation succeeded, swap out the old ID for the new one
    if (newRenderID != 0) {
        if (m_RenderID != 0) {
            glDeleteProgram(m_RenderID);
        }
        m_RenderID = newRenderID;
        m_uniformLocationCache.clear();
        cacheUniformLocations();
        CORE_INFO("Shader hot-reload successful for: {}", m_filepath);
    } else {
        CORE_ERROR("Shader hot-reload failed for: {}. Keeping previous version.", m_filepath);
    }
}

void Pipeline::reloadIfModified() {
    if (m_filepath.empty()) return;
    
    try {
        if (std::filesystem::exists(m_filepath)) {
            long long currentWriteTime = std::filesystem::last_write_time(m_filepath).time_since_epoch().count();
            if (currentWriteTime != m_lastWriteTime) {
                m_lastWriteTime = currentWriteTime;
                reload();
            }
        }
    } catch (...) {
        // Ignore file access errors during polling
    }
}

} // namespace lgt
