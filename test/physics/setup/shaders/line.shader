#lk_shader vertex 
#version 330 core
layout(location = 0) in vec2 pos;

uniform mat4 u_transform;

void main()
{
    gl_Position = u_transform * vec4(pos.xy, 0.0, 1.0);
}

#lk_shader fragment
#version 330 core
layout(location = 0) out vec4 color;

uniform vec4 u_color;

void main()
{
    color = u_color;
}
