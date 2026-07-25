#pragma once
#include "AccelerationStructure.h"
#include "BlueNoise.h"

namespace lgt {

class RayTracer {
public:
    void Init() { m_initialized = true; }
    void Shutdown() { m_initialized = false; }
    
    void BindForTrace(const AccelerationStructure* accel, const BlueNoise& noise) const {
        if (accel) accel->Bind();
        noise.Bind(12, 13);  // texture units 12, 13 for noise
    }
    
    void UnbindAfterTrace(const AccelerationStructure* accel) const {
        if (accel) accel->Unbind();
    }
    
    bool IsReady() const { return m_initialized; }
    
private:
    bool m_initialized = false;
};

} // namespace lgt
