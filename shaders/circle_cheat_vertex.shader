#version 410 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in mat4 aInstanceModelMat;
layout (location = 6) in vec4 aInstanceColor;

uniform mat4 u_MVP;

out vec4 v_Color;
out vec2 v_UV;

void main()
{
    gl_Position = u_MVP * aInstanceModelMat * vec4(aPos, 0.0, 1.0);
    v_UV = aPos.xy;
    v_Color = aInstanceColor;
}
