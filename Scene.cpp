#include "Scene.h"

void Scene::Load(const std::string& fileName) {
	rapidjson::Document config;
	std::ifstream ifs(fileName);
	rapidjson::IStreamWrapper isw(ifs);
	config.ParseStream(isw);

	for (const auto& cfg : config["entities"].GetArray()) {
		if (cfg.HasMember("disabled") && cfg["disabled"].GetBool()) continue;
		auto model = std::make_shared<Model>();
		auto entity = std::make_shared<Entity>(model);
		model->Load(cfg["model"].GetString());
		if (cfg.HasMember("animations")) {
			for (const auto& anim : cfg["animations"].GetArray()) {
				model->LoadAnimation(anim.GetString());
			}
		}
		if (cfg.HasMember("position")) {
			const auto& pos = cfg["position"].GetArray();
			entity->mPos = { pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat() };
		}
		mEntities.push_back(entity);
	}
}