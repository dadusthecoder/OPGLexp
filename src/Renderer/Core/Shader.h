#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace lgt {

    class Shader {
    public:
        virtual ~Shader();

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        // Compute dispatch
        virtual void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) = 0;

        // Uniforms
        virtual void SetInt(const std::string& name, int value) = 0;
        virtual void SetInt3(const std::string& name, const glm::ivec3& value) = 0;
        virtual void SetUInt(const std::string& name, uint32_t value) = 0;
        virtual void SetIntArray(const std::string& name, int* values, uint32_t count) = 0;
        virtual void SetFloat(const std::string& name, float value) = 0;
        virtual void SetFloat2(const std::string& name, const glm::vec2& value) = 0;
        virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

        virtual const std::string& GetName() const = 0;

        static Shader* Create(const std::string& filepath);
        static Shader* CreateCompute(const std::string& filepath);

        virtual void Reload() = 0;
        static void ReloadAll();
    };

}
