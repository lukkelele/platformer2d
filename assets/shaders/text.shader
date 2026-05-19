#lk_shader vertex
#version 450 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec4 outlinecolor;
layout(location = 4) in float outlinewidth;

out vec4 v_color;
out vec2 v_texcoord;
out vec4 v_outlinecolor;
out float v_outlinewidth;

uniform mat4 u_viewproj;

void main()
{
    gl_Position = u_viewproj * vec4(pos, 1.0);
    v_color = color;
    v_texcoord = texcoord;
    v_outlinecolor = outlinecolor;
    v_outlinewidth = outlinewidth;
}

#lk_shader fragment
#version 450 core
layout(location = 0) out vec4 color;

in vec4 v_color;
in vec2 v_texcoord;
in vec4 v_outlinecolor;
in float v_outlinewidth;

uniform sampler2D u_atlas;
uniform float u_pxrange;
uniform float u_brightness = 1.0;

float median3(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screen_px_range()
{
    vec2 unit_range = vec2(u_pxrange) / vec2(textureSize(u_atlas, 0));
    vec2 screen_tex_size = vec2(1.0) / fwidth(v_texcoord);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

void main()
{
    vec3 msd = texture(u_atlas, v_texcoord).rgb;
    float sd = median3(msd.r, msd.g, msd.b);
    float screen_px = screen_px_range();

    float fill_alpha = clamp(screen_px * (sd - 0.5) + 0.5, 0.0, 1.0);

    if (v_outlinewidth <= 0.0)
    {
        color = vec4(v_color.rgb * u_brightness, v_color.a * fill_alpha);
        return;
    }

    float outline_thresh = 0.5 - (v_outlinewidth / screen_px);
    float outer_alpha = clamp(screen_px * (sd - outline_thresh) + 0.5, 0.0, 1.0);

    vec3 fill_rgb = v_color.rgb * u_brightness;
    vec3 outline_rgb = v_outlinecolor.rgb * u_brightness;
    vec3 rgb = mix(outline_rgb, fill_rgb, fill_alpha);
    float a = max(v_color.a * fill_alpha, v_outlinecolor.a * outer_alpha);
    color = vec4(rgb, a);
}
