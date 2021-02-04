#pragma once

#include "Main.h"
#include "Model.h"

struct Entity {
	typedef std::shared_ptr<Entity> Entity_;
	Model_ mModel = nullptr;
	glm::vec3 mPos = { 0,0,0 };
	glm::vec3 mFront = { 0,0,1 };
	glm::vec3 mUp = { 0,1,0 };
	glm::quat mRot = { 1,0,0,0 };
	glm::vec3 mScale = { 1,1,1 };
	glm::mat4 mTransform = glm::identity<glm::mat4>();
	AnimationController_ mAnimationController;
	bool mControllable = false;
	bool mUseGravity = false;
	glm::vec3 mVelocity = { 0,0,0 };
	glm::vec3 mGravity = { 0, -200.0f, 0 };
	std::string mName;
	Entity_ mAttachTo;
	uint32_t mAttachToNode = -1;
	Entity() {}
	Entity(Model_ model) : mModel(model) {}
	Entity(Model_ model, const glm::vec3& pos) : mModel(model), mPos(pos) {}

	Entity_ Clone() const {
		auto clone = std::make_shared<Entity>();
		clone->mModel = mModel;
		clone->mPos = mPos;
		clone->mFront = mFront;
		clone->mUp = mUp;
		clone->mRot = mRot;
		clone->mScale = mScale;
		clone->mControllable = mControllable;
		clone->mUseGravity = mUseGravity;
		clone->mGravity = mGravity;
		return clone;
	}

	void Init() {
		if (mModel && mModel->mAnimationSet) {
			mAnimationController = std::make_shared<AnimationController>(mModel->mAnimationSet);
		}
	}
	void Update(float absoluteTime, float deltaTime) {
		if (mAnimationController) {
			mAnimationController->Update(absoluteTime);
		}
		mPos += mVelocity * deltaTime;
		if (mUseGravity) {
			mVelocity += mGravity * deltaTime;
			if (mPos.y < 0) mPos.y = 0;
		}
		mTransform = glm::translate(glm::identity<glm::mat4>(), mPos);
		mTransform *= glm::mat4_cast(mRot);
		mTransform = glm::scale(mTransform, mScale);
		if (mAttachTo && mAttachToNode != -1) {
			//mTransform = glm::translate(mTransform, glm::vec3(-1.5f, 0, 1.9f)); // FIXME: where/how do we get this offset?
			mTransform = mAttachTo->mTransform * mAttachTo->mAnimationController->mLocalTransforms[mAttachToNode] * mTransform; // FIXME: can be 1 frame behind
		}
	}

	void Walk(float f) {
		mPos += mFront * f;
	}

	void Strafe(float f) {
		mPos += glm::normalize(glm::cross(mFront, mUp)) * f;
	}
};
typedef std::shared_ptr<Entity> Entity_;

struct Scene {
	std::vector<Entity_> mEntities;

	float mCameraDistance = 10.0f;
	float mCameraRotationX = 0.0f;
	float mCameraRotationY = 0.0f;

	Entity_ mSelected;
	size_t mSelectedIndex = -1;

	void Load(const std::string& fileName);

	void Init() {
		for (auto& entity : mEntities) {
			entity->Init();
		}
		SelectNext();
	}

	void Update(float absoluteTime, float deltaTime) {
		/*std::for_each(std::execution::par, mEntities.begin(), mEntities.end(), [absoluteTime, deltaTime](auto& entity) {
			entity->Update(absoluteTime, deltaTime);
		});*/
		for (auto& entity : mEntities) {
			entity->Update(absoluteTime, deltaTime);
		}
	}

	Entity_ Find(const std::string& name) const {
		for (const auto& entity : mEntities) {
			if (entity->mName == name) return entity;
		}
		return nullptr;
	}

	void SelectNext() {
		mSelectedIndex++;
		if (mSelectedIndex > mEntities.size()) {
			mSelectedIndex = 0;
		}
		mSelected = mSelectedIndex < mEntities.size() ? mEntities[mSelectedIndex] : nullptr;
		if (nullptr != mSelected) {
			mCameraDistance = glm::length(mSelected->mModel->mAABB.mHalfSize) * 2.0f; // FIXME
		}
	}
};
typedef std::shared_ptr<Scene> Scene_;
