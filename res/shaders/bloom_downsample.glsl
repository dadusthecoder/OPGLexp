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
uniform vec2      u_SrcTexelSize;  // 1/resolution
uniform float     u_Threshold;
uniform int       u_MipLevel;

void main() {
    vec2 uv = v_TexCoord;
    float x = u_SrcTexelSize.x;
    float y = u_SrcTexelSize.y;
    // 13-tap Jimenez 2014 downsample
    vec3 a = texture(u_Source, vec2(uv.x-2*x, uv.y+2*y)).rgb;
    vec3 b = texture(u_Source, vec2(uv.x,     uv.y+2*y)).rgb;
    vec3 c = texture(u_Source, vec2(uv.x+2*x, uv.y+2*y)).rgb;
    vec3 d = texture(u_Source, vec2(uv.x-2*x, uv.y    )).rgb;
    vec3 e = texture(u_Source, vec2(uv.x,     uv.y    )).rgb;
    vec3 f = texture(u_Source, vec2(uv.x+2*x, uv.y    )).rgb;
    vec3 g = texture(u_Source, vec2(uv.x-2*x, uv.y-2*y)).rgb;
    vec3 h = texture(u_Source, vec2(uv.x,     uv.y-2*y)).rgb;
    vec3 i = texture(u_Source, vec2(uv.x+2*x, uv.y-2*y)).rgb;
    vec3 j = texture(u_Source, vec2(uv.x-x,   uv.y+y  )).rgb;
    vec3 k = texture(u_Source, vec2(uv.x+x,   uv.y+y  )).rgb;
    vec3 l = texture(u_Source, vec2(uv.x-x,   uv.y-y  )).rgb;
    vec3 m = texture(u_Source, vec2(uv.x+x,   uv.y-y  )).rgb;
    vec3 result = e*0.125 + (a+c+g+i)*0.03125 + (b+d+f+h)*0.0625 + (j+k+l+m)*0.125;
    
    if (u_MipLevel == 0) {
        float brightness = max(result.r, max(result.g, result.b));
        float contribution = max(0.0, brightness - u_Threshold);
        contribution /= max(brightness, 0.00001);
        result *= contribution;
    }
    
    o_Color = vec4(result, 1.0);
}
