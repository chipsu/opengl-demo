#version 450

layout(location=0) in vec3 inColor;
layout(location=1) in vec3 inBarycentric;

layout(location=0) out vec4 outColor;

void main() {
	outColor = vec4(inColor * 2.0, 1.0);

	if(inBarycentric.x < 0.01 || inBarycentric.y < 0.01 || inBarycentric.z < 0.01) {
		outColor = outColor * 0.25;
	}
}
