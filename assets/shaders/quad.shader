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

uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;
uniform sampler2D u_texture3;
uniform sampler2D u_texture4;
uniform sampler2D u_texture5;
uniform sampler2D u_texture6;
uniform sampler2D u_texture7;
uniform sampler2D u_texture8;

void main()
{
    vec4 tex = vec4(0.0);
    switch (v_texindex)
    {
        case 0: tex = texture(u_texture0, v_texcoord); break;
        case 1: tex = texture(u_texture1, v_texcoord); break;
        case 2: tex = texture(u_texture2, v_texcoord); break;
        case 3: tex = texture(u_texture3, v_texcoord); break;
        case 4: tex = texture(u_texture4, v_texcoord); break;
        case 5: tex = texture(u_texture5, v_texcoord); break;
        case 6: tex = texture(u_texture6, v_texcoord); break;
        case 7: tex = texture(u_texture7, v_texcoord); break;
        case 8: tex = texture(u_texture8, v_texcoord); break;
    }

    color = tex * v_color;

    /* Debug */
    //color = vec4(vec3(float(v_texindex)/16.0), 1.0);
}