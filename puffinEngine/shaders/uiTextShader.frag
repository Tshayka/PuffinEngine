#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D fontAtlas;

layout (binding = 2) uniform UBOS {
	vec4 outlineColor;
	vec2 offset;
	float outlineDistance;
	float outline;
} uboSettings;

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outFragColor;

void main() {
    float distance = texture(fontAtlas, inTexCoord).a;
    float smoothing = fwidth(distance);	
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
	vec3 color = vec3(alpha);
									 
	if (uboSettings.outlineDistance > 0.0) {
		float w = 1.0 - uboSettings.outlineDistance;
		alpha = smoothstep(w - smoothing, w + smoothing, distance);
        color += mix(vec3(alpha), uboSettings.outlineColor.rgb, alpha);
    }									 
									 
    outFragColor = vec4(color, alpha);	
}