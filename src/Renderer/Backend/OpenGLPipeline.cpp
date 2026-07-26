#include "OpenGLPipeline.h"
#include "../../Vendor/glad.h"

namespace lgt {

    static GLenum DepthCompareOpToGL(DepthCompareOp op) {
        switch (op) {
            case DepthCompareOp::Never: return GL_NEVER;
            case DepthCompareOp::Less: return GL_LESS;
            case DepthCompareOp::Equal: return GL_EQUAL;
            case DepthCompareOp::LessOrEqual: return GL_LEQUAL;
            case DepthCompareOp::Greater: return GL_GREATER;
            case DepthCompareOp::NotEqual: return GL_NOTEQUAL;
            case DepthCompareOp::GreaterOrEqual: return GL_GEQUAL;
            case DepthCompareOp::Always: return GL_ALWAYS;
        }
        return GL_LESS;
    }

    static GLenum BlendFactorToGL(BlendFactor factor) {
        switch (factor) {
            case BlendFactor::Zero: return GL_ZERO;
            case BlendFactor::One: return GL_ONE;
            case BlendFactor::SrcColor: return GL_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor: return GL_DST_COLOR;
            case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
            case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha: return GL_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
            case BlendFactor::SrcAlphaSaturate: return GL_SRC_ALPHA_SATURATE;
        }
        return GL_ZERO;
    }

    OpenGLPipeline::OpenGLPipeline(const PipelineState& state)
        : m_State(state) {
    }

    void OpenGLPipeline::Bind() const {
        if (m_State.depthTest) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(DepthCompareOpToGL(m_State.depthFunc));
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        glDepthMask(m_State.depthWrite ? GL_TRUE : GL_FALSE);

        if (m_State.blendEnable) {
            glEnable(GL_BLEND);
            glBlendFunc(BlendFactorToGL(m_State.srcBlend), BlendFactorToGL(m_State.dstBlend));
        } else {
            glDisable(GL_BLEND);
        }

        if (m_State.cullMode == CullMode::None) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            if (m_State.cullMode == CullMode::Front) glCullFace(GL_FRONT);
            else if (m_State.cullMode == CullMode::Back) glCullFace(GL_BACK);
            else if (m_State.cullMode == CullMode::FrontAndBack) glCullFace(GL_FRONT_AND_BACK);
        }

        if (m_State.polygonMode == PolygonMode::Fill) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else if (m_State.polygonMode == PolygonMode::Line) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else if (m_State.polygonMode == PolygonMode::Point) glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    Pipeline* Pipeline::Create(const PipelineState& state) {
        return new OpenGLPipeline(state);
    }

}
