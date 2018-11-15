#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform UniformBufferObjectParam {
	vec3 light_color;
	float exposure;
	vec3 light_pos[1];
} uboParam;

// material parameters
layout(set = 0, binding = 2) uniform samplerCube samplerIrradiance;
layout(set = 0, binding = 3) uniform sampler2D albedoMap;
layout(set = 0, binding = 4) uniform sampler2D metallicMap;
layout(set = 0, binding = 5) uniform sampler2D roughnessMap;
layout(set = 0, binding = 6) uniform sampler2D normalMap; // bump
layout(set = 0, binding = 7) uniform sampler2D aoMap;

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 CameraPos;
layout(location = 4) in vec3 Color;

layout(location = 0) out vec4 outColor;

#define PI 3.14159265359

// Additional resources: 
// http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
// http://filmicgames.com/archives/75
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

float random(vec2 co) {
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

vec3 Uncharted2Tonemap(vec3 x) {
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec2 Hammersley(uint i, uint N) {
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N) {
	float a = Roughness * Roughness;
	
	float Phi = 2.0 * PI * Xi.x + random(N.xz) * 0.1;
	float CosTheta = sqrt((1.0 - Xi.y) / ( 1.0 + (a*a - 1.0) * Xi.y));
	float SinTheta = sqrt(1.0 - CosTheta * CosTheta);

	// from spherical coordinates to cartesian coordinates
	vec3 H = vec3(cos(Phi) * SinTheta, sin(Phi) * SinTheta, CosTheta);
	
	// from tangent-space vector to world-space sample vector
	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 TangentX = normalize(cross(UpVector, N));
	vec3 TangentY = cross(N, TangentX);
	
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float rough_sq = roughness * roughness;
	float alpha2 = rough_sq * rough_sq;
	float NdotH = max(dot(N, H), 0.0);
	float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0)  * pow(1.0 - cosTheta, 5.0);
}

vec3 PrefilteredEnvMap( vec3 unitReflection, float roughness, uint NumSamples) {
	vec3 N = unitReflection;
	vec3 V = unitReflection;

	vec3 prefilteredColor = vec3(0.0);
	float totalWeight = 0.0;
	
	for(uint i = 0u; i < NumSamples; ++i)
	{
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, roughness, N);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0); 

		if( NdotL > 0.0 ) {
			float D  = DistributionGGX(N, H, roughness);
			float NdotH = max(dot(N, H), 0.0);
			float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

            float resolution = 512.0; // resolution of source cubemap (per face)
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(NumSamples) * pdf + 0.0001);

			float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
			prefilteredColor += textureLod(samplerIrradiance, L, 0.0).rgb * NdotL;
						
			totalWeight += NdotL;
		}
	}
	return prefilteredColor / totalWeight;
}

vec2 IntegrateBRDF(float NdotV, float roughness, uint NumSamples) {
	const vec3 N = vec3(0.0, 0.0, 1.0);
	vec3 V = vec3(sqrt(1.0 - NdotV*NdotV), 0.0, NdotV);

	float A = 0.0;
	float B = 0.0;
	for(uint i = 0u; i < NumSamples; ++i) {
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, roughness, N);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
		float VdotH = max(dot(V, H), 0.0);

		if (NdotL > 0.0) {
			float G = GeometrySmith(N, V, L, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);
			
			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	A/= float(NumSamples);
	B/= float(NumSamples);
	return vec2(A, B); 
}

vec3 getNormalFromMap()
{
	vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N  = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main() 
{
	const uint NumSamples = 256;

	// material properties
	vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2)); //OK
	float metallic = texture(metallicMap, TexCoords).r;
	float roughness = texture(roughnessMap, TexCoords).r;
	float ao = texture(aoMap, TexCoords).r;

	// input lighting data
	vec3 N = getNormalFromMap();
	vec3 V = normalize(CameraPos - WorldPos); 
	vec3 R = reflect(-V, N); // 2.0 * dot( V,N ) * N - V; 
		
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic); 

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < uboParam.light_pos.length(); ++i) {
		vec3 L = normalize(uboParam.light_pos[i] - WorldPos);
		vec3 H = normalize (V + L);
		float distance = length(uboParam.light_pos[i] - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = uboParam.light_color * attenuation;
			
		// Microfacet SpecularBRDF
		float NDF = DistributionGGX(N, H, roughness); 
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0); 		
			
		vec3 nominator = NDF * F * G;
		float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; 
		vec3 specular = nominator / denominator;

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic; 
		
		float NdotL = max(dot(N, L), 0.0); 

		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	} 

	vec3 fresnel = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness); 

	vec3 kS = fresnel;
    vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic; 

	vec3 irradiance = texture(samplerIrradiance, N).rgb;
	vec3 diffuse = irradiance * albedo;

	vec3 prefilteredColor = PrefilteredEnvMap(R, roughness, NumSamples); //OK
	vec2 brdf = IntegrateBRDF(max(dot(N, V), 0.0), roughness, NumSamples).xy; // float NdotV = abs(dot(N, V)) + 0.001;
	vec3 specular = prefilteredColor * (fresnel * brdf.x + brdf.y); //approximate_specular_IBL

	vec3 ambient = (kD * diffuse + specular); // * texture(aoMap, inUV).r;
	vec3 color = ambient + Lo; 
	
	// Tone mapping
	color = Uncharted2Tonemap(color * uboParam.exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
	//color = color / (color + vec3(1.0)); 

	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2));
		
	outColor = vec4(color, 1.0);
}