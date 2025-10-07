#version 330 core
layout(location = 0) in vec3 pos;

uniform vec2 u_offset;
uniform mat4 u_transform;

void main()
{
    vec4 offset = vec4(u_offset.x, u_offset.y, 0.0, 0.0);
    //gl_Position = vec4(pos.xyz, 1.0f) + offset;
    gl_Position = u_transform * (vec4(pos.xyz, 1.0) + offset);
}