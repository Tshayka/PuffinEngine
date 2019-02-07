#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 2, binding = 0) uniform UboClouds {
	mat4 model;
	mat4 proj;
    mat4 view;
	vec3 cameraPos;
	float time;
} uboc;

layout(set = 2, binding = 1) uniform UboCloudsMatrices
{
	mat4 model;
} ubo_data_dynamic;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord; 
layout(location = 3) in vec3 inNormals;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outColor;
layout(location = 4) out vec3 outCameraPos;
layout(location = 5) out float outTime;

out gl_PerVertex 
{
    vec4 gl_Position;
};

vec3 colors[3] = vec3[](
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0)
);


void main() 
{
	outColor = colors[gl_VertexIndex];

	fragTexCoord = inTexCoord;
	outNormal = mat3(ubo_data_dynamic.model) * inNormals;

	mat4 modelView = uboc.view * ubo_data_dynamic.model;
	outWorldPos = vec3(ubo_data_dynamic.model * vec4(inPosition, 1.0));

	outCameraPos = uboc.cameraPos;
	outTime = uboc.time;
	
	gl_Position = uboc.proj * modelView * vec4(inPosition.xyz, 1.0);
}
