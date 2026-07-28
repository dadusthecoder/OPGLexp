#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;

out vec3 v_LocalPos;

uniform mat4 u_ViewProjection;

void main() {
    v_LocalPos = a_Position;
    // Remove translation by setting w to 1 after rotation
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

#include "include/common.glsl"

in vec3 v_LocalPos;
uniform samplerCube u_EnvMap;
out vec4 o_Color;

void main() {
    vec3 N = normalize(v_LocalPos);
    vec3 irradiance = vec3(0.0);
    vec3 up = vec3(0,1,0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);
    
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < TWO_PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            irradiance += texture(u_EnvMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    o_Color = vec4(PI * irradiance / nrSamples, 1.0);
}
