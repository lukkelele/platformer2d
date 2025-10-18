#lk_shader vertex 
#version 450 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in int texindex;
layout(location = 4) in float tilefactor;

out vec4 v_color;
out vec2 v_texcoord;
flat out int v_texindex;
out float v_tilefactor;

layout(std140, binding = 0) uniform ub_camera
{
    mat4 u_viewproj;
};

void main()
{
    gl_Position = u_viewproj * vec4(pos.xyz, 1.0);

    v_color = color;
    v_texcoord = texcoord;
    v_texindex = texindex;
    v_tilefactor = tilefactor;
}

#lk_shader fragment
#version 450 core
layout(location = 0) out vec4 color;

in vec4 v_color;
in vec2 v_texcoord;
flat in int v_texindex;
in float v_tilefactor;

#define MAX_TEXTURES 16
uniform sampler2D u_textures[MAX_TEXTURES];

void main()
{
    vec4 tex = texture(u_textures[v_texindex], v_texcoord);
    color = tex * v_color;
}