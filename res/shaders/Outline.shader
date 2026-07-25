#shader Vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_Model;

void main() {
    // Displace along surface normal for correct outlines on any mesh
    vec3 scaledPos = a_Position + a_Normal * 0.02;
    gl_Position = u_Projection * u_View * u_Model * vec4(scaledPos, 1.0);
}

#shader Fragment
#version 450 core

out vec4 FragColor;

void main() {
    // Unity/Unreal orange outline
    FragColor = vec4(1.0, 0.5, 0.0, 1.0);
}
