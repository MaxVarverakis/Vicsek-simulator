#version 410 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 position;
layout (location = 3) in vec2 heading;
// layout (location = 4) in vec4 aInstanceColor;

uniform mat4 u_MVP;
uniform float u_scale;
uniform bool u_color;

uniform bool u_debug;
uniform int u_selectedID;
uniform int u_numNeighbors;
uniform int u_neighborIDs[16];

out vec4 v_Color;
out vec2 v_UV;

void main()
{
    // scale, rotate, and translate the vertex position
    vec2 rotatedPos = vec2(
        aPos.x * heading.x - aPos.y * heading.y,
        aPos.x * heading.y + aPos.y * heading.x
    );
    vec2 p = position + rotatedPos * u_scale;

    gl_Position = u_MVP * vec4(p, 0.0, 1.0);
    v_UV = aPos.xy;

    float angle = atan(heading.y, heading.x);
    
    if (u_color)
    {
        float PI = 3.1415926535897932384626433832795;
        float r = sin(angle) * 0.5 + 0.5;
        float g = sin(angle + 2.0 * PI / 3.0) * 0.5 + 0.5;
        float b = sin(angle + 4.0 * PI / 3.0) * 0.5 + 0.5;
        v_Color = vec4(r, g, b, 1.0);
    }
    else
    {
        if (u_debug)
        {
            if (gl_InstanceID == u_selectedID)
            {
                v_Color = vec4(1.0, 0.0, 0.0, 1.0); // Red for selected
            }
            else
            {
                bool isNeighbor = false;
                for (int i = 0; i < u_numNeighbors; ++i)
                {
                    if (gl_InstanceID == u_neighborIDs[i])
                    {
                        isNeighbor = true;
                        break;
                    }
                }
                if (isNeighbor)
                {
                    v_Color = vec4(0.0, 1.0, 0.0, 1.0); // Green for neighbors
                }
                else
                {
                    v_Color = vec4(1.0, 1.0, 1.0, 1.0); // White for others
                }
            }
        }
        else
        {
            v_Color = vec4(1.0, 1.0, 1.0, 1.0); // Default color
        }
    }
}
