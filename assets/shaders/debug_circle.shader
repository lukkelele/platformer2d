#lk_shader vertex 
#version 450 core
layout(location = 0) in vec3 pos;

void main()
{
    gl_Position = vec4(pos.xyz, 1.0);
}

#lk_shader fragment
#version 450 core
layout(location = 0) out vec4 color;

uniform vec4 u_color;

void main()
{
    color = u_color;
}