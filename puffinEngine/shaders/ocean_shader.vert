#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 3, binding = 0) uniform UboOcean {
    mat4 model;
	mat4 proj;
    mat4 view;
    vec3 cameraPos;
    float time;
} uboo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord; 
layout(location = 3) in vec3 inNormals;

layout(location = 0) out vec4 outWorldPos;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormals;
layout(location = 3) out vec3 outColor;
layout(location = 4) out vec3 outCameraPos;

out gl_PerVertex {
    vec4 gl_Position;
};



void main() {
    outColor = inColor;
	outTexCoord = inTexCoord;
    outNormals = inNormals;
    outCameraPos = uboo.cameraPos;
    outWorldPos =  uboo.proj * uboo.view * uboo.model * vec4(inPosition.xyz, 1.0);
	gl_Position = outWorldPos;
}
