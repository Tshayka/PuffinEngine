#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 5, binding = 1) uniform samplerCube samplerIrradiance;

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec3 Color;
layout(location = 2) in vec3 CameraPos;
layout(location = 3) in vec3 Normal;
layout(location = 4) in float Time;
layout(location = 5) in vec2 TexCoords;

layout(location = 0) out vec4 outColor;

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0)  * pow(1.0 - cosTheta, 5.0);
}

void main() {
	vec3 N = normalize(Normal);
	vec3 V = normalize(CameraPos - WorldPos); 
	vec3 R = reflect(-V, N);

	// vec3 albedo = pow(Color, vec3(2.2));
	vec3 albedo = Color;
	float roughness = 1.0;
	float metallic = 0.1;

	vec3 F0 = vec3(0.04);
	//F0 = mix(F0, albedo, metallic);  

	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
	vec3 irradiance = texture(samplerIrradiance, N).rgb;
	vec3 diffuse = irradiance * albedo;
	vec3 ambient = (kD * diffuse);

	vec3 color = pow(ambient, vec3(1.0 / 2.2)); 
              
	outColor = vec4(color, 1.0);
}