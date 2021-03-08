#pragma once

#include "Main.h"

struct Camera {
	glm::vec3 mPos = { 0,0,0 };
	glm::vec3 mFront = { 0,0,1 };
	glm::vec3 mUp = { 0,1,0 };
	glm::vec3 mRight;

	float mFov = glm::radians(45.0f);
	float mAspect = 1.0f;
	float mNear = 0.1f;
	float mFar = 10000.0f;

	glm::mat4 mView;
	glm::mat4 mProjection;

	void Look(float yaw, float pitch) {
		glm::vec3 dir = {
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
			sin(glm::radians(pitch)),
			sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
		};
		mFront = glm::normalize(dir);
	}

	void SetAspect(int width, int height) {
		mAspect = width / (float)height;
	}

	void UpdateView() {
		mView = glm::lookAt(mPos, mPos + mFront, mUp);
	}

	void UpdateProjection() {
		mProjection = glm::perspective(mFov, mAspect, mNear, mFar);
	}
};
