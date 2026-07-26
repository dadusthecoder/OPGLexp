#pragma once

#include <stdint.h>

namespace lgt {

    enum class DepthCompareOp {
        Never = 0,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class CullMode {
        None = 0,
        Front,
        Back,
        FrontAndBack
    };

    enum class BlendFactor {
        Zero = 0,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        SrcAlphaSaturate
    };

    enum class PolygonMode {
        Fill = 0,
        Line,
        Point
    };

    struct PipelineState {
        bool depthTest = true;
        bool depthWrite = true;
        DepthCompareOp depthFunc = DepthCompareOp::Less;
        
        bool blendEnable = false;
        BlendFactor srcBlend = BlendFactor::SrcAlpha;
        BlendFactor dstBlend = BlendFactor::OneMinusSrcAlpha;
        
        CullMode cullMode = CullMode::Back;
        PolygonMode polygonMode = PolygonMode::Fill;
    };

    class Pipeline {
    public:
        virtual ~Pipeline() = default;

        virtual void Bind() const = 0;
        virtual const PipelineState& GetState() const = 0;

        static Pipeline* Create(const PipelineState& state);
    };

}
