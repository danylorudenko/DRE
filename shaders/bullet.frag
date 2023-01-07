#version 450 core

//#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 in_color;
layout(location = 0) out vec4 finalColor;

void main()
{
	finalColor = vec4(in_color, 1.0);
}

