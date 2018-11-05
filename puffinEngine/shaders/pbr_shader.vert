#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 proj;
    mat4 view;
	vec3 camera_pos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord; 
layout(location = 3) in vec3 inNormals;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outCameraPos;
layout(location = 4) out vec3 outColor;

layout(push_constant) uniform PushConsts
{
	vec3 objPos;
} pushConsts;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
	vec3 locPos = vec3(ubo.model * vec4(inPosition, 1.0));
	fragTexCoord = inTexCoord;
	outNormal = mat3(ubo.model) * inNormals;
	outCameraPos = ubo.camera_pos;
	outColor = inColor;
	
	outWorldPos = locPos + pushConsts.objPos;
	gl_Position = ubo.proj * ubo.view * vec4(outWorldPos, 1.0);
}
