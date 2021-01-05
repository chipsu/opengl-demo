#pragma once

#include "Main.h"

struct AABB {
	glm::vec3 mCenter = { 0, 0, 0 };
	glm::vec3 mHalfSize = { 0, 0, 0 };
	AABB() {}
	AABB(const glm::vec3& center, const glm::vec3& halfSize) : mCenter(center), mHalfSize(halfSize) {}
	glm::vec3 GetMin() const {
		return mCenter - mHalfSize;
	}
	glm::vec3 GetMax() const {
		return mCenter + mHalfSize;
	}
	AABB Extend(const glm::vec3& v) const {
		auto min = GetMin();
		auto max = GetMax();
		for (int i = 0; i < 3; ++i) {
			min[i] = std::min(min[i], v[i]);
			max[i] = std::max(max[i], v[i]);
		}
		return AABB::FromExtents(min, max);
	}
	AABB Extend(std::initializer_list<glm::vec3> args) const {
		auto min = GetMin();
		auto max = GetMax();
		for (int i = 0; i < 3; ++i) {
			for (const auto & arg : args) {
				min[i] = std::min(min[i], arg[i]);
				max[i] = std::max(max[i], arg[i]);
			}
		}
		return AABB::FromExtents(min, max);
	}
	AABB Extend(const AABB& other) const {
		return Extend({ other.GetMin(), other.GetMax() });
	}
	static AABB FromExtents(const glm::vec3& min, const glm::vec3& max) {
		glm::vec3 mHalfSize = (max - min) * 0.5f;
		return AABB(min + mHalfSize, mHalfSize);
	}
	static AABB FromVertices(const std::vector<Vertex>& vertices) {
		if (!vertices.size()) {
			return AABB();
		}
		glm::vec3 min = vertices[0].mPos;
		glm::vec3 max = vertices[0].mPos;
		for (const auto& v : vertices) {
			for (int i = 0; i < 3; ++i) {
				min[i] = std::min(min[i], v.mPos[i]);
				max[i] = std::max(max[i], v.mPos[i]);
			}
		}
		return FromExtents(min, max);
	}
};
