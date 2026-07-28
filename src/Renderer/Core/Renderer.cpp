#include "Renderer.h"
#include "../../Vendor/glad.h"
#include "../Resources/Material.h"
#include "../Resources/Mesh.h"
#include "Framebuffer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "../../Helpers/DebugStats.h"
#include "../../Helpers/Logger.h"
#include "../Passes/BVHPass.h"
#include "../Passes/RTAOPass.h"
#include "../Passes/DDGIPass.h"
#include "../Passes/IBLPass.h"
#include "../Passes/TAAPass.h"
#include "../Passes/BloomPass.h"
#include "../Passes/RTShadowPass.h"
#include "../Passes/LightCullingPass.h"

namespace lgt {

    RenderCommandQueue Renderer::s_CommandQueue;

    static GLuint s_VAO = 0;
    static Framebuffer* s_GBuffer = nullptr;
    static uint32_t s_ViewportWidth = 800;
    static uint32_t s_ViewportHeight = 600;
    static glm::mat4 s_ViewProjection = glm::mat4(1.0f);
    static glm::mat4 s_PrevViewProjection = glm::mat4(1.0f);
    
    static GLuint s_QuadVAO = 0;
    static GLuint s_QuadVBO = 0;
    static Shader* s_LightingShader = nullptr;
    static Shader* s_PostProcessShader = nullptr;
    static Framebuffer* s_HDRBuffer = nullptr;
    static Framebuffer* s_FinalBuffer = nullptr;
    static std::vector<Renderer::LightData> s_Lights;
    static Buffer* s_LightDataBuffer = nullptr;

    // GPU-Driven Global Buffers
    static Buffer* s_GlobalVertexBuffer = nullptr;
    static Buffer* s_GlobalIndexBuffer = nullptr;
    static Buffer* s_GlobalMeshletBuffer = nullptr;
    static Buffer* s_GlobalInstanceBuffer = nullptr;
    static Buffer* s_GlobalIndirectDrawBuffer = nullptr;
    static Buffer* s_GlobalDrawCountBuffer = nullptr;
    static Shader* s_CullShader = nullptr;
    
    // For storing the loaded global geometry state
    static std::vector<float> s_GlobalVertices;
    static std::vector<uint32_t> s_GlobalIndices;
    static std::vector<Meshlet> s_GlobalMeshlets;
    
    static int s_FrameIndex = 0;
    static bool s_EnableRTAO = false;
    static bool s_EnableDDGI = true;
    static bool s_EnableMeshletCulling = true;
    static bool s_EnableTAA = true;
    static float s_TAABlendFactor = 0.1f;
    static bool s_EnableBloom = true;
    static float s_BloomThreshold = 1.0f;
    static float s_BloomStrength = 0.04f;
    static bool s_EnableRTShadows = true;

    void Renderer::Init() {
        if (s_VAO == 0) {
            glGenVertexArrays(1, &s_VAO);
            glBindVertexArray(s_VAO);
        }

        if (!s_GBuffer) {
            FramebufferDescriptor desc;
            desc.width = s_ViewportWidth;
            desc.height = s_ViewportHeight;
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA8));    // [0] Albedo
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA16F));  // [1] Normal
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RGBA16F));  // [2] PBR
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::RG16F));    // [3] Velocity
            desc.attachments.push_back(FramebufferAttachment(TextureFormat::Depth32F)); // Depth
            
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
            s_LightingShader = Shader::Create("res/shaders/lighting.glsl");
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

        if (!s_CullShader) {
            s_CullShader = Shader::Create("res/shaders/cull.comp");
        }

        // Initialize empty global buffers if not already created (UploadGlobalGeometry will resize them)
        if (!s_GlobalInstanceBuffer) {
            s_GlobalInstanceBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, 10000 * sizeof(InstanceData), nullptr, BufferUsage::DynamicDraw);
        }
        if (!s_GlobalIndirectDrawBuffer) {
            s_GlobalIndirectDrawBuffer = Buffer::Create(BufferType::DrawIndirectBuffer, 10000 * sizeof(DrawCommand), nullptr, BufferUsage::DynamicDraw);
        }
        if (!s_GlobalDrawCountBuffer) {
            uint32_t initialCount = 0;
            s_GlobalDrawCountBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, sizeof(uint32_t), &initialCount, BufferUsage::DynamicDraw);
        }
        
        CORE_INFO("Renderer: Calling RTAOPass::Init");
        RTAOPass::Init(s_ViewportWidth, s_ViewportHeight);
        CORE_INFO("Renderer: Calling DDGIPass::Init");
        DDGIPass::Init(glm::ivec3(8, 4, 8), glm::vec3(-10.0f, -1.0f, -10.0f), glm::vec3(2.5f, 2.0f, 2.5f));
        CORE_INFO("Renderer: Calling TAAPass::Init");
        TAAPass::Init(s_ViewportWidth, s_ViewportHeight);
        CORE_INFO("Renderer: Calling BloomPass::Init");
        BloomPass::Init(s_ViewportWidth, s_ViewportHeight);
        
        CORE_INFO("Renderer: Calling RTShadowPass::Init");
        RTShadowPass::Init(s_ViewportWidth, s_ViewportHeight);

        CORE_INFO("Renderer: Calling LightCullingPass::Init");
        LightCullingPass::Init(s_ViewportWidth, s_ViewportHeight);

        // Setup light SSBO
        s_LightDataBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, 1000 * sizeof(LightData) + 16, nullptr, BufferUsage::DynamicDraw);

        CORE_INFO("Renderer: Init Complete");
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
        if (s_CullShader) {
            delete s_CullShader;
            s_CullShader = nullptr;
        }
        if (s_GlobalVertexBuffer) { delete s_GlobalVertexBuffer; s_GlobalVertexBuffer = nullptr; }
        if (s_GlobalIndexBuffer) { delete s_GlobalIndexBuffer; s_GlobalIndexBuffer = nullptr; }
        if (s_GlobalMeshletBuffer) { delete s_GlobalMeshletBuffer; s_GlobalMeshletBuffer = nullptr; }
        if (s_GlobalInstanceBuffer) { delete s_GlobalInstanceBuffer; s_GlobalInstanceBuffer = nullptr; }
        if (s_GlobalIndirectDrawBuffer) { delete s_GlobalIndirectDrawBuffer; s_GlobalIndirectDrawBuffer = nullptr; }
        if (s_GlobalDrawCountBuffer) { delete s_GlobalDrawCountBuffer; s_GlobalDrawCountBuffer = nullptr; }
        RTAOPass::Shutdown();
        DDGIPass::Shutdown();
        TAAPass::Shutdown();
        BloomPass::Shutdown();
        RTShadowPass::Shutdown();
        LightCullingPass::Shutdown();

        s_GlobalVertices.clear();
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
        RTAOPass::Resize(width, height);
        TAAPass::Resize(width, height);
        BloomPass::Resize(width, height);
        RTShadowPass::Resize(width, height);
        LightCullingPass::Resize(width, height);
        SetViewport(0, 0, width, height);
    }

    void Renderer::BeginFrame() {
        s_CommandQueue.Clear();
        s_FrameIndex++;
    }

    void Renderer::EndFrame() {
        s_CommandQueue.Clear();
        s_Lights.clear();
    }

    static glm::vec3 s_CameraPosition = glm::vec3(0.0f);

    glm::mat4 s_ViewMatrix;
    glm::mat4 s_ProjMatrix;

    void Renderer::BeginScene(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& cameraPosition) {
        glm::vec2 jitter = GetJitter();
        glm::mat4 jitterMat = glm::translate(glm::mat4(1.0f), glm::vec3(jitter.x, jitter.y, 0.0f));
        s_ViewMatrix = viewMatrix;
        s_ProjMatrix = projMatrix;
        s_ViewProjection = jitterMat * projMatrix * viewMatrix;
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
        
        bool useGlobalBuffers = s_GlobalMeshletBuffer && s_CullShader;

        // --- 0. Prepare Indirect Draw Data ---
        if (useGlobalBuffers) {
            std::vector<InstanceData> instances;
            instances.reserve(s_CommandQueue.m_Commands.size());

            std::unordered_map<Mesh*, uint32_t> meshOffsets;
            s_GlobalMeshlets.clear();

            for (const auto& cmd : s_CommandQueue.m_Commands) {
                if (cmd.mesh) {
                    if (meshOffsets.find(cmd.mesh) == meshOffsets.end()) {
                        meshOffsets[cmd.mesh] = static_cast<uint32_t>(s_GlobalMeshlets.size());
                        s_GlobalMeshlets.insert(s_GlobalMeshlets.end(), cmd.mesh->GetMeshlets().begin(), cmd.mesh->GetMeshlets().end());
                    }

                    InstanceData inst;
                    inst.Transform = cmd.transform;
                    inst.meshletInfo.x = meshOffsets[cmd.mesh];          // firstMeshlet
                    inst.meshletInfo.y = static_cast<uint32_t>(cmd.mesh->GetMeshlets().size()); // meshletCount
                    inst.meshletInfo.z = 0;
                    inst.meshletInfo.w = 0;
                    
                    if (cmd.material) {
                        inst.color = glm::vec4(cmd.material->Albedo, 1.0f);
                        inst.pbr = glm::vec4(cmd.material->Metallic, cmd.material->Roughness, 1.0f, 0.0f);
                        
                        inst.albedoMapHandle = cmd.material->AlbedoMap ? cmd.material->AlbedoMap->GetBindlessHandle() : 0;
                        inst.normalMapHandle = cmd.material->NormalMap ? cmd.material->NormalMap->GetBindlessHandle() : 0;
                        inst.pbrMapHandle = cmd.material->MetallicMap ? cmd.material->MetallicMap->GetBindlessHandle() : 0;
                    } else {
                        inst.color = glm::vec4(1.0f);
                        inst.pbr = glm::vec4(0.0f, 0.5f, 1.0f, 0.0f);
                        
                        inst.albedoMapHandle = 0;
                        inst.normalMapHandle = 0;
                        inst.pbrMapHandle = 0;
                    }
                    inst.padding = 0;
                    
                    instances.push_back(inst);
                }
            }

            if (!instances.empty()) {
                if (!s_GlobalMeshlets.empty()) {
                    // Recreate buffer if size exceeded
                    if (s_GlobalMeshlets.size() * sizeof(Meshlet) > s_GlobalMeshletBuffer->GetSize()) {
                        delete s_GlobalMeshletBuffer;
                        s_GlobalMeshletBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, s_GlobalMeshlets.size() * sizeof(Meshlet), s_GlobalMeshlets.data());
                    } else {
                        s_GlobalMeshletBuffer->SetData(s_GlobalMeshlets.data(), s_GlobalMeshlets.size() * sizeof(Meshlet), 0);
                    }
                }
                
                if (instances.size() * sizeof(InstanceData) > s_GlobalInstanceBuffer->GetSize()) {
                    delete s_GlobalInstanceBuffer;
                    s_GlobalInstanceBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, instances.size() * sizeof(InstanceData) * 2, instances.data(), BufferUsage::DynamicDraw);
                } else {
                    s_GlobalInstanceBuffer->SetData(instances.data(), instances.size() * sizeof(InstanceData), 0);
                }
                
                uint32_t zero = 0;
                s_GlobalDrawCountBuffer->SetData(&zero, sizeof(uint32_t), 0);

                s_CullShader->Bind();

                glm::mat4 vp = s_ViewProjection;
                glm::vec4 planes[6];
                planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]); // Left
                planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]); // Right
                planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]); // Bottom
                planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]); // Top
                planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]); // Near
                planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]); // Far

                for (int i = 0; i < 6; i++) {
                    float len = glm::length(glm::vec3(planes[i]));
                    planes[i] /= len;
                    s_CullShader->SetFloat4("u_FrustumPlanes[" + std::to_string(i) + "]", planes[i]);
                    
                    lgt::DebugStats::Report("Frustum Plane " + std::to_string(i), std::to_string(planes[i].x) + ", " + std::to_string(planes[i].y) + ", " + std::to_string(planes[i].z) + "  w:" + std::to_string(planes[i].w));
                }

                s_CullShader->SetUInt("u_InstanceCount", (uint32_t)instances.size());
                s_CullShader->SetInt("u_EnableCulling", s_EnableMeshletCulling ? 1 : 0);

                s_GlobalMeshletBuffer->BindBase(0);
                s_GlobalInstanceBuffer->BindBase(1);
                s_GlobalIndirectDrawBuffer->BindBase(2);
                s_GlobalDrawCountBuffer->BindBase(3);

                glDispatchCompute((instances.size() + 63) / 64, 1, 1);
                glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

                // Read back GPU indirect draw count for Debug Stats
                uint32_t culledCount = 0;
                glGetNamedBufferSubData(s_GlobalDrawCountBuffer->GetRendererID(), 0, sizeof(uint32_t), &culledCount);

                lgt::DebugStats::Report("Instance Count", instances.size());
                lgt::DebugStats::Report("Draw Commands Count", culledCount);
                lgt::DebugStats::Report("Meshlets Passing Culling", std::to_string(culledCount) + " / " + std::to_string(s_GlobalMeshlets.size()));
            }
        }

        // --- 1. Geometry Pass ---
        if (s_GBuffer) {
            s_GBuffer->Bind();
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            
            if (useGlobalBuffers && !s_CommandQueue.m_Commands.empty() && s_CommandQueue.m_Commands[0].material) {
                auto material = s_CommandQueue.m_Commands[0].material;
                material->Bind();
                material->GetShader()->SetMat4("u_ViewProjection", s_ViewProjection);
                material->GetShader()->SetMat4("u_PrevViewProjection", s_PrevViewProjection);
                // Bind dummy VAO just so OpenGL doesn't complain
                glBindVertexArray(s_VAO);
                
                // Bind global SSBOs for bindless vertex pulling
                s_GlobalVertexBuffer->BindBase(4);
                s_GlobalInstanceBuffer->BindBase(5);
                
                s_GlobalIndexBuffer->Bind();
                s_GlobalIndirectDrawBuffer->Bind();
                
                // Bind count buffer to GL_PARAMETER_BUFFER
                glBindBuffer(GL_PARAMETER_BUFFER, s_GlobalDrawCountBuffer->GetRendererID());
                
                glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 0, 10000, 0); // Need OpenGL 4.6
                
                glBindBuffer(GL_PARAMETER_BUFFER, 0);
                s_GlobalIndirectDrawBuffer->Unbind();
                s_GlobalIndexBuffer->Unbind();
            } else {
                // Legacy loop
                for (const auto& cmd : s_CommandQueue.m_Commands) {
                    if (cmd.pipeline) cmd.pipeline->Bind();
                    
                    if (cmd.material) {
                        cmd.material->Bind();
                        cmd.material->GetShader()->SetMat4("u_Model", cmd.transform);
                        cmd.material->GetShader()->SetMat4("u_ViewProjection", s_ViewProjection);
                        cmd.material->GetShader()->SetMat4("u_PrevViewProjection", s_PrevViewProjection);
                    }

                    if (cmd.mesh) {
                        cmd.mesh->Bind();
                        glDrawElementsInstanced(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, nullptr, cmd.instanceCount);
                        cmd.mesh->Unbind();
                    }
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
            }
            s_GBuffer->Unbind();
        }

        // --- 1.5. RTAO + DDGI Passes ---
        if (s_GBuffer) {
            if (s_EnableRTAO) {
                glm::mat4 invVP = glm::inverse(s_ViewProjection);
                RTAOPass::Execute(
                    s_GBuffer->GetDepthAttachment()->GetRendererID(),
                    s_GBuffer->GetColorAttachment(1)->GetRendererID(),
                    2.0f, 8, s_FrameIndex, invVP);
            }

            if (s_EnableDDGI) {
                glm::vec3 sunDir(0, -1, 0), sunColor(1.0f);
                float sunIntensity = 1.0f;
                for (const auto& l : s_Lights) {
                    if (l.Type == 0) {
                        sunDir = l.Direction;
                        sunColor = l.Color;
                        sunIntensity = l.Intensity;
                        break;
                    }
                }
                DDGIPass::Execute(sunDir, sunColor, sunIntensity, s_FrameIndex);
            }

            if (s_EnableRTShadows) {
                glm::vec3 sunDir(0, -1, 0);
                for (const auto& l : s_Lights) {
                    if (l.Type == 0) {
                        sunDir = l.Direction;
                        break;
                    }
                }
                RTShadowPass::Execute(
                    s_GBuffer->GetDepthAttachment()->GetRendererID(),
                    s_GBuffer->GetColorAttachment(1)->GetRendererID(),
                    glm::inverse(s_ViewProjection), s_CameraPosition, sunDir
                );
            }
        }

        // --- 1.8. Clustered Light Culling ---
        // Upload lights to SSBO
        if (s_LightDataBuffer && !s_Lights.empty()) {
            // Need to match std430 layout of LightData
            struct alignas(16) Std430Light {
                glm::vec4 Position;
                glm::vec4 Color;
                int Type;
                float Intensity;
                float Radius;
                float Falloff;
                glm::vec4 Direction;
            };

            std::vector<Std430Light> stdLights(s_Lights.size());
            for (size_t i = 0; i < s_Lights.size(); i++) {
                stdLights[i].Position = glm::vec4(s_Lights[i].Position, 1.0f);
                stdLights[i].Color = glm::vec4(s_Lights[i].Color, 1.0f);
                stdLights[i].Type = s_Lights[i].Type;
                stdLights[i].Intensity = s_Lights[i].Intensity;
                stdLights[i].Radius = s_Lights[i].Radius;
                stdLights[i].Falloff = 1.0f;
                stdLights[i].Direction = glm::vec4(s_Lights[i].Direction, 0.0f);
            }
            s_LightDataBuffer->SetData(stdLights.data(), stdLights.size() * sizeof(Std430Light));
            s_LightDataBuffer->BindBase(1); // Binding 1 for lights

            LightCullingPass::Execute(s_ViewMatrix, s_ProjMatrix, glm::inverse(s_ProjMatrix), (uint32_t)s_Lights.size());
        }

        // --- 2. Lighting Pass (to HDR Buffer) ---
        if (s_HDRBuffer && s_LightingShader && s_GBuffer) {
            s_HDRBuffer->Bind();
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Skybox color (will be tone mapped!)
            glClear(GL_COLOR_BUFFER_BIT);
            
            s_LightingShader->Bind();
            
            s_LightingShader->SetMat4("u_InvViewProjection", glm::inverse(s_ViewProjection));
            s_LightingShader->SetFloat3("u_CameraPos", s_CameraPosition);
            s_LightingShader->SetMat4("u_ViewMatrix", s_ViewMatrix);
            s_LightingShader->SetInt3("u_GridSize", LightCullingPass::GetGridSize());
            s_LightingShader->SetFloat("u_ZNear", 0.1f);
            s_LightingShader->SetFloat("u_ZFar", 1000.0f);

            // Bind G-Buffer textures
            s_GBuffer->GetColorAttachment(0)->Bind(0); // AlbedoSpec
            s_GBuffer->GetColorAttachment(1)->Bind(1); // Normal
            s_GBuffer->GetColorAttachment(2)->Bind(2); // PBR
            s_GBuffer->GetDepthAttachment()->Bind(3); // Depth
            
            s_LightingShader->SetInt("u_gAlbedo", 0);
            s_LightingShader->SetInt("u_gNormal", 1);
            s_LightingShader->SetInt("u_gPBR", 2);
            s_LightingShader->SetInt("u_gDepth", 3);

            if (IBLPass::IsReady()) {
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_CUBE_MAP, IBLPass::GetIrradianceMapID());
                s_LightingShader->SetInt("u_IrradianceMap", 4);

                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_CUBE_MAP, IBLPass::GetPrefilterMapID());
                s_LightingShader->SetInt("u_PrefilterMap", 5);

                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, IBLPass::GetBrdfLutID());
                s_LightingShader->SetInt("u_BrdfLut", 6);
            }
            
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, RTAOPass::GetAOTextureID());
            s_LightingShader->SetInt("u_AOTexture", 7);
            s_LightingShader->SetInt("u_EnableRTAO", s_EnableRTAO ? 1 : 0);

            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, DDGIPass::GetIrradianceAtlasID());
            s_LightingShader->SetInt("u_DDGIIrradiance", 8);
            s_LightingShader->SetInt("u_EnableDDGI", s_EnableDDGI ? 1 : 0);

            s_LightingShader->SetInt("u_EnableRTShadows", s_EnableRTShadows ? 1 : 0);
            if (s_EnableRTShadows) {
                glActiveTexture(GL_TEXTURE9);
                glBindTexture(GL_TEXTURE_2D, RTShadowPass::GetShadowMaskTextureID());
                s_LightingShader->SetInt("u_ShadowMask", 9);
            }

            s_LightingShader->SetInt3("u_DDGIProbeGridSize", glm::ivec3(DDGIPass::GetGridSize().x, DDGIPass::GetGridSize().y, DDGIPass::GetGridSize().z));
            s_LightingShader->SetFloat3("u_DDGIProbeOrigin", DDGIPass::GetProbeOrigin());
            s_LightingShader->SetFloat3("u_DDGIProbeSpacing", DDGIPass::GetProbeSpacing());
            
            glBindVertexArray(s_QuadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            s_HDRBuffer->Unbind();
        }

        // --- 2.5 TAA and Bloom ---
        uint32_t colorTex = s_HDRBuffer->GetColorAttachment(0)->GetRendererID();
        uint32_t resolvedTex = colorTex;
        
        if (s_EnableTAA) {
            TAAPass::Execute(colorTex, 
                             s_GBuffer->GetColorAttachment(3)->GetRendererID(), 
                             s_GBuffer->GetDepthAttachment()->GetRendererID(), 
                             s_FrameIndex, s_TAABlendFactor);
            resolvedTex = TAAPass::GetResolvedTextureID();
        }
        
        uint32_t bloomTex = 0;
        if (s_EnableBloom) {
            BloomPass::Execute(resolvedTex, s_BloomThreshold, s_BloomStrength);
            bloomTex = BloomPass::GetBloomTextureID();
        }

        // --- 3. Post-Processing Pass (to Final Buffer) ---
        if (s_PostProcessShader && s_HDRBuffer && s_FinalBuffer) {
            s_FinalBuffer->Bind();
            glDisable(GL_DEPTH_TEST);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            s_PostProcessShader->Bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, resolvedTex);
            s_PostProcessShader->SetInt("u_HDRBuffer", 0);
            
            s_PostProcessShader->SetInt("u_EnableBloom", s_EnableBloom ? 1 : 0);
            if (s_EnableBloom) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, bloomTex);
                s_PostProcessShader->SetInt("u_BloomBuffer", 1);
            }
            
            s_PostProcessShader->SetFloat("u_Exposure", 1.0f); // Default exposure

            glBindVertexArray(s_QuadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            s_FinalBuffer->Unbind();
        }

        s_PrevViewProjection = s_ViewProjection;
    }

    void* Renderer::GetFinalColorBufferTextureID() {
        if (s_FinalBuffer) {
            return (void*)(intptr_t)s_FinalBuffer->GetColorAttachment(0)->GetRendererID();
        }
        return nullptr;
    }

    int Renderer::GetFrameIndex() {
        return s_FrameIndex;
    }

    glm::vec2 Renderer::GetJitter() {
        return TAAPass::GetJitter(s_FrameIndex);
    }

    void Renderer::Present() {
        if (!s_FinalBuffer) return;
        uint32_t fboID = s_FinalBuffer->GetRendererID();
        uint32_t width = s_FinalBuffer->GetWidth();
        uint32_t height = s_FinalBuffer->GetHeight();
        
        glBlitNamedFramebuffer(fboID, 0, 
                               0, 0, width, height, 
                               0, 0, width, height, 
                               GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    RenderCommandQueue& Renderer::GetQueue() {
        return s_CommandQueue;
    }

    void Renderer::UploadGlobalGeometry(const std::vector<float>& vertices, const std::vector<uint32_t>& indices, const std::vector<Meshlet>& meshlets) {
        s_GlobalVertices = vertices;
        s_GlobalIndices = indices;
        s_GlobalMeshlets = meshlets;

        if (s_GlobalVertexBuffer) delete s_GlobalVertexBuffer;
        if (s_GlobalIndexBuffer) delete s_GlobalIndexBuffer;
        if (s_GlobalMeshletBuffer) delete s_GlobalMeshletBuffer;

        s_GlobalVertexBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, vertices.size() * sizeof(float), vertices.data(), BufferUsage::StaticCopy);
        s_GlobalIndexBuffer = Buffer::Create(BufferType::IndexBuffer, indices.size() * sizeof(uint32_t), indices.data(), BufferUsage::StaticCopy);
        s_GlobalMeshletBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, meshlets.size() * sizeof(Meshlet), meshlets.data(), BufferUsage::StaticCopy);

        BVHPass::Build(vertices, indices);
    }

    void Renderer::SetRTAOEnabled(bool enabled) {
        s_EnableRTAO = enabled;
    }

    bool Renderer::IsRTAOEnabled() {
        return s_EnableRTAO;
    }

    void Renderer::SetDDGIEnabled(bool enabled) {
        s_EnableDDGI = enabled;
    }

    bool Renderer::IsDDGIEnabled() {
        return s_EnableDDGI;
    }

    void Renderer::SetMeshletCullingEnabled(bool enabled) {
        s_EnableMeshletCulling = enabled;
    }

    bool Renderer::IsMeshletCullingEnabled() {
        return s_EnableMeshletCulling;
    }

    void Renderer::SetTAAEnabled(bool enabled) { s_EnableTAA = enabled; }
    bool Renderer::IsTAAEnabled() { return s_EnableTAA; }
    void Renderer::SetTAABlendFactor(float factor) { s_TAABlendFactor = factor; }
    float Renderer::GetTAABlendFactor() { return s_TAABlendFactor; }

    void Renderer::SetBloomEnabled(bool enabled) { s_EnableBloom = enabled; }
    bool Renderer::IsBloomEnabled() { return s_EnableBloom; }
    void Renderer::SetBloomThreshold(float threshold) { s_BloomThreshold = threshold; }
    float Renderer::GetBloomThreshold() { return s_BloomThreshold; }
    void Renderer::SetBloomStrength(float strength) { s_BloomStrength = strength; }
    float Renderer::GetBloomStrength() { return s_BloomStrength; }

    void Renderer::SetRTShadowsEnabled(bool enabled) { s_EnableRTShadows = enabled; }
    bool Renderer::IsRTShadowsEnabled() { return s_EnableRTShadows; }

}
