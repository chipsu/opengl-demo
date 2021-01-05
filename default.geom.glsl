#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3[] inColor;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outBarycentric;

void main() {
    outColor = inColor[0];
    outBarycentric = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    outColor = inColor[1];
    outBarycentric = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    outColor = inColor[2];
    outBarycentric = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}
