#include "Scene.h"

using namespace rapidjson;

void Scene::Load(const std::string& fileName) {
	rapidjson::Document config;
	std::ifstream ifs(fileName);
	rapidjson::IStreamWrapper isw(ifs);
	config.ParseStream<kParseDefaultFlags | kParseCommentsFlag | kParseTrailingCommasFlag>(isw);

	for (const auto& cfg : config["entities"].GetArray()) {
		if (cfg.HasMember("disabled") && cfg["disabled"].GetBool()) continue;
		auto model = std::make_shared<Model>();
		auto entity = std::make_shared<Entity>(model);
		ModelOptions modelOptions;
		if (cfg.HasMember("modelOptions")) {
			const auto& opts = cfg["modelOptions"].GetObject();
			if (opts.HasMember("scale")) {
				modelOptions.mScale = opts["scale"].GetFloat();
			}
		}
		model->Load(cfg["model"].GetString(), modelOptions);
		if (cfg.HasMember("animations")) {
			for (const auto& anim : cfg["animations"].GetArray()) {
				model->LoadAnimation(anim.GetString(), modelOptions, true);
			}
		}
		if (cfg.HasMember("name")) {
			entity->mName = cfg["name"].GetString();
		}
		if (cfg.HasMember("position")) {
			const auto& pos = cfg["position"].GetArray();
			entity->mPos = { pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat() };
		}
		if (cfg.HasMember("rotation")) {
			const auto& pos = cfg["rotation"].GetArray();
			entity->mRot = glm::quat(glm::vec3(glm::radians(pos[0].GetFloat()), glm::radians(pos[1].GetFloat()), glm::radians(pos[2].GetFloat())));
		}
		if (cfg.HasMember("scale")) {
			const auto& pos = cfg["scale"].GetArray();
			entity->mScale = { pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat() };
		}
		entity->mControllable = cfg.HasMember("controllable") && cfg["controllable"].GetBool();
		entity->mUseGravity = cfg.HasMember("useGravity") && cfg["useGravity"].GetBool();

		if (cfg.HasMember("attachTo")) {
			auto obj = cfg["attachTo"].GetObject();
			auto attachTo = Find(obj["name"].GetString());
			assert(nullptr != attachTo);
			std::string nodeName = obj["node"].GetString();
			auto node = attachTo->mModel->mAnimationSet->GetBoneIndex(nodeName);
			assert(node != -1);
			entity->mAttachTo = attachTo;
			entity->mAttachToNode = node;
		}

		if (cfg.HasMember("array")) {
			auto arrayObj = cfg["array"].GetArray();
			float arraySpacing = cfg.HasMember("arraySpacing") ? cfg["arraySpacing"].GetFloat() : glm::length(model->mAABB.mHalfSize);
			for (size_t ax = 0; ax < arrayObj[0].GetInt(); ++ax) {
				for (size_t ay = 0; ay < arrayObj[1].GetInt(); ++ay) {
					for (size_t az = 0; az < arrayObj[2].GetInt(); ++az) {
						auto arrayEntity = entity->Clone();;
						arrayEntity->mPos += glm::vec3(ax * arraySpacing, ay * arraySpacing, az * arraySpacing);
						mEntities.push_back(arrayEntity);
					}
				}
			}
		} else {
			mEntities.push_back(entity);
		}
	}
}