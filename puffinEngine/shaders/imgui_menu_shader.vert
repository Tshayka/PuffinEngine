#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec4 inColor;

layout (push_constant) uniform PushConstants {
	vec2 scale;
	vec2 translate;
} push_constants;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outTexCoord;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	outColor = inColor;
	outTexCoord = inTexCoord;
	gl_Position = vec4(inPosition * push_constants.scale + push_constants.translate, 0.0, 1.0);
}