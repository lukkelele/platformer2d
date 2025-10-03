#version 330 core

out vec4 color;

void main()
{
    float lerp = gl_FragCoord.y / 700.0f;
    
    color = mix(
        vec4(1.0f, 1.0f, 1.0f, 1.0f), 
        vec4(0.2f, 0.2f, 0.2f, 1.0f), 
        lerp
    );
}