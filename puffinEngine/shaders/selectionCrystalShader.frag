#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 CameraPos;
layout(location = 3) in vec3 Normal;
layout(location = 4) in float Time;

layout(location = 0) out vec4 outColor;

void main() {
              
	outColor = vec4(Color.xyz, 0.7);
}