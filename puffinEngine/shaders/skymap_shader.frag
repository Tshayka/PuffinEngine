#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 1, binding = 1) uniform UniformBufferObjectParam {
	vec3 lightColor;
	float exposure;
	vec3 lightPosition[1];
} uboParam;

layout(set = 1, binding = 2) uniform samplerCube texCubemapSampler;

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(texCubemapSampler, fragTexCoord).rgb;
	float gamma = 1.0;
	// Gamma correction
	color = pow(color, vec3(1.0f / gamma));
	outColor = vec4(color, 1.0);
}
