#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D samplerFont;

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

void main(void)
{
	float color = texture(samplerFont, inTexCoord).r;
	outColor = vec4(vec3(color), 1.0);
}