#include "Shader.h"
#include "../Backend/OpenGLShader.h"

namespace lgt {

    static std::vector<Shader*> s_AllShaders;

    Shader::~Shader() {
        auto it = std::find(s_AllShaders.begin(), s_AllShaders.end(), this);
        if (it != s_AllShaders.end())
            s_AllShaders.erase(it);
    }

    Shader* Shader::Create(const std::string& filepath) {
        Shader* shader = new OpenGLShader(filepath);
        s_AllShaders.push_back(shader);
        return shader;
    }

    Shader* Shader::CreateCompute(const std::string& filepath) {
        Shader* shader = new OpenGLShader(filepath);
        s_AllShaders.push_back(shader);
        return shader;
    }

    void Shader::ReloadAll() {
        for (Shader* shader : s_AllShaders) {
            shader->Reload();
        }
    }

}
