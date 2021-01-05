#pragma once

#include "Main.h"
#include "Model.h"

struct Entity {
	Model_ mModel = nullptr;
	glm::vec3 mPos = { 0,0,0 };
	Entity() {}
	Entity(Model_ model) : mModel(model) {}
	Entity(Model_ model, const glm::vec3& pos) : mModel(model), mPos(pos) {}
	void Update(float absoluteTime, float deltaTime) {
		if (mModel) {
			mModel->Update(absoluteTime, deltaTime);
		}
	}
};
typedef std::shared_ptr<Entity> Entity_;

struct Scene {
	std::vector<Entity_> mEntities;

	float mCameraDistance = 10.0f;
	float mCameraRotation = 0.0f;
	glm::vec3 mCameraCenter = { 0, 0, 0 };

	void Update(float absoluteTime, float deltaTime) {
		for (auto& entity : mEntities) {
			entity->Update(absoluteTime, deltaTime);
		}
	}
};
typedef std::shared_ptr<Scene> Scene_;
