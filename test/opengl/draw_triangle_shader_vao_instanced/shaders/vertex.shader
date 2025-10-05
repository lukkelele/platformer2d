#version 330 core
layout(location = 0) in vec4 pos;
layout(location = 1) in vec2 offset;

void main()
{
    gl_Position = pos;
    gl_Position.x -= offset.x;
    gl_Position.y -= offset.y;
}