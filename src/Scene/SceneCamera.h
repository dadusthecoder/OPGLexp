#pragma once
#include "../Renderer/Core/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace lgt {

    class SceneCamera : public Camera {
    public:
        enum class ProjectionType { Perspective = 0, Orthographic = 1 };

        SceneCamera() { RecalculateProjection(); }
        virtual ~SceneCamera() = default;

        void SetPerspective(float verticalFOV, float nearClip, float farClip) {
            m_ProjectionType = ProjectionType::Perspective;
            m_PerspectiveFOV = verticalFOV;
            m_PerspectiveNear = nearClip;
            m_PerspectiveFar = farClip;
            RecalculateProjection();
        }

        float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
        float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
        float GetPerspectiveFarClip() const { return m_PerspectiveFar; }

        void SetOrthographic(float size, float nearClip, float farClip) {
            m_ProjectionType = ProjectionType::Orthographic;
            m_OrthographicSize = size;
            m_OrthographicNear = nearClip;
            m_OrthographicFar = farClip;
            RecalculateProjection();
        }

        void SetViewportSize(uint32_t width, uint32_t height) {
            if (width > 0 && height > 0) {
                m_AspectRatio = (float)width / (float)height;
                RecalculateProjection();
            }
        }

        ProjectionType GetProjectionType() const { return m_ProjectionType; }
        void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

    private:
        void RecalculateProjection() {
            if (m_ProjectionType == ProjectionType::Perspective) {
                m_Projection = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
            } else {
                float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
                float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
                float orthoBottom = -m_OrthographicSize * 0.5f;
                float orthoTop = m_OrthographicSize * 0.5f;
                m_Projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
            }
        }

    private:
        ProjectionType m_ProjectionType = ProjectionType::Perspective;

        float m_PerspectiveFOV = glm::radians(45.0f);
        float m_PerspectiveNear = 0.1f, m_PerspectiveFar = 1000.0f;

        float m_OrthographicSize = 10.0f;
        float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;

        float m_AspectRatio = 800.0f / 600.0f;
    };
}
