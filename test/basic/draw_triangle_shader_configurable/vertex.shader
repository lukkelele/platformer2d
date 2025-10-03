#version 330 core

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 color;

smooth out vec4 v_color;

void main()
{
    gl_Position = pos;
    v_color = color;
}