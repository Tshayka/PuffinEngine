#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 3, binding = 1) uniform samplerCube samplerIrradiance;
layout(set = 3, binding = 2) uniform sampler2D offscreenReflect;
layout(set = 3, binding = 3) uniform sampler2D offscreenRefract;

layout(location = 0) in vec4 WorldPos;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Color;
layout(location = 4) in vec3 CameraPos;

layout (location = 0) out vec4 outColor;

void main() {
	vec4 tmp = vec4(1.0 / WorldPos.w);
	vec4 projCoord = WorldPos * tmp;

	projCoord += vec4(1.0);
	projCoord *= vec4(0.5);

	vec3 N = normalize(Normal);
	vec3 V = normalize(CameraPos - WorldPos.xyz); 
	vec3 R = reflect(-V, N); 
	vec3 irradiance = texture(samplerIrradiance, N).rgb;
	outColor = vec4(irradiance * 0.25, 1.0);

	if (gl_FrontFacing) 
	{
		// Only render mirrored scene on front facing (upper) side of mirror surface
		vec4 reflection = vec4(0.0);
		for (int x = -3; x <= 3; x++) {
			for (int y = -3; y <= 3; y++) {
				reflection += texture(offscreenReflect, vec2(projCoord.s + x, projCoord.t + y)) / 49.0;
			}
		}
		outColor += reflection * 1.5 * (irradiance.r);
	};

	outColor = texture(offscreenRefract, vec2(projCoord.x, projCoord.y));
}