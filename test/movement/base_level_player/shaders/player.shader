#lk_shader vertex 
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

#lk_shader fragment
#version 330 core
layout(location = 0) out vec4 color;

in vec2 v_texcoord;

uniform vec4 u_color;
uniform sampler2D u_texture;

void main()
{
    vec4 texture_color = texture(u_texture, v_texcoord);
    color = texture_color * u_color;
}
