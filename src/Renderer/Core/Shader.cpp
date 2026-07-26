#include "Shader.h"
#include "../Backend/OpenGLShader.h"

namespace lgt {

    Shader* Shader::Create(const std::string& filepath) {
        // Currently we only support OpenGL
        return new OpenGLShader(filepath);
    }

    Shader* Shader::CreateCompute(const std::string& filepath) {
        return new OpenGLShader(filepath);
    }

}
