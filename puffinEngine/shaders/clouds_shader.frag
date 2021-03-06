#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 outWorldPos;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Color;
layout(location = 4) in vec3 CameraPos;
layout(location = 5) in float Time;

layout (location = 0) out vec4 outColor;


float random (in vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 _st) {
    vec2 i = floor(_st);
    vec2 f = fract(_st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

#define NUM_OCTAVES 5

float fbm ( in vec2 _st) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    // Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5),
                    -sin(0.5), cos(0.50));
    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(_st);
        _st = rot * _st * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main() {

	vec3 V = normalize(-outWorldPos);
	vec3 N = normalize(Normal);

	const float fresnelReflective = 0.9;
	float F = clamp(pow(dot(V, N), fresnelReflective), 0.0, 1.0);



	vec2 u_resolution = vec2(4.0, 4.0);
	vec2 st = outWorldPos.xy/u_resolution.xy*3.;
    // st += st * abs(sin(Time*0.1)*3.0);
    vec3 color = vec3(0.0);

    vec2 q = vec2(0.);
    q.x = fbm( st + 0.00*Time);
    q.y = fbm( st + vec2(1.0));

    vec2 r = vec2(0.);
    r.x = fbm( st + 1.0*q + vec2(1.7,9.2)+ 0.15*Time);
    r.y = fbm( st + 1.0*q + vec2(8.3,2.8)+ 0.126*Time);

    float f = fbm(st+r);

   color = mix(vec3(0.050,0.050,0.050),
                vec3(0.870,0.870,0.870),
                clamp((f*f)*4.0,0.0,1.0));

    color = mix(color,
                vec3(0.386,0.693,0.975),
                clamp(length(q),0.0,1.0));

    color = mix(color,
                vec3(0.485,0.480,0.465),
                clamp(length(r.x),0.0,1.0));
                
	outColor = vec4((f*f*f+.6*f*f+.5*f)*color, 0.0);
}