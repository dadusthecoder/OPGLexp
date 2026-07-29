#include "DebugRenderer.h"
#include "../../Vendor/glad.h"
#include <vector>
#include "Shader.h"
#include "../../Helpers/Logger.h"

namespace lgt {

    struct DebugRendererData {
        uint32_t VAO = 0;
        uint32_t VBO = 0;
        
        uint32_t LineCount = 0;
        std::vector<DebugRenderer::LineVertex> LineVertices;

        Shader* LineShader;
        glm::mat4 ViewProjMatrix;
    };

    static DebugRendererData s_Data;

    void DebugRenderer::Init() {
        glGenVertexArrays(1, &s_Data.VAO);
        glGenBuffers(1, &s_Data.VBO);

        glBindVertexArray(s_Data.VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, s_Data.VBO);
        // Pre-allocate buffer for 10000 lines (20000 vertices)
        glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertex) * 20000, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (const void*)offsetof(LineVertex, Position));
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (const void*)offsetof(LineVertex, Color));

        glBindVertexArray(0);

        s_Data.LineShader = Shader::Create("res/shaders/line.glsl");
    }

    void DebugRenderer::Shutdown() {
        glDeleteVertexArrays(1, &s_Data.VAO);
        glDeleteBuffers(1, &s_Data.VBO);
    }

    void DebugRenderer::BeginScene(const glm::mat4& viewProjMatrix) {
        s_Data.ViewProjMatrix = viewProjMatrix;
        s_Data.LineCount = 0;
        s_Data.LineVertices.clear();
    }

    void DebugRenderer::EndScene() {
        if (s_Data.LineCount == 0) return;

        glBindBuffer(GL_ARRAY_BUFFER, s_Data.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, s_Data.LineVertices.size() * sizeof(LineVertex), s_Data.LineVertices.data());

        s_Data.LineShader->Bind();
        s_Data.LineShader->SetMat4("u_ViewProjection", s_Data.ViewProjMatrix);

        glBindVertexArray(s_Data.VAO);
        glDrawArrays(GL_LINES, 0, s_Data.LineCount * 2);
        glBindVertexArray(0);
    }

    void DebugRenderer::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color) {
        if (s_Data.LineCount >= 10000) return; // Reached max lines for this frame

        s_Data.LineVertices.push_back({ p0, color });
        s_Data.LineVertices.push_back({ p1, color });
        s_Data.LineCount++;
    }

}
