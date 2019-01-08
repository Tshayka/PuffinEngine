#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 3, binding = 1) uniform samplerCube samplerIrradiance;
layout(set = 3, binding = 2) uniform sampler2D offscreenReflect;
layout(set = 3, binding = 3) uniform sampler2D offscreenRefract;

layout(location = 0) in vec4 clipSpaceReal;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Color;
layout(location = 4) in vec3 CameraPos;
layout(location = 5) in vec3 CurrentVertex;
layout(location = 6) in vec4 clipSpaceGrid;

layout (location = 0) out vec4 outColor;

vec3 calcNormal(vec3 vertex0, vec3 vertex1, vec3 vertex2){
	vec3 tangent = vertex1 - vertex0;
	vec3 bitangent = vertex2 - vertex0;
	return normalize(cross(tangent, bitangent));
}

vec2 clipSpaceToTexCoords(vec4 clipSpace) {
	vec2 tmp = vec2(1.0 / clipSpace.w);
	vec2 projCoord = clipSpace.xy * tmp;
	projCoord += vec2(1.0);
	projCoord *= vec2(0.5);
	return clamp(projCoord, 0.002, 0.998);
}

void main() {

	vec2 texCoordsReal = clipSpaceToTexCoords(clipSpaceReal);
	vec2 texCoordsGrid = clipSpaceToTexCoords(clipSpaceGrid);

	vec3 V = normalize(CameraPos - CurrentVertex);
	vec3 N = normalize(Normal);

	const float fresnelReflective = 0.9;
	float F = clamp(pow(dot(V, N), fresnelReflective), 0.0, 1.0);

//	float F = pow(1.0 - clamp(dot(normalize(CameraPos - WorldPos.xyz), normalize(Normal)), 0.0, 1.0), 1.0);

	vec4 reflection = texture(offscreenReflect, texCoordsGrid);
	vec4 refraction = texture(offscreenRefract, texCoordsGrid);

	vec4 refractColor = vec4(0.0, 0.41, 0.56, 1.0);
	reflection = mix(reflection, refractColor, 0.1);

	outColor = mix(reflection, refraction, F);
}