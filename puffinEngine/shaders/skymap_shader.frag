#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 1, binding = 1) uniform samplerCube texCubemapSampler;

layout(location = 0) in vec3 fragTexCoord;
layout(location = 1) in float gamma;
layout(location = 2) in float exposure;

layout(location = 0) out vec4 outColor;

void main() 
{

    vec3 color = texture(texCubemapSampler, fragTexCoord).rgb;
	// Gamma correction
	color = pow(color, vec3(1.0f / gamma));
	outColor = vec4(color, 1.0);
}
