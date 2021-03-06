#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec3 Color;

layout(location = 0) out vec4 outColor;

void main() {	
	outColor = vec4(Color, 1.0);
}