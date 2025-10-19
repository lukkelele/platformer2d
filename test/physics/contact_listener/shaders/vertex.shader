#version 330 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texcoord;

out vec2 v_texcoord;
uniform mat4 u_transform;

void main()
{
    gl_Position = u_transform * vec4(pos.xy, 0.0, 1.0);

    v_texcoord = texcoord;
}