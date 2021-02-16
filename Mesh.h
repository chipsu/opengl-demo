#pragma once

#include "Main.h"
#include "Vertex.h"
#include "AABB.h"

struct Mesh {
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;
	GLuint mVertexBuffer = 0;
	GLuint mIndexBuffer = 0;
	GLuint mVertexArray = 0;
	bool mVertexBufferDirty = true;
	bool mHidden = false;
	AABB mAABB;
	GLuint mMode = GL_TRIANGLES;

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh() {}
	~Mesh() {
		if (mVertexBuffer) glDeleteBuffers(1, &mVertexBuffer);
		if (mIndexBuffer) glDeleteBuffers(1, &mIndexBuffer);
		if (mVertexArray) glDeleteVertexArrays(1, &mVertexArray);
	}
	void Bind() {
		if (mVertexBufferDirty) {
			UpdateVertexBuffer();
			mVertexBufferDirty = false;
		}
		if (!mIndexBuffer) {
			UpdateIndexBuffer();
		}
		if (!mVertexArray) {
			UpdateVertexArray();
		}
		glBindVertexArray(mVertexArray);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	}
	void UpdateVertexBuffer() {
		if (!mVertexBuffer) glGenBuffers(1, &mVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), &mVertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void UpdateVertexArray() {
		if (!mVertexArray) glGenVertexArrays(1, &mVertexArray);
		glBindVertexArray(mVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
		Vertex::MapVertexArray();
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void UpdateIndexBuffer() {
		if (!mIndexBuffer) glGenBuffers(1, &mIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(uint32_t), &mIndices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	void UpdateAABB() {
		mAABB = AABB::FromVertices(mVertices);
	}
};
typedef std::shared_ptr<Mesh> Mesh_;
