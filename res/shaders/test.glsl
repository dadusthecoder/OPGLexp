#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_Model;
uniform mat4 u_ViewProjection;

void main() {
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

layout(location = 0) out vec4 o_Color;

struct Material {
    vec3 Albedo;
    float Metallic;
    float Roughness;
};

uniform Material u_Material;

void main() {
    // Simple solid color for now to prove Material binds
    // We add a tiny offset from metallic and roughness just so the compiler doesn't optimize them out
    vec3 color = u_Material.Albedo + (u_Material.Metallic * 0.0001) + (u_Material.Roughness * 0.0001);
    o_Color = vec4(color, 1.0);
}
