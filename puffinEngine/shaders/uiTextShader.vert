#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoord;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 model;
} ubo;

layout (push_constant) uniform PushConstants {
	vec2 frameBufferSize;
} push_constants;

layout (location = 0) out vec2 outTexCoord;

void main() {
	outTexCoord = inTexCoord;

	vec2 vertexPositionHomoneneousspace = inPosition - push_constants.frameBufferSize;
	vertexPositionHomoneneousspace/=push_constants.frameBufferSize;
	gl_Position = ubo.projection * ubo.model * vec4(vertexPositionHomoneneousspace, 0.0, 1.0);
}
