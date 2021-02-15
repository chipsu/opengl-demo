#version 450

layout(location=0) uniform mat4 uProj;
layout(location=1) uniform mat4 uView;
layout(location=2) uniform mat4 uModel;
layout(location=3) uniform float uTime;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inColor;

layout(location=0) out vec3 outColor;

void main() {
    //vec3 offset  = inPosition + inNormal * uTime * 0.1;

	// TODO: use custom vertex format for particles
	float scale = 1.0f;
	vec3 center = inNormal;
	vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
	vec3 camUp = vec3(uView[0][1], uView[1][1], uView[2][1]);
	vec3 pos = center + camRight * inPosition.x * scale + camUp * inPosition.y * scale;

	gl_Position = uProj * uView * uModel * vec4(pos, 1.0f);
    
    outColor = vec3(1,0,0);
}
