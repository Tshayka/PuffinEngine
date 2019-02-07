#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 6, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 proj;
    mat4 view;
	vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord; 
layout(location = 3) in vec3 inNormals;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outColor;

layout(push_constant) uniform PushConsts {
	vec4 renderLimitPlane;
	vec3 objectPosition;
	bool glow;
	vec3 color;
} pushConsts;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	vec3 locPos = vec3(ubo.model * vec4(inPosition, 1.0));
	outColor = inColor;
	outWorldPos = locPos + pushConsts.objectPosition;
	gl_Position = ubo.proj * ubo.view * vec4(outWorldPos, 1.0);
}

