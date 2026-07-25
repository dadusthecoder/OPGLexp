#shader Vertex
#version 460 core
layout(location = 0) in vec3 pos;

out vec3 localPos;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    localPos = pos;
    vec4 clipPos = u_Projection * mat4(mat3(u_View)) * vec4(localPos, 1.0);
    gl_Position = clipPos.xyww;
}

#shader Fragment
#version 460 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube u_EnvironmentMap;
uniform float u_Exposure;
uniform float u_Lod;

void main()
{
    vec3 color = textureLod(u_EnvironmentMap, localPos, u_Lod).rgb * u_Exposure;
    FragColor = vec4(color, 1.0);
}
