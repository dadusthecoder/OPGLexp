#include "Renderer.h"
#include "../../Vendor/glad.h"
#include "../Resources/Material.h"
#include "../Resources/Mesh.h"
#include "Framebuffer.h"

namespace lgt {

    RenderCommandQueue Renderer::s_CommandQueue;

    static GLuint s_VAO = 0;
    static Framebuffer* s_GBuffer = nullptr;
    static uint32_t s_ViewportWidth = 800;
    static uint32_t s_ViewportHeight = 600;
    static glm::mat4 s_ViewProjection = glm::mat4(1.0f);
    
    static GLuint s_QuadVAO = 0;
    static GLuint s_QuadVBO = 0;
    static Shader* s_LightingShader = nullptr;
    static Shader* s_PostProcessShader = nullptr;
    static Framebuffer* s_HDRBuffer = nullptr;
    static Framebuffer* s_FinalBuffer = nullptr;
    static std::vector<Renderer::LightData> s_Lights;

    void Renderer::Init() {
        if (s_VAO == 0) {
            glGenVertexArrays(1, &s_VAO);
            glBindVertexArray(s_VAO);
        }

        if (!s_GBuffer) {
            FramebufferDescriptor desc;
            desc.width = s_ViewportWidth;
            desc.height = s_ViewportHeight;
            // Color attachment 0: Albedo (RGB) + Specular/Emission (A)
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA8));
            // Color attachment 1: Normal (RGB)
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA16F));
            // Color attachment 2: PBR (Metallic, Roughness, AO)
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA8));
            // Depth attachment
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::Depth24Stencil8));
            
            s_GBuffer = Framebuffer::Create(desc);
        }

        if (s_QuadVAO == 0) {
            float quadVertices[] = {
                // positions        // texCoords
                -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
                 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
                 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
            };
            glGenVertexArrays(1, &s_QuadVAO);
            glGenBuffers(1, &s_QuadVBO);
            glBindVertexArray(s_QuadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }

        if (!s_LightingShader) {
            s_LightingShader = Shader::Create("res/shaders/lighting_pass.glsl");
        }

        if (!s_HDRBuffer) {
            FramebufferDescriptor hdrDesc;
            hdrDesc.width = s_ViewportWidth;
            hdrDesc.height = s_ViewportHeight;
            // 16-bit float color buffer for HDR
            hdrDesc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA16F));
            
            s_HDRBuffer = Framebuffer::Create(hdrDesc);
        }

        if (!s_FinalBuffer) {
            FramebufferDescriptor finalDesc;
            finalDesc.width = s_ViewportWidth;
            finalDesc.height = s_ViewportHeight;
            finalDesc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA8));
            
            s_FinalBuffer = Framebuffer::Create(finalDesc);
        }

        if (!s_PostProcessShader) {
            s_PostProcessShader = Shader::Create("res/shaders/post_process.glsl");
        }
    }

    void Renderer::Shutdown() {
        s_CommandQueue.Clear();
        if (s_VAO != 0) {
            glDeleteVertexArrays(1, &s_VAO);
            s_VAO = 0;
        }
        if (s_GBuffer) {
            delete s_GBuffer;
            s_GBuffer = nullptr;
        }
        if (s_HDRBuffer) {
            delete s_HDRBuffer;
            s_HDRBuffer = nullptr;
        }
        if (s_FinalBuffer) {
            delete s_FinalBuffer;
            s_FinalBuffer = nullptr;
        }
        if (s_QuadVAO != 0) {
            glDeleteVertexArrays(1, &s_QuadVAO);
            s_QuadVAO = 0;
        }
        if (s_QuadVBO != 0) {
            glDeleteBuffers(1, &s_QuadVBO);
            s_QuadVBO = 0;
        }
        if (s_LightingShader) {
            delete s_LightingShader;
            s_LightingShader = nullptr;
        }
        if (s_PostProcessShader) {
            delete s_PostProcessShader;
            s_PostProcessShader = nullptr;
        }
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
        s_ViewportWidth = width;
        s_ViewportHeight = height;
        if (s_GBuffer) {
            s_GBuffer->Resize(width, height);
        }
        if (s_HDRBuffer) {
            s_HDRBuffer->Resize(width, height);
        }
        if (s_FinalBuffer) {
            s_FinalBuffer->Resize(width, height);
        }
        SetViewport(0, 0, width, height);
    }

    void Renderer::BeginFrame() {
        s_CommandQueue.Clear();
    }

    void Renderer::EndFrame() {
        s_CommandQueue.Clear();
        s_Lights.clear();
    }

    static glm::vec3 s_CameraPosition = glm::vec3(0.0f);

    void Renderer::BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition) {
        s_ViewProjection = viewProjection;
        s_CameraPosition = cameraPosition;
    }

    void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        glViewport(x, y, width, height); // Temporary direct GL call, will abstract to backend later
    }

    void Renderer::SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void Renderer::Clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::SubmitLight(const LightData& light) {
        s_Lights.push_back(light);
    }

    void Renderer::Submit(const RenderCommand& command) {
        s_CommandQueue.Submit(command);
    }

    void Renderer::ExecuteQueue() {
        s_CommandQueue.Sort();
        
        // --- 1. Geometry Pass ---
        if (s_GBuffer) {
            s_GBuffer->Bind();
            glClearColor(0, 0, 0, 1); // Clear G-Buffer with black to avoid weird normals/pbr on empty space
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            
            for (const auto& cmd : s_CommandQueue.m_Commands) {
                if (cmd.pipeline) cmd.pipeline->Bind();
                
                if (cmd.material) {
                    cmd.material->Bind();
                    cmd.material->GetShader()->SetMat4("u_Model", cmd.transform);
                    cmd.material->GetShader()->SetMat4("u_ViewProjection", s_ViewProjection);
                }

                // Use Mesh's own VAO (has correct vertex layout)
                if (cmd.mesh) {
                    cmd.mesh->Bind();
                    glDrawElementsInstanced(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, nullptr, cmd.instanceCount);
                    cmd.mesh->Unbind();
                }
                // Legacy fallback for raw buffer commands
                else if (cmd.vertexBuffer) {
                    glBindVertexArray(s_VAO);
                    cmd.vertexBuffer->Bind();
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
                    glEnableVertexAttribArray(2);
                    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
                    if (cmd.indexBuffer) {
                        cmd.indexBuffer->Bind();
                        glDrawElementsInstanced(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, nullptr, cmd.instanceCount);
                    }
                }
            }
            s_GBuffer->Unbind();
        }

        // --- 2. Lighting Pass (to HDR Buffer) ---
        if (s_HDRBuffer && s_LightingShader && s_GBuffer) {
            s_HDRBuffer->Bind();
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Skybox color (will be tone mapped!)
            glClear(GL_COLOR_BUFFER_BIT);
            
            s_LightingShader->Bind();
            
            s_LightingShader->SetMat4("u_InverseViewProjection", glm::inverse(s_ViewProjection));
            s_LightingShader->SetFloat3("u_CameraPosition", s_CameraPosition);

            // Set Lights
            s_LightingShader->SetInt("u_LightCount", (int)s_Lights.size());
            for (size_t i = 0; i < s_Lights.size(); i++) {
                std::string prefix = "u_Lights[" + std::to_string(i) + "].";
                s_LightingShader->SetFloat3(prefix + "Position", s_Lights[i].Position);
                s_LightingShader->SetFloat3(prefix + "Direction", s_Lights[i].Direction);
                s_LightingShader->SetFloat3(prefix + "Color", s_Lights[i].Color);
                s_LightingShader->SetFloat(prefix + "Intensity", s_Lights[i].Intensity);
                s_LightingShader->SetInt(prefix + "Type", s_Lights[i].Type);
                s_LightingShader->SetFloat(prefix + "Radius", s_Lights[i].Radius);
            }

            // Bind G-Buffer textures
            s_GBuffer->GetColorAttachment(0)->Bind(0); // AlbedoSpec
            s_GBuffer->GetColorAttachment(1)->Bind(1); // Normal
            s_GBuffer->GetColorAttachment(2)->Bind(2); // PBR
            s_GBuffer->GetDepthAttachment()->Bind(3); // Depth
            
            s_LightingShader->SetInt("u_gAlbedoSpec", 0);
            s_LightingShader->SetInt("u_gNormal", 1);
            s_LightingShader->SetInt("u_gPBR", 2);
            s_LightingShader->SetInt("u_gDepth", 3);
            
            glBindVertexArray(s_QuadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            s_HDRBuffer->Unbind();
        }

        // --- 3. Post-Processing Pass (to Final Buffer) ---
        if (s_PostProcessShader && s_HDRBuffer && s_FinalBuffer) {
            s_FinalBuffer->Bind();
            glDisable(GL_DEPTH_TEST);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            s_PostProcessShader->Bind();

            s_HDRBuffer->GetColorAttachment(0)->Bind(0);
            s_PostProcessShader->SetInt("u_HDRBuffer", 0);
            s_PostProcessShader->SetFloat("u_Exposure", 1.0f); // Default exposure

            glBindVertexArray(s_QuadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            s_FinalBuffer->Unbind();
        }
    }

    void* Renderer::GetFinalColorBufferTextureID() {
        if (s_FinalBuffer) {
            return (void*)(intptr_t)s_FinalBuffer->GetColorAttachment(0)->GetRendererID();
        }
        return nullptr;
    }

    RenderCommandQueue& Renderer::GetQueue() {
        return s_CommandQueue;
    }

}
