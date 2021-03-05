#pragma once

#include "Main.h"
#include "Mesh.h"
#include "Animation.h"
#include "AABB.h"

struct ModelMesh {
	typedef std::shared_ptr<ModelMesh> ModelMesh_;
	Mesh_ mMesh;
	glm::mat4 mTransform;

	ModelMesh(Mesh_ mesh, const glm::mat4& transform) : mMesh(mesh), mTransform(transform) {}
};
typedef ModelMesh::ModelMesh_ ModelMesh_;

struct ModelOptions {
	float mScale = 1.0f;
};

struct Model {
	std::string mName;
	std::vector<ModelMesh_> mMeshes;
	AnimationSet_ mAnimationSet;
	AABB mAABB;
	void Load(const std::string& fileName) { Load(fileName, {}); }
	void Load(const std::string& fileName, const ModelOptions& options);
	void LoadAnimation(const std::string& fileName, bool append = false) {
		LoadAnimation(fileName, {}, append);
	}
	void LoadAnimation(const std::string& fileName, const ModelOptions& options, bool append = false);
	void UpdateAABB() {
		// FIXME
		mAABB.mCenter = { 0,0,0 };
		mAABB.mHalfSize = { .5,1,.5 };
	}
	bool HasAnimations() const {
		// FIXME
		return nullptr != mAnimationSet && mAnimationSet->mAnimations.size() > 0;
	}
};
typedef std::shared_ptr<Model> Model_;
