#shader Vertex
#version 450 core

struct Light {
    vec4 position;
    vec4 color;
    vec4 direction;
    vec4 params;
};

layout(std430, binding = 1) readonly buffer LightBuffer {
    Light lights[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec4 v_Color;
out vec2 v_TexCoords;
flat out int v_LightType;

void main() {
    int lightIndex = gl_VertexID / 6;
    int vertexIndex = gl_VertexID % 6;
    Light light = lights[lightIndex];
    
    vec3 right = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 up = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);
    
    // Scale gizmo slightly based on radius for visual feedback, but keep it small
    float size = clamp(light.direction.w * 0.05, 0.2, 1.0); 
    
    vec3 center = light.position.xyz;
    
    vec3 positions[6];
    vec2 uvs[6];
    
    // Triangle 1
    positions[0] = center - right * size - up * size; uvs[0] = vec2(0.0, 0.0); // Bottom-left
    positions[1] = center + right * size - up * size; uvs[1] = vec2(1.0, 0.0); // Bottom-right
    positions[2] = center - right * size + up * size; uvs[2] = vec2(0.0, 1.0); // Top-left
    
    // Triangle 2
    positions[3] = center + right * size - up * size; uvs[3] = vec2(1.0, 0.0); // Bottom-right
    positions[4] = center + right * size + up * size; uvs[4] = vec2(1.0, 1.0); // Top-right
    positions[5] = center - right * size + up * size; uvs[5] = vec2(0.0, 1.0); // Top-left

    v_Color = light.color;
    v_TexCoords = uvs[vertexIndex];
    v_LightType = int(light.position.w);
    gl_Position = u_Projection * u_View * vec4(positions[vertexIndex], 1.0);
}

#shader Fragment
#version 450 core

in vec4 v_Color;
in vec2 v_TexCoords;
flat in int v_LightType;
out vec4 FragColor;

uniform sampler2D u_PointLightIcon;
uniform sampler2D u_DirectionalLightIcon;

void main() {
    vec4 texColor;
    if (v_LightType == 1) {
        texColor = texture(u_DirectionalLightIcon, v_TexCoords);
    } else {
        texColor = texture(u_PointLightIcon, v_TexCoords);
    }
    
    if (texColor.r < 0.1) {
        discard;
    }
    
    // Use the texture color (it's white, so we multiply by light color)
    FragColor = vec4(v_Color.rgb * texColor.rgb, 1.0);
}
