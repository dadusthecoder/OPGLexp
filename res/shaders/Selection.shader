#shader Vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_Model;

void main() {
    gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0);
}

#shader Fragment
#version 450 core

uniform int u_EntityID;

layout(location = 0) out int FragColor;

void main() {
    FragColor = u_EntityID;
}
