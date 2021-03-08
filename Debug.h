#pragma once

#include "Main.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"

struct DebugLine {
	glm::vec3 mStart;
	glm::vec3 mEnd;
	glm::vec3 mColor;
};

struct DebugPoint {
	glm::vec3 mPos;
	glm::vec3 mColor = { 1,1,1 };
	float mScale = 1.0f;
	DebugPoint(float x, float y, float z, float s = 1.0f) : DebugPoint(glm::vec3(x, y, z), s) {}
	DebugPoint(const glm::vec3& p, float s = 1.0f) {
		mPos = p;
		mScale = s;
	}
};

struct DebugRenderer {
	std::vector<DebugLine> mLines;
	std::vector<DebugPoint> mPoints;
	ShaderProgram_ mLineProgram;
	ShaderProgram_ mPointProgram;

	DebugRenderer() {
		mLineProgram = ShaderProgram::Load("debug");
		mPointProgram = ShaderProgram::Load("particles");
	}

	void Clear() {
		mLines.clear();
		mPoints.clear();
	}

	void AddLine(const DebugLine& line) { mLines.push_back(line); }
	void AddCube(const glm::vec3& pos, const AABB& aabb, const glm::vec3& color) {
		auto up = glm::vec3(0, 1, 0);
		auto s = aabb.mHalfSize * 2;
		auto a = pos + aabb.mCenter - aabb.mHalfSize;
		auto b = a + s * glm::vec3(1, 0, 0);
		auto c = b + s * glm::vec3(0, 0, 1);
		auto d = c + s * glm::vec3(-1, 0, 0);

		//AddLine({ { a.x, a.y, a.z }, a + aabb.mHalfSize * 2, {0,0,1} });

		AddLine({ a, b, color });
		AddLine({ b, c, color });
		AddLine({ c, d, color });
		AddLine({ d, a, color });

		AddLine({ a, a + s * up, color });
		AddLine({ b, b + s * up, color });
		AddLine({ c, c + s * up, color });
		AddLine({ d, d + s * up, color });

		AddLine({ a + s * up, b + s * up, color });
		AddLine({ b + s * up, c + s * up, color });
		AddLine({ c + s * up, d + s * up, color });
		AddLine({ d + s * up, a + s * up, color });
	}

	void AddGrid(float scale, float halfSize, const glm::vec3& color) {
		for (int dx = -halfSize; dx <= halfSize; ++dx) {
			AddLine({ {dx * scale, 0, -halfSize * scale}, {dx * scale, 0, halfSize * scale}, color });
			AddLine({ {-halfSize * scale, 0, dx * scale}, {halfSize * scale, 0, dx * scale}, color });
		}
	}

	void AddPoint(const DebugPoint& point) {
		mPoints.push_back(point);
	}

	void AddPoint(const glm::vec3& point) {
		mPoints.push_back({ point });
	}

	void Render(const Camera& cam) {
		auto drawDebugLines = [](auto& debugLines) {
			for (auto& debugLine : debugLines) {
				auto debugMesh = std::make_shared<Mesh>();
				debugMesh->mIndices.push_back(0);
				debugMesh->mIndices.push_back(1);
				debugMesh->mVertices.push_back({
					debugLine.mStart,
					{},
					debugLine.mColor
					});
				debugMesh->mVertices.push_back({
					debugLine.mEnd,
					{},
					debugLine.mColor
					});
				debugMesh->Bind();
				glDrawElements(GL_LINES, debugMesh->mIndices.size(), GL_UNSIGNED_INT, 0);
			}
		};

		auto drawDebugPoints = [](auto& debugPoints) {
			if (debugPoints.size() < 1) return;
			auto debugMesh = std::make_shared<Mesh>();
			for (auto& debugPoint : debugPoints) {
				float ps = 0.25f;
				float quad[] = {
					0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
				};
				for (int v = 0; v < 12; v += 2) {
					Vertex vertex;
					vertex.mPos = debugPoint.mPos;
					vertex.mNormal.x = (quad[v] - 0.5f) * ps;
					vertex.mNormal.y = (quad[v + 1] - 0.5f) * ps;
					vertex.mNormal.z = debugPoint.mScale;
					debugMesh->mVertices.push_back(vertex);
					debugMesh->mIndices.push_back(debugMesh->mIndices.size());
				}
			}
			debugMesh->Bind();
			glDrawElements(GL_TRIANGLES, debugMesh->mIndices.size(), GL_UNSIGNED_INT, 0);
		};

		auto debugTransform = glm::identity<glm::mat4>();

		glUseProgram(mLineProgram->mID);
		glUniformMatrix4fv(glGetUniformLocation(mLineProgram->mID, "uProj"), 1, GL_FALSE, (GLfloat*)&cam.mProjection[0]);
		glUniformMatrix4fv(glGetUniformLocation(mLineProgram->mID, "uView"), 1, GL_FALSE, (GLfloat*)&cam.mView[0]);
		glUniformMatrix4fv(glGetUniformLocation(mLineProgram->mID, "uModel"), 1, GL_FALSE, (GLfloat*)&debugTransform[0]);

		drawDebugLines(mLines);

		glUseProgram(mPointProgram->mID);
		glUniformMatrix4fv(glGetUniformLocation(mPointProgram->mID, "uProj"), 1, GL_FALSE, (GLfloat*)&cam.mProjection[0]);
		glUniformMatrix4fv(glGetUniformLocation(mPointProgram->mID, "uView"), 1, GL_FALSE, (GLfloat*)&cam.mView[0]);
		glUniformMatrix4fv(glGetUniformLocation(mPointProgram->mID, "uModel"), 1, GL_FALSE, (GLfloat*)&debugTransform[0]);

		drawDebugPoints(mPoints);
	}
};
