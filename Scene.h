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
	glm::vec3 mGravity = { 0, -10, 0 };
	bool mGrounded = false;
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
		mPrevPos = mPos;
		mPrevRot = mRot;
	}

	glm::vec3 mForce = { 0,0,0 };
	float mMass = 1.0f;
	float mStep = 1.0f / 60.0f;
	float mAccum = 0.0f;
	float mMaxVelocity = 100.0f;
	glm::vec3 mPrevPos;
	glm::quat mPrevRot;
	std::deque<float> mHistoryY;

	glm::vec3 GetGravity() {
		return mUseGravity ? mGravity : glm::zero<glm::vec3>();
	}

	glm::vec3 GetForce() {
		return GetGravity() + mForce;
	}

	float GetDampening() {
		return mGrounded ? 2.0f : 1.0f;
	}

	void UpdatePhysics(float deltaTime) {
		if (!mUseGravity) return;
		mPrevPos = mPos;
		mPrevRot = mRot;
		mAccum += deltaTime;
		if (mAccum < mStep) return;
		while (mAccum >= mStep) {
			UpdatePhysicsStep();
			mAccum -= mStep;
		}
		if (mPos.y <= 0) {
			mPos.y = 0;
			if (!mGrounded && mForce.y < 0.001f) {
				std::cout << "Grounded!" << std::endl;
				mGrounded = true;
				//mHistoryY.push_back(-0.5f);
				//if (mHistoryY.size() > 200) mHistoryY.pop_front();
			}
		}
		if (mPos.y > 0.0f) {
			mHistoryY.push_back(mPos.y);
			if (mHistoryY.size() > 200) mHistoryY.pop_front();
		}
	}

	void UpdatePhysicsStep() {
		//auto accel = GetForce() / mMass;
		//mVelocity = accel;
		//mPos += mVelocity * mStep;
		//mPos.y = 1.0f + sin(absoluteTime);
		/*mPos += mStep * (mVelocity + accel * mStep * 0.5f);
		mVelocity += accel * mStep;
		mVelocity = mVelocity / (1.0f + GetDampening() * mStep);*/
		mVelocity = mVelocity + GetGravity() * mStep + mForce;
		mPos = mPos + mVelocity * mStep;
		mVelocity = mVelocity / (1.0f + GetDampening() * mStep);

		auto velocity = glm::length(mVelocity);
		if (velocity > mMaxVelocity) {
			mVelocity = mVelocity * (mMaxVelocity / velocity);
		}

		mForce = { 0,0,0 };
	}
	
	void Update(float absoluteTime, float deltaTime) {
		if (mAnimationController) {
			mAnimationController->Update(absoluteTime);
		}
		UpdatePhysics(deltaTime);
		auto lp = std::min(deltaTime, 1.0f);
		auto pos = glm::lerp(mPrevPos, mPos, lp);
		auto rot = glm::lerp(mPrevRot, mRot, lp);
		mTransform = glm::translate(glm::identity<glm::mat4>(), pos);
		mTransform *= glm::mat4_cast(rot);
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

	void Jump() {
		if (!mGrounded) return;
		std::cout << "JumpJumpJump!" << std::endl;
		mForce = mGravity * -2.0f;
		mGrounded = false;
		//mHistoryY.push_back(-0.25f);
		//if (mHistoryY.size() > 200) mHistoryY.pop_front();
	}
};
typedef std::shared_ptr<Entity> Entity_;

struct ModelEntity : Entity {

};

struct ParticleEntity : Entity {

};

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
