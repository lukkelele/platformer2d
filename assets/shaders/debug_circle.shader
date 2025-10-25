#lk_shader vertex 
#version 450 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 localpos;

uniform mat4 u_viewproj;

out vec2 v_localpos;

void main()
{
    gl_Position = u_viewproj * vec4(pos.xy, 0.0, 1.0);

	v_localpos = localpos;
}


#lk_shader fragment
#version 450 core
layout(location = 0) out vec4 color;

in vec2 v_localpos;

uniform vec4 u_color;
uniform float u_thickness;

void main()
{
	const float dist = sqrt(dot(v_localpos, v_localpos));
	const float fade = fwidth(dist);
	if ((dist > 1.0f) || (dist < 1.0f - u_thickness - fade))
	{
	    discard;
	}

    color = u_color;
}
