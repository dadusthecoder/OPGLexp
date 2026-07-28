#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
out vec3 v_LocalPos;
uniform mat4 u_ViewProjection;

void main() {
    v_LocalPos = a_Position;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

#include "include/common.glsl"

in vec3 v_LocalPos;
out vec4 o_Color;

uniform sampler2D u_EquirectMap;

void main() {
    vec3 v = normalize(v_LocalPos);
    vec2 uv = vec2(atan(v.z, v.x) / TWO_PI + 0.5, asin(v.y) / PI + 0.5);
    vec3 color = texture(u_EquirectMap, uv).rgb;
    o_Color = vec4(color, 1.0);
}
