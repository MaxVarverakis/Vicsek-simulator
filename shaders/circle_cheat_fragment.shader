#version 410 core

layout (location = 0) out vec4 FragColor;

in vec4 v_Color;
in vec2 v_UV;

const float transition_fuzz = 0.1;

void main()
{
    // Calculate (squared) distance from the center of the circle (0,0)
    float dist = dot(v_UV, v_UV); // save the sqrt comparing squares instead (r_local^2 = 1^2 = 1)

    FragColor = smoothstep(0.0, transition_fuzz, 1.0 - dist) * v_Color;
}

