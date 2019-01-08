#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.14159265359
#define waveAmplitude 1.0
#define waveLength 10.0

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

layout(location = 0) out vec4 clipSpaceReal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormals;
layout(location = 3) out vec3 outColor;
layout(location = 4) out vec3 outCameraPos;
layout(location = 5) out vec3 outCurrentVertex;
layout(location = 6) out vec4 clipSpaceGrid;

float generateOffset(float x, float z){
	float radianX = (x/waveLength + uboo.time) * 2.0 * PI;
	float radianZ = (z/waveLength + uboo.time) * 2.0 * PI;
	return waveAmplitude * 0.5 * (sin(radianZ)+cos(radianX));
}

vec3 applyDistortion(vec3 vertex){
	float xDistortion = generateOffset(vertex.x, vertex.z);
	float yDistortion = generateOffset(vertex.x, vertex.z);
	float zDistortion = generateOffset(vertex.x, vertex.z);
	return vertex + vec3(xDistortion, yDistortion, zDistortion); 
}

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec3 currentVertex = inPosition;

    clipSpaceGrid = uboo.proj * uboo.view * uboo.model * vec4(currentVertex, 1.0);

    currentVertex = applyDistortion(currentVertex);

    outColor = inColor;
	outTexCoord = inTexCoord;
    outNormals = inNormals;
    outCameraPos = uboo.cameraPos;
    outCurrentVertex = currentVertex;
    clipSpaceReal =  uboo.proj * uboo.view * uboo.model * vec4(currentVertex, 1.0);
	gl_Position = clipSpaceReal;
}
