#version 330 core
layout(location = 0) in vec4 pos;

uniform vec2 u_offset;

void main()
{
    vec4 offset = vec4(u_offset.x, u_offset.y, 0.0, 0.0);
    gl_Position = pos + offset;
}