#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 5, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 proj;
    mat4 view;
	vec3 cameraPos;
	float time;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inTexCoord;
layout(location = 3) in vec3 inNormals;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec3 outCameraPos;
layout(location = 3) out vec3 outNormals;
layout(location = 4) out float outTime;

layout(push_constant) uniform PushConsts {
	vec4 color;
	vec4 renderLimitPlane;
	vec3 objectPosition;
	bool selected;
} pushConsts;

out gl_PerVertex {
    vec4 gl_Position;
	float gl_ClipDistance[];
};

void main() {
	vec3 locPos = vec3(ubo.model * vec4(inPosition, 1.0));
	outCameraPos = ubo.cameraPos;
    outColor = pushConsts.color;
    outNormals = inNormals;
	outTime = ubo.time;
	
	outWorldPos = locPos + pushConsts.objectPosition;
	gl_Position = ubo.proj * ubo.view * vec4(outWorldPos, 1.0);
	gl_ClipDistance[0] = dot(vec4(outWorldPos,0.0), pushConsts.renderLimitPlane);
}
