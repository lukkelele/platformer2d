#version 330 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texcoord;

out vec2 v_texcoord;
uniform vec2 u_offset;

void main()
{
    vec2 pos_offset = pos + u_offset;
    gl_Position = vec4(pos_offset, 0.0, 1.0);

    v_texcoord = texcoord;
}