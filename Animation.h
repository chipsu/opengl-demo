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

	AnimationNode(const std::string& name, AnimationNode_ parent, const glm::mat4& transform) : mName(name), mParent(parent), mTransform(transform) {
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

	void ReadNodeHierarchy(std::vector<glm::mat4>& finalTransforms, size_t index, float absoluteTime) {
		const auto& animation = mAnimations[index];
		const auto animationTime = animation->GetAnimationTime(absoluteTime);
		ReadNodeHierarchy(finalTransforms, animation, animationTime, animation->mRootNode, mGlobalInverseTransform);
	}

	void ReadNodeHierarchy(std::vector<glm::mat4>& finalTransforms, Animation_ animation, const float time, const AnimationNode_ node, const glm::mat4 parentTransform) {
		const auto nodeTransform = animation->GetNodeTransform(time, node);
		const auto combinedTransform = parentTransform * nodeTransform;

		const auto it = mBoneMappings.find(node->mName); // TODO: node->mBoneIndex?
		if (it != mBoneMappings.end()) {
			const auto boneIndex = it->second;
			finalTransforms[boneIndex] = combinedTransform * mBoneOffsets[boneIndex];
		}

		for (auto& childNode : node->mChildren) {
			ReadNodeHierarchy(finalTransforms, animation, time, childNode, combinedTransform);
		}
	}
};
typedef std::shared_ptr<AnimationSet> AnimationSet_;

struct AnimationController {
	AnimationSet_ mAnimationSet;
	size_t mAnimationIndex = -1;
	std::vector<glm::mat4> mFinalTransforms;

	AnimationController(AnimationSet_ animationSet) {
		mAnimationSet = animationSet;
	}

	/*
	Hmm, idk how this works
	move this to own controller maybe. some sequencer thing
		animation bone priorities:
		- walking + attack -> legs walking, arms attacking
		- walking + jump -> jump prio
		ik..

	*/
	float mBlendInDuration = 0.5f;
	float mBlendOut = 0.5f;
	std::map<size_t, float> mBlendMap;

	void BlendAnimation(const size_t index, const float weight) {
		assert(index != -1);
		mBlendMap[index] = weight;
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
		if (!GetAnimationEnabled()) {
			return;
		}
		mFinalTransforms.resize(mAnimationSet->mBoneMappings.size()); // FIXME
		ReadNodeHierarchy(mAnimationIndex, absoluteTime);
	}

	std::vector<glm::mat4> mBlendedTransforms;

	void UpdateBlended(float absoluteTime) {
		mBlendedTransforms.resize(mAnimationSet->mBoneMappings.size()); // FIXME
		std::fill(mBlendedTransforms.begin(), mBlendedTransforms.end(), glm::zero<glm::mat4>());

		for (const auto& it : mBlendMap) {
			const float weight = it.second;
			if (weight < 0.001f) continue;
			ReadNodeHierarchy(it.first, absoluteTime);
			for (size_t x = 0; x < mFinalTransforms.size(); ++x) {
				mBlendedTransforms[x] += mFinalTransforms[x] * weight;
			}
		}
	}

	void ReadNodeHierarchy(size_t index, float absoluteTime) {
		mAnimationSet->ReadNodeHierarchy(mFinalTransforms, index, absoluteTime);
	}
};
typedef std::shared_ptr<AnimationController> AnimationController_;
