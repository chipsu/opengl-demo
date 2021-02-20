#pragma once

#include "Main.h"
#include "Scene.h"

struct Input {
	static Input* sInstance;
	GLFWwindow* mWindow;
	Scene_ mScene;
	size_t mDebugMesh = 0;
	float mNow = 0;
	float mDelta = 0;

	Input(GLFWwindow* window, Scene_ scene) : mWindow(window), mScene(scene) {
		sInstance = this;
		glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			Input::sInstance->OnKey(window, key, scancode, action, mods);
		});
		glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
			Input::sInstance->OnMousePos(window, xpos, ypos);
		});
		glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
			Input::sInstance->OnMouseButton(window, button, action, mods);
		});
		glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
			Input::sInstance->OnMouseScroll(window, xoffset, yoffset);
		});
	}

	~Input() {
		glfwSetKeyCallback(mWindow, nullptr);
		glfwSetCursorPosCallback(mWindow, nullptr);
		glfwSetMouseButtonCallback(mWindow, nullptr);
		glfwSetScrollCallback(mWindow, nullptr);
		sInstance = nullptr;
	}

	void SetDebugMesh(Entity_ entity, size_t index) {
		//auto& meshes = entity->mModel->mMeshes;
		//mDebugMesh = index;

		//if (mDebugMesh >= meshes.size()) {
		//	mDebugMesh = -1;
		//}

		//if (mDebugMesh == -1) {
		//	std::cout << "DEBUG: Show all meshes" << std::endl;
		//	for (auto& mesh : meshes) {
		//		mesh->mHidden = false;
		//	}
		//	//mScene->mCameraCenter = entity->mModel->mAABB.mCenter; // TODO OFFSET
		//}
		//else {
		//	std::cout << "DEBUG: Show mesh " << mDebugMesh << std::endl;
		//	for (auto& mesh : meshes) {
		//		mesh->mHidden = true;
		//	}
		//	meshes[mDebugMesh]->mHidden = false;
		//	//mScene->mCameraCenter = meshes[mDebugMesh]->mAABB.mCenter; // TODO OFFSET
		//}
	}

	float mLastCast = 0;
	void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (false) printf("key_callback: %d %d %d\n", key, action, mods);
		if (action == GLFW_RELEASE) {
			auto& entity = mScene->mSelected;
			if (nullptr != entity) {
				auto& animationController = entity->mAnimationController;
				if (key == GLFW_KEY_LEFT) {
					SetDebugMesh(entity, mDebugMesh - 1);
				}
				if (key == GLFW_KEY_RIGHT) {
					SetDebugMesh(entity, mDebugMesh + 1);
				}
				if (key == GLFW_KEY_UP) {
					SetDebugMesh(entity, -1);
				}
				if (key == GLFW_KEY_ENTER) {
					size_t animationIndex = animationController->GetAnimationIndex() + 1;
					if (animationIndex >= animationController->GetAnimationCount()) animationIndex = -1;
					animationController->SetAnimationIndex(animationIndex);
					const auto animation = animationController->GetAnimation();
					std::cout << "Animation: " << animationIndex << ", " << (animation ? animation->mName : "DISABLED") << std::endl;
				}
				if (mLastCast + 1.0f < mNow && key == GLFW_KEY_1) {
					auto spawn = std::make_shared<ParticleEntity>();
					spawn->mPos = entity->mPos;
					spawn->mRot = entity->mRot;
					spawn->mForce = entity->mFront * 10.0f - entity->mGravity * 2.0f;
					spawn->mUseGravity = true;
					spawn->Init();
					mScene->mEntities.push_back(spawn);
					mLastCast = mNow;
				}
			}
			if (key == GLFW_KEY_TAB) {
				mScene->SelectNext();
			}
		}
	}

	double mMouseX = 0;
	double mMouseY = 0;
	float mRotateSpeedX = 2.0f;
	float mRotateSpeedY = 2.0f;
	float mLimitY = glm::half_pi<float>() * 0.9f;

	void OnMousePos(GLFWwindow* window, double xpos, double ypos) {
		if (false) printf("cursor_position_callback: %f %f\n", xpos, ypos);
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			mScene->mCameraRotationX = (mMouseX - xpos) * mRotateSpeedX;
			mScene->mCameraRotationY = (mMouseY - ypos) * mRotateSpeedY;
			//mScene->mCameraRotationX += (mMouseX - xpos) * mRotateSpeedX;
			//mScene->mCameraRotationY += (mMouseY - ypos) * mRotateSpeedY;
			//mScene->mCameraRotationY = glm::clamp(mScene->mCameraRotationY, -mLimitY, mLimitY);
			//std::cout << mScene->mCameraRotationX << ", " << mScene->mCameraRotationY << std::endl;
			//glfwSetCursorPos(window, mStartX, mStartY);
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			mScene->mCameraRotationX = 0.0f;
			mScene->mCameraRotationY = 0.0f;
		}
		mMouseX = xpos;
		mMouseY = ypos;
	}

	void OnMouseButton(GLFWwindow* window, int button, int action, int mods) {
		if (false) printf("mouse_button_callback: %d %d %d\n", button, action, mods);
	}

	void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
		if (false) printf("scroll_callback: %f %f\n", xoffset, yoffset);
		const float scale = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 1.0f : 0.1f;
		mScene->mCameraDistance -= (float)yoffset * scale;
		mScene->mCameraDistance = glm::clamp(mScene->mCameraDistance, 1.0f, 1000.0f);
		//printf("SCROLL: %f\n", mScene->mCameraDistance);
	}
};
