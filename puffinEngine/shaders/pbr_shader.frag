#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform UniformBufferObjectParam 
{
	vec3 light_color;
	vec3 light_pos[1];
	float exposure;
	float gamma;
} ubo_parameters;

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

#define PI 3.1415926535897

// Additional resources: 
// http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
// http://filmicgames.com/archives/75
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

float random(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec2 Hammersley(uint i, uint N)
{
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness;
	
	float Phi = 2 * PI * Xi.x + random(N.xz) * 0.1;;
	float CosTheta = sqrt((1.0 - Xi.y) / ( 1.0 + (a*a - 1.0) * Xi.y));
	float SinTheta = sqrt(1.0 - CosTheta * CosTheta);

	// from spherical coordinates to cartesian coordinates
	vec3 H = vec3(SinTheta * cos(Phi), SinTheta * sin(Phi), CosTheta);
	
	// from tangent-space vector to world-space sample vector
	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) :vec3(1.0, 0.0, 0.0);
	vec3 TangentX = normalize(cross(UpVector, N));
	vec3 TangentY = cross(N, TangentX);
	
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float DistributionGGX(float dotNH, float roughness)
{
	float rough_sq = roughness * roughness;
	float alpha2 = rough_sq * rough_sq;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

float GeometrySmith(float roughness, float dotNV, float dotNL)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float ggx1 = dotNL / (dotNL * (1.0 - k) + k);
	float ggx2 = dotNV / (dotNV * (1.0 - k) + k);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float dotVH, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - dotVH, 5.0);
}

vec3 fresnelSchlickRoughness(float dotVH, vec3 F0, vec3 prefiltered_map)
{
	return F0 + (clamp(vec3(1.0 - prefiltered_map), 0.0, 1.0) - F0) * pow(1.0 - dotVH, 5.0);
}

vec3 PrefilteredEnvMap( vec3 unitReflection, float roughness, uint NumSamples)
{
	vec3 N = unitReflection;
	vec3 V = unitReflection;

	vec3 PrefilteredColor = vec3(0.0);
	float TotalWeight = 0.0;
	
	for( uint sampleIndex = 0; sampleIndex < NumSamples; sampleIndex++ )
	{
		vec2 vXi = Hammersley(sampleIndex, NumSamples);
		vec3 H = ImportanceSampleGGX(vXi, roughness, N);
		vec3 L = 2.0 * dot( V, H ) * H - V;

		float NoL = clamp(dot(N, L), 0.0, 1.0); 
		if( NoL > 0.0 )
		{
			PrefilteredColor += textureLod(samplerIrradiance, L, 0.0).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedo)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.001, 1.0);
	float dotVH = clamp(dot(V, H), 0.0, 1.0);
 
	// Light color fixed
	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		// Microfacet SpecularBRDF
		float D = DistributionGGX(dotNH, roughness); 
		float G = GeometrySmith(roughness, dotNV, dotNL);
		vec3 F = fresnelSchlick(dotVH, F0);		
		
		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);		
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
		color += (kD * albedo / PI + spec) * dotNL;
	}

	return color;
}

vec2 BRDF(float NoV, float roughness, uint NumSamples)
{
	// Normal always points along z-axis for the 2D lookup 
	const vec3 N = vec3(0.0, 0.0, 1.0);
	vec3 V = vec3(sqrt(1.0 - NoV*NoV), 0.0, NoV);

	float A = 0;
	float B = 0;
	for(uint i = 0u; i < NumSamples; i++) 
	{
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, roughness, N);
		vec3 L = -normalize(reflect(V, H));

		 -normalize(reflect(V, H));

		float dotNL = clamp(L.z, 0.001, 1.0);
		float dotNV = clamp(dot(N, V), 0.0, 1.0);
		float dotVH = clamp(dot(V, H), 0.0, 1.0); 
		float dotNH = clamp(H.z, 0.0, 1.0);

		if (dotNL > 0.0) {
			float G = GeometrySmith(roughness, dotNV, dotNL);
			
			float G_Vis = (G * dotVH) / (dotNH * dotNV);
			float Fc = pow(1.0 - dotVH, 5.0);
			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	return vec2(A, B) / float(NumSamples);
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

	// input lighting data
	vec3 N = normalize(Normal); //getNormalFromMap();
	vec3 V = normalize(CameraPos - WorldPos); 
	vec3 R = reflect(-V, N); // 2.0 * dot( V,N ) * N - V; 
		
	vec3 F0 = vec3(0.04); 
	vec3 diffuse_color = albedo * (vec3(1.0) - F0);
	diffuse_color *= 1.0 - metallic;
	vec3 specular_color = mix(F0, albedo, metallic);
	//float reflectance = max(max(specular_color.r, specular_color.g), specular_color.b);
	

	float NdotV = clamp(dot(N, V), 0.001, 1.0); // float NdotV = abs(dot(N, V)) + 0.001;
	vec2 EnvBRDF = BRDF(NdotV, roughness, NumSamples).rg;
	vec3 prefiltered_color = PrefilteredEnvMap(R, roughness, NumSamples); //OK
	
	vec3 specular = prefiltered_color * (specular_color * EnvBRDF.x + EnvBRDF.y); //approximate_specular_IBL
	vec3 diffuse = prefiltered_color * diffuse_color;
	vec3 fresnel = fresnelSchlickRoughness(clamp(dot(N, V), 0.0, 1.0), F0, prefiltered_color);

	vec3 color = (diffuse + specular + fresnel); // * texture(aoMap, inUV).r;
	
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < ubo_parameters.light_pos[i].length(); i++) 
	{
		vec3 L = normalize(ubo_parameters.light_pos[i].xyz - WorldPos);
		Lo += specularContribution(L, V, N, specular_color, metallic, roughness, albedo);
	} 

	color += Lo;

	// Tone mapping
	//color = Uncharted2Tonemap(color * ubo_parameters.exposure);
	//color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f))); 
	
	// Gamma correction
	//color = pow(color, vec3(1.0f / ubo_parameters.gamma));
		
	outColor = vec4(color, 1.0);
}