#type vertex
#version 460 core
out vec2 v_TexCoord;
void main() {
    vec2 pos[3] = vec2[](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
    vec2 uvs[3] = vec2[](vec2(0,0),  vec2(2,0),  vec2(0,2));
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    v_TexCoord  = uvs[gl_VertexID];
}

#type fragment
#version 460 core
in  vec2 v_TexCoord;
out vec4 o_Color;

uniform sampler2D u_Source;
uniform vec2      u_FilterRadius;
uniform float     u_Strength;  // bloom blend strength

void main() {
    vec2 uv = v_TexCoord;
    float x = u_FilterRadius.x;
    float y = u_FilterRadius.y;
    // 9-tap tent filter upsample
    vec3 a = texture(u_Source, vec2(uv.x-x, uv.y+y)).rgb;
    vec3 b = texture(u_Source, vec2(uv.x,   uv.y+y)).rgb;
    vec3 c = texture(u_Source, vec2(uv.x+x, uv.y+y)).rgb;
    vec3 d = texture(u_Source, vec2(uv.x-x, uv.y  )).rgb;
    vec3 e = texture(u_Source, vec2(uv.x,   uv.y  )).rgb;
    vec3 f = texture(u_Source, vec2(uv.x+x, uv.y  )).rgb;
    vec3 g = texture(u_Source, vec2(uv.x-x, uv.y-y)).rgb;
    vec3 h = texture(u_Source, vec2(uv.x,   uv.y-y)).rgb;
    vec3 i = texture(u_Source, vec2(uv.x+x, uv.y-y)).rgb;
    vec3 result = (a+c+g+i + 2.0*(b+d+f+h) + 4.0*e) / 16.0;
    o_Color = vec4(result * u_Strength, 1.0);
}
