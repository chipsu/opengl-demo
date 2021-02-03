#pragma once

#include "Main.h"

template<typename T>
struct KeyFrame {
	float mTime;
	T mValue;
	KeyFrame(float time, T value) : mTime(time), mValue(value) {}
};

typedef KeyFrame<glm::vec3> VectorKey;
typedef KeyFrame<glm::quat> QuatKey;

template<typename T>
inline size_t GetKeyFrameIndex(const float time, const std::vector<T>& keys) {
	for (size_t i = 0; i < keys.size() - 1; i++) {
		if (time < (float)keys[i + 1].mTime) {
			return i;
		}
	}
	return 0;
}

template<typename TValue, typename TMixer>
inline TValue InterpolateKeyFrames(const float time, const std::vector<KeyFrame<TValue>>& keys, TMixer mix) {
	if (keys.size() == 1) {
		return keys[0].mValue;
	}
	const size_t frameIndex = GetKeyFrameIndex(time, keys);
	const auto& currentFrame = keys[frameIndex];
	const auto& nextFrame = keys[(frameIndex + 1) % keys.size()];
	const float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
	const auto start = currentFrame.mValue;
	const auto end = nextFrame.mValue;
	return mix(start, end, delta);
}

inline glm::vec3 InterpolateKeyFrames(const float time, const std::vector<VectorKey>& keys) {
	return InterpolateKeyFrames(time, keys, [](const glm::vec3& a, const glm::vec3& b, const float t) -> glm::vec3 { return glm::mix(a, b, t); });
}

inline glm::quat InterpolateKeyFrames(const float time, const std::vector<QuatKey>& keys) {
	return InterpolateKeyFrames(time, keys, [](const glm::quat& a, const glm::quat& b, const float t) -> glm::quat { return glm::slerp(a, b, t); });
}

struct AnimationTrack {
	std::string mName;
	std::vector<VectorKey> mPositionKeys;
	std::vector<QuatKey> mRotationKeys;
	std::vector<VectorKey> mScalingKeys;
	// TODO: aiAnimNode aiAnimBehaviour mPreState;
	// TODO: aiAnimNode aiAnimBehaviour mPostState;

	glm::vec3 InterpolateTranslation(const float time) const {
		return InterpolateKeyFrames(time, mPositionKeys);
	}
	glm::quat InterpolateRotation(const float time) const {
		return InterpolateKeyFrames(time, mRotationKeys);
	}
	glm::vec3 InterpolateScale(const float time) const {
		return InterpolateKeyFrames(time, mScalingKeys);
	}
	glm::mat4 InterpolateTransform(const float time) const {
		const auto translation = InterpolateTranslation(time);
		const auto rotation = InterpolateRotation(time);
		const auto scale = InterpolateScale(time);

		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), translation);
		transform *= glm::mat4_cast(rotation);
		transform = glm::scale(transform, scale);

		return transform;
	}
};
typedef std::shared_ptr<AnimationTrack> AnimationTrack_;

struct AnimationNode {
	typedef std::shared_ptr<AnimationNode> AnimationNode_;
	std::string mName;
	std::vector<AnimationNode_> mChildren;
	AnimationNode_ mParent;
	glm::mat4 mTransform;
	uint32_t mCachedBoneIndex = -2;

	AnimationNode(const std::string& name, AnimationNode_ parent, const glm::mat4& transform) : mName(name), mParent(parent), mTransform(transform) {
	}

	void Recurse(std::function<void(AnimationNode& node)> callback) {
		callback(*this);
		for (auto& child : mChildren) {
			child->Recurse(callback);
		}
	}
};
typedef AnimationNode::AnimationNode_ AnimationNode_;

struct Animation {
	std::string mName;
	std::vector<AnimationTrack_> mAnimationTracks;
	float mTicksPerSecond = 0;
	float mDuration = 0;
	AnimationNode_ mRootNode;

	// TODO: Map by Name
	AnimationTrack_ GetAnimationTrack(const std::string& name) const {
		for (const auto& track : mAnimationTracks) {
			if (track->mName == name) {
				return track;
			}
		}
		return nullptr;
	}

	glm::mat4 GetNodeTransform(const float time, const AnimationNode_ node) const {
		const auto& track = GetAnimationTrack(node->mName);
		if (nullptr == track) {
			return node->mTransform;
		}
		return track->InterpolateTransform(time);
	}

	float GetAnimationTime(const float time) {
		const float tps = mTicksPerSecond ? mTicksPerSecond : 25.0f;
		const float ticks = time * tps;
		return fmod(ticks, mDuration);
	}
};
typedef std::shared_ptr<Animation> Animation_;

// TODO: naming idk
struct AnimationSet {
	std::vector<Animation_> mAnimations;
	std::map<std::string, uint32_t> mBoneMappings;
	std::vector<glm::mat4> mBoneOffsets;
	glm::mat4 mGlobalInverseTransform;

	size_t GetAnimationIndex(const std::string& name) const {
		for (size_t i = 0; i < mAnimations.size(); ++i) {
			if (mAnimations[i]->mName == name) {
				return i;
			}
		}
		return -1;
	}

	uint32_t GetBoneIndex(AnimationNode_ node) const {
		if (node->mCachedBoneIndex != -2) return node->mCachedBoneIndex;
		node->mCachedBoneIndex = GetBoneIndex(node->mName);
		return node->mCachedBoneIndex;
	}

	uint32_t GetBoneIndex(const std::string& name) const {
		const auto it = mBoneMappings.find(name);
		if (it == mBoneMappings.end()) return -1;
		return it->second;
	}

	uint32_t MapBone(const std::string& name, const glm::mat4& boneOffset) {
		const auto it = mBoneMappings.find(name);
		if (it != mBoneMappings.end()) {
			return it->second;
		}
		const uint32_t id = mBoneMappings.size();
		mBoneMappings[name] = id;
		mBoneOffsets.resize(mBoneMappings.size());
		mBoneOffsets[id] = boneOffset;
		//std::cout << "Bone " << name << " mapped to " << id << std::endl;
		return id;
	}
};
typedef std::shared_ptr<AnimationSet> AnimationSet_;

struct AnimationController {
	AnimationSet_ mAnimationSet;
	size_t mAnimationIndex = -1;
	std::vector<glm::mat4> mFinalTransforms;

	// FIX THIS
	bool mBlended = false;
	float mBlendInDuration = 0.5f;
	float mBlendOut = 0.5f;
	std::map<size_t, float> mBlendMap;
	std::vector<glm::mat4> mBlendTransforms;

	float mMinDelta = 1.0f / 60.0f;
	float mNextUpdate = 0.0f;

	AnimationController(AnimationSet_ animationSet) {
		mAnimationSet = animationSet;
		mFinalTransforms.resize(mAnimationSet->mBoneMappings.size(), glm::identity<glm::mat4>()); // FIXME
		mBlendTransforms.resize(mAnimationSet->mBoneMappings.size()); // FIXME
	}

	/*
	Hmm, idk how this works
	move this to own controller maybe. some sequencer thing
		animation bone priorities:
		- walking + attack -> legs walking, arms attacking
		- walking + jump -> jump prio
		ik..

	*/

	void BlendAnimation(const size_t index, const float weight) {
		assert(index != -1);
		mBlendMap[index] = weight;
	}

	void BlendAnimation(const std::string& name, const float weight) {
		const auto index = mAnimationSet->GetAnimationIndex(name);
		BlendAnimation(index, weight);
	}

	/*void BlendAnimation(const size_t index, const float weight, const size_t iterations) {
	}
	void BlendAnimation(const size_t index, const float weight, const float duration) {
	}
	void BlendAnimation(const size_t index, const float weight, const float start, const float end) {
	}*/

	void SetAnimationIndex(size_t animationIndex) {
		mAnimationIndex = animationIndex;
	}
	size_t GetAnimationIndex() const {
		return mAnimationIndex;
	}
	size_t GetAnimationCount() const {
		return mAnimationSet->mAnimations.size();
	}
	bool GetAnimationEnabled() const {
		return mAnimationIndex < GetAnimationCount();
	}
	Animation_ GetAnimation() const {
		return mAnimationIndex < GetAnimationCount() ? mAnimationSet->mAnimations[mAnimationIndex] : nullptr;
	}

	void Update(float absoluteTime) {
		if (absoluteTime < mNextUpdate) {
			return;
		}
		mNextUpdate = absoluteTime + mMinDelta;
		if (mBlended) {
			UpdateBlended(absoluteTime);
			return;
		}
		if (!GetAnimationEnabled()) {
			return;
		}
		ReadNodeHierarchy(mFinalTransforms, mAnimationIndex, absoluteTime);
	}

	void UpdateBlended(float absoluteTime) {
		std::fill(mFinalTransforms.begin(), mFinalTransforms.end(), glm::zero<glm::mat4>());

		for (const auto& it : mBlendMap) {
			const float weight = it.second;
			if (weight < 0.001f) continue;
			ReadNodeHierarchy(mBlendTransforms, it.first, absoluteTime);
			std::transform(std::execution::par, mFinalTransforms.begin(), mFinalTransforms.end(), mBlendTransforms.begin(), mFinalTransforms.begin(), [weight](glm::mat4& a, glm::mat4& b) {
				return a + b * weight;
			});
		}
	}

	void ReadNodeHierarchy(std::vector<glm::mat4>& finalTransforms, size_t index, float absoluteTime) {
		const auto& animation = mAnimationSet->mAnimations[index];
		const auto animationTime = animation->GetAnimationTime(absoluteTime);
		ReadNodeHierarchy(finalTransforms, animation, animationTime, animation->mRootNode, mAnimationSet->mGlobalInverseTransform);
	}

	void ReadNodeHierarchy(std::vector<glm::mat4>& finalTransforms, Animation_ animation, const float time, const AnimationNode_ node, const glm::mat4 parentTransform) {
		const auto nodeTransform = GetNodeTransform(animation, time, node);
		const auto combinedTransform = parentTransform * nodeTransform;

		const auto boneIndex = mAnimationSet->GetBoneIndex(node);
		if (boneIndex != -1) {
			finalTransforms[boneIndex] = combinedTransform * mAnimationSet->mBoneOffsets[boneIndex];
		}

		for (auto& childNode : node->mChildren) {
			ReadNodeHierarchy(finalTransforms, animation, time, childNode, combinedTransform);
		}
	}

	glm::quat mHeadRot = glm::identity<glm::quat>(); // FIXME

	virtual glm::mat4 GetNodeTransform(Animation_ animation, float time, const AnimationNode_ node) {
		// FIXME: Temp test
		if (node->mName == "Head") {
			glm::mat4 head = glm::toMat4(mHeadRot);
			return animation->GetNodeTransform(time, node) * head;
		}
		//std::cout << node->mName << std::endl;
		return animation->GetNodeTransform(time, node);
	}
};
typedef std::shared_ptr<AnimationController> AnimationController_;

