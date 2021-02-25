#pragma once

#include "Main.h"
#include "Model.h"
#include "Shader.h"
#include "btBulletDynamicsCommon.h"

inline btVector3 cast_vec3(const glm::vec3& v) {
	return btVector3(v[0], v[1], v[2]);
}

struct Scene;

struct Entity {
	typedef std::shared_ptr<Entity> Entity_;
	ShaderProgram_ mShaderProgram;
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
	btRigidBody* mRigidBody = nullptr;

	Entity() {}
	Entity(Model_ model) : mModel(model) {}
	Entity(Model_ model, const glm::vec3& pos) : mModel(model), mPos(pos) {}

	virtual Entity_ Clone() const {
		auto clone = std::make_shared<Entity>();
		clone->mShaderProgram = mShaderProgram;
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

	virtual void Init(Scene& scene);

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
	
	virtual void Update(float absoluteTime, float deltaTime) {
		if (mAnimationController) {
			mAnimationController->Update(absoluteTime);
		}
		if (mRigidBody) return;
		UpdatePhysics(deltaTime);
		auto lp = std::min(deltaTime, 1.0f);
		auto pos = glm::lerp(mPrevPos, mPos, lp);
		auto rot = glm::lerp(mPrevRot, mRot, lp);
		mTransform = glm::translate(glm::identity<glm::mat4>(), pos);
		mTransform *= glm::mat4_cast(rot);
		mTransform = glm::scale(mTransform, mScale);
		if (mAttachTo) {
			if (mAttachToNode != -1) {
				mTransform = mAttachTo->mTransform * mAttachTo->mAnimationController->mLocalTransforms[mAttachToNode] * mTransform; // FIXME: can be 1 frame behind
			} else {
				mTransform = mAttachTo->mTransform * mTransform;
			}
		}
	}

	void Move(const glm::vec3& v) {
		if (mRigidBody) {
			mRigidBody->setLinearVelocity(cast_vec3(v));
			return;
		}
		mPos += v;
	}

	void Walk(float f) {
		if (mRigidBody) {
			mRigidBody->setLinearVelocity(cast_vec3(mFront * f));
			return;
		}
		mPos += mFront * f;
	}

	void Strafe(float f) {
		if (mRigidBody) {
			mRigidBody->setLinearVelocity(cast_vec3(glm::normalize(glm::cross(mFront, mUp)) * f));
			return;
		}
		mPos += glm::normalize(glm::cross(mFront, mUp)) * f;
	}

	void Jump() {
		if (mRigidBody) {
			std::cout << "JumpJumpJump!" << std::endl;
			mRigidBody->applyCentralForce(btVector3(0, 100, 0));
			return;
		}
		if (!mGrounded) return;
		std::cout << "JumpJumpJump!" << std::endl;
		mForce = mGravity * -2.0f;
		mGrounded = false;
		//mHistoryY.push_back(-0.25f);
		//if (mHistoryY.size() > 200) mHistoryY.pop_front();
	}

	virtual void Load(Scene& scene, const rapidjson::Value& cfg);
};
typedef std::shared_ptr<Entity> Entity_;

struct ModelEntity : Entity {
	virtual void Load(Scene& scene, const rapidjson::Value& cfg);
};

struct ParticleEntity : Entity {
	virtual void Init(Scene& scene);
	virtual void Load(Scene& scene, const rapidjson::Value& cfg);
	virtual void Update(float absoluteTime, float deltaTime);
};

struct Scene {
	typedef std::function<Entity_()> EntityConstructor;
	std::vector<Entity_> mEntities;

	btDiscreteDynamicsWorld* mDynamicsWorld = nullptr;

	float mCameraDistance = 10.0f;
	float mCameraRotationX = 0.0f;
	float mCameraRotationY = 0.0f;

	Entity_ mSelected;
	size_t mSelectedIndex = -1;

	Scene();

	void Load(const std::string& fileName);

	void Init() {
		for (auto& entity : mEntities) {
			entity->Init(*this);
		}
		SelectNext();
	}

	float mAccum = 0.0f;
	float mStep = 1.0f / 60.0;
	void Update(float absoluteTime, float deltaTime) {
		mAccum += deltaTime;
		while (mAccum >= mStep) {
			mDynamicsWorld->stepSimulation(mStep);
			mAccum -= mStep;
		}
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

	std::map<std::string, EntityConstructor> mTypes;
	void Register(const std::string& type, EntityConstructor constructor) {
		mTypes[type] = constructor;
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
