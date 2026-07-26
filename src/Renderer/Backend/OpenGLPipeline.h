#pragma once
#include "../Core/Pipeline.h"

namespace lgt {

    class OpenGLPipeline : public Pipeline {
    public:
        OpenGLPipeline(const PipelineState& state);
        virtual ~OpenGLPipeline() = default;

        virtual void Bind() const override;
        virtual const PipelineState& GetState() const override { return m_State; }

    private:
        PipelineState m_State;
    };

}
