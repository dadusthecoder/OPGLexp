#shader Vertex
#version 460 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_View;
uniform mat4 u_Projection;

// DDGI volume config
uniform vec3  u_DDGIOrigin;
uniform vec3  u_DDGISpacing;
uniform ivec3 u_DDGIProbeCount;

// Instance index determines which probe we are rendering
out vec3 v_ProbePosition;
out flat int v_ProbeLinearIndex;
out vec3 v_Normal;

void main() {
    int linearIndex = gl_InstanceID;
    
    // Decode linear index to 3D grid coord
    int x = linearIndex % u_DDGIProbeCount.x;
    int y = (linearIndex / u_DDGIProbeCount.x) % u_DDGIProbeCount.y;
    int z = linearIndex / (u_DDGIProbeCount.x * u_DDGIProbeCount.y);
    
    vec3 probePos = u_DDGIOrigin + vec3(x, y, z) * u_DDGISpacing;
    
    // Scale down the sphere
    float sphereRadius = 0.2;
    vec3 worldPos = probePos + a_Position * sphereRadius;
    
    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);
    
    v_ProbePosition = probePos;
    v_ProbeLinearIndex = linearIndex;
    v_Normal = normalize(a_Position);
}

#shader Fragment
#version 460 core

in vec3 v_ProbePosition;
in flat int v_ProbeLinearIndex;
in vec3 v_Normal;

out vec4 FragColor;

// DDGI Atlas
layout(binding = 0) uniform sampler2D u_IrradianceAtlas;

uniform ivec3 u_DDGIProbeCount;
const int irradianceTexSize = 8;
const int border = 1;
const int probeTexSize = irradianceTexSize + 2 * border;

vec2 signNotZero(vec2 v) {
    return vec2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
}

vec2 OctahedralEncode(vec3 v) {
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0) {
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    }
    return result;
}

void main() {
    int linearIdx = v_ProbeLinearIndex;
    
    int atlasX = (linearIdx % u_DDGIProbeCount.x) * probeTexSize + border;
    int atlasY = (linearIdx / u_DDGIProbeCount.x) * probeTexSize + border;
    
    vec2 octCoord = OctahedralEncode(normalize(v_Normal));
    vec2 probeUV = (octCoord * 0.5 + 0.5) * float(irradianceTexSize);
    
    vec2 atlasSize = vec2(textureSize(u_IrradianceAtlas, 0));
    vec2 uv = (vec2(atlasX, atlasY) + probeUV) / atlasSize;
    
    vec3 irradiance = texture(u_IrradianceAtlas, uv).rgb;
    
    // Tone mapping for display
    irradiance = irradiance / (irradiance + vec3(1.0));
    
    FragColor = vec4(pow(irradiance, vec3(1.0/2.2)), 1.0);
}
