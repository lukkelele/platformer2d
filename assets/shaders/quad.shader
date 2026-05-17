#lk_shader vertex
#version 450 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in int texindex;
layout(location = 4) in float tilefactor;
layout(location = 5) in float outlinethickness;
layout(location = 6) in vec4 outlinecolor;

out vec4 v_color;
out vec2 v_texcoord;
flat out int v_texindex;
out float v_tilefactor;
out float v_outlinethickness;
out vec4 v_outlinecolor;

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
    v_outlinethickness = outlinethickness;
    v_outlinecolor = outlinecolor;
}

#lk_shader fragment
#version 450 core
layout(location = 0) out vec4 color;

in vec4 v_color;
in vec2 v_texcoord;
flat in int v_texindex;
in float v_tilefactor;
in float v_outlinethickness;
in vec4 v_outlinecolor;

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
uniform sampler2D u_texture9;
uniform sampler2D u_texture10;
uniform sampler2D u_texture11;
uniform sampler2D u_texture12;
uniform sampler2D u_texture13;
uniform sampler2D u_texture14;
uniform sampler2D u_texture15;
uniform float u_brightness = 1.0;

vec4 sample_texture(vec2 uv)
{
    vec4 tex = vec4(0.0);
    switch (v_texindex)
    {
        case 0: tex = texture(u_texture0, uv); break;
        case 1: tex = texture(u_texture1, uv); break;
        case 2: tex = texture(u_texture2, uv); break;
        case 3: tex = texture(u_texture3, uv); break;
        case 4: tex = texture(u_texture4, uv); break;
        case 5: tex = texture(u_texture5, uv); break;
        case 6: tex = texture(u_texture6, uv); break;
        case 7: tex = texture(u_texture7, uv); break;
        case 8: tex = texture(u_texture8, uv); break;
        case 9: tex = texture(u_texture9, uv); break;
        case 10: tex = texture(u_texture10, uv); break;
        case 11: tex = texture(u_texture11, uv); break;
        case 12: tex = texture(u_texture12, uv); break;
        case 13: tex = texture(u_texture13, uv); break;
        case 14: tex = texture(u_texture14, uv); break;
        case 15: tex = texture(u_texture15, uv); break;
    }
    return tex;
}

vec2 get_texel_size()
{
    vec2 size = vec2(1.0);
    switch (v_texindex)
    {
        case 0: size = vec2(textureSize(u_texture0, 0)); break;
        case 1: size = vec2(textureSize(u_texture1, 0)); break;
        case 2: size = vec2(textureSize(u_texture2, 0)); break;
        case 3: size = vec2(textureSize(u_texture3, 0)); break;
        case 4: size = vec2(textureSize(u_texture4, 0)); break;
        case 5: size = vec2(textureSize(u_texture5, 0)); break;
        case 6: size = vec2(textureSize(u_texture6, 0)); break;
        case 7: size = vec2(textureSize(u_texture7, 0)); break;
        case 8: size = vec2(textureSize(u_texture8, 0)); break;
        case 9: size = vec2(textureSize(u_texture9, 0)); break;
        case 10: size = vec2(textureSize(u_texture10, 0)); break;
        case 11: size = vec2(textureSize(u_texture11, 0)); break;
        case 12: size = vec2(textureSize(u_texture12, 0)); break;
        case 13: size = vec2(textureSize(u_texture13, 0)); break;
        case 14: size = vec2(textureSize(u_texture14, 0)); break;
        case 15: size = vec2(textureSize(u_texture15, 0)); break;
    }
    return (1.0 / size);
}

void main()
{
    vec4 tex = sample_texture(v_texcoord);
    float alpha = tex.a;
    vec4 base_color = tex * v_color;

    if (v_outlinethickness == 0.0)
    {
        color = vec4(base_color.rgb * u_brightness, base_color.a);
        return;
    }

    float pixel_uv_x = length(vec2(dFdx(v_texcoord.x), dFdy(v_texcoord.x)));
    float pixel_uv_y = length(vec2(dFdx(v_texcoord.y), dFdy(v_texcoord.y)));
    float border_uv_x = v_outlinethickness * pixel_uv_x;
    float border_uv_y = v_outlinethickness * pixel_uv_y;
    float dist_x = min(v_texcoord.x, 1.0 - v_texcoord.x);
    float dist_y = min(v_texcoord.y, 1.0 - v_texcoord.y);

    bool is_border = (dist_x <= border_uv_x) || (dist_y <= border_uv_y);
    if (is_border)
    {
        color = vec4(v_outlinecolor.rgb * u_brightness, v_outlinecolor.a);
    }
    else
    {
        color = vec4(base_color.rgb * u_brightness, base_color.a);
    }

    /* Debug */
    //color = vec4(vec3(float(v_texindex)/16.0), 1.0);
}