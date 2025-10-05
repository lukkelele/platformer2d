#version 330 core
layout(location = 0) in vec3 pos;

uniform vec2 u_offset;

void main()
{
    vec4 offset = vec4(u_offset.x, u_offset.y, 0.0, 0.0);
    gl_Position = vec4(pos.xyz, 1.0f) + offset;
}