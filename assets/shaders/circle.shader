#lk_shader vertex 
#version 450 core
layout(location = 0) in vec3 worldpos;
layout(location = 1) in float thickness;
layout(location = 2) in vec2 localpos;
layout(location = 3) in vec4 color;

layout(std140, binding = 0) uniform ub_camera
{
	mat4 u_viewproj;
};

struct vertex_output
{
	vec2 localpos;
	float thickness;
	vec4 color;
};

layout(location = 0) out vertex_output v_output;

void main()
{
    gl_Position = u_viewproj * vec4(worldpos.xyz, 1.0);

	v_output.localpos = localpos;
	v_output.thickness = thickness;
	v_output.color = color;
}

#lk_shader fragment
#version 450 core
layout(location = 0) out vec4 color;

struct vertex_output
{
	vec2 localpos;
	float thickness;
	vec4 color;
};

layout(location = 0) in vertex_output v_input;

void main()
{
	const float dist = sqrt(dot(v_input.localpos, v_input.localpos));
	const float fade = fwidth(dist);
	//const float fade = 0.010f;
	if ((dist > 1.0f) || (dist < 1.0f - v_input.thickness - fade))
	{
	    discard;
	}

	float alpha = 1.0f - smoothstep(1.0f - fade, 1.0f, dist);
	alpha *= smoothstep(1.0f - v_input.thickness - fade, 1.0f - v_input.thickness, dist);
	color = v_input.color;
	color.a = v_input.color.a * alpha;
}