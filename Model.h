#pragma once

#include "Main.h"
#include "Mesh.h"
#include "Animation.h"
#include "AABB.h"

struct Model {
	std::vector<Mesh_> mMeshes;
	AnimationController_ mAnimationController;
	AABB mAABB;
	void Update(float absoluteTime, float deltaTime) {
		if (mAnimationController) {
			mAnimationController->Update(absoluteTime);
		}
	}
	void Load(const std::string& fileName);
	void LoadAnimation(const std::string& fileName, bool append = false);
	void UpdateAABB() {
		AABB aabb;
		for (const auto& mesh : mMeshes) {
			mesh->UpdateAABB();
			aabb = aabb.Extend(mesh->mAABB);
		}
		mAABB = aabb;
	}
};
typedef std::shared_ptr<Model> Model_;
