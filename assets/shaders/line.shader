#lk_shader vertex 
#version 450 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

out vec4 v_color;

layout(std140, binding = 0) uniform ub_camera
{
    mat4 u_viewproj;
};

void main()
{
    gl_Position = u_viewproj * vec4(pos.xyz, 1.0);

    v_color = color;
}

#lk_shader fragment
#version 450 core
layout(location = 0) out vec4 color;

in vec4 v_color;

void main()
{
    color = v_color;
}
