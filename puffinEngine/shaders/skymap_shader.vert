#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 1, binding = 0) uniform UniformBufferObjectSky 
{
   mat4 view;
   mat4 proj;
   float exposure;
   float gamma;
} ubos;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inTexCoord;
layout(location = 3) in vec3 inNormals;

layout(location = 0) out vec3 fragTexCoord;
layout(location = 1) out float outGamma;
layout(location = 2) out float outExposure;

out gl_PerVertex 
{
    vec4 gl_Position;
};


void main() 
{
	fragTexCoord = inPosition;
	outExposure = ubos.exposure;
	outGamma = ubos.gamma;
	gl_Position = ubos.proj * ubos.view * vec4(inPosition, 1.0);
}
