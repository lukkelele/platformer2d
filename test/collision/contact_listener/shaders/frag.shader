#version 330 core
layout(location = 0) out vec4 color;

in vec2 v_texcoord;

uniform vec4 u_color;
uniform sampler2D u_texture;

void main()
{
    vec4 texture_color = texture(u_texture, v_texcoord);
    color = texture_color * u_color;
}