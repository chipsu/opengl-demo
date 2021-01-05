#pragma once

#include "Main.h"

struct Vertex {
	glm::vec3 mPos = { 0, 0, 0 };
	glm::vec3 mNormal = { 0, 0, 0 };
	glm::vec3 mColor = { 0, 0, 0 };
	float mBoneWeights[MAX_VERTEX_WEIGHTS] = { 0 };
	uint32_t mBoneIndices[MAX_VERTEX_WEIGHTS] = { 0 };

	bool AddBoneWeight(size_t index, float weight) {
		for (size_t i = 0; i < MAX_VERTEX_WEIGHTS; ++i) {
			if (mBoneWeights[i] == 0.0f) {
				mBoneWeights[i] = weight;
				mBoneIndices[i] = index;
				return true;
			}
		}
		//std::cerr << "MAX_VERTEX_WEIGHTS (" << MAX_VERTEX_WEIGHTS << ") reached" << std::endl;
		return false;
	}

	/*layout(location = 0) in vec3 inPosition;
	layout(location = 1) in vec3 inNormal;
	layout(location = 2) in vec3 inColor;
	layout(location = 3) in vec4 inBoneWeights;
	layout(location = 4) in uvec4 inBoneIndices;*/
	static void MapVertexArray() {
		GLuint index = 0;
		auto attr = [&index](GLuint size, GLenum type, size_t offset) {
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, size, type, GL_TRUE, sizeof(Vertex), (GLvoid*)offset);
			//std::cout << "Vertex: " << index << "@" << offset << " size=" << size << std::endl;
			index++;
		};
		attr(3, GL_FLOAT, offsetof(Vertex, mPos));
		attr(3, GL_FLOAT, offsetof(Vertex, mNormal));
		attr(3, GL_FLOAT, offsetof(Vertex, mColor));
		attr(MAX_VERTEX_WEIGHTS, GL_FLOAT, offsetof(Vertex, mBoneWeights));
		attr(MAX_VERTEX_WEIGHTS, GL_FLOAT, offsetof(Vertex, mBoneIndices)); // NOTE: GL_UNSIGNED_INT does not work here for some reason
	}
};
