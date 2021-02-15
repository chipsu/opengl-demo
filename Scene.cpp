#include "Scene.h"

using namespace rapidjson;

bool ReadBool(const rapidjson::Value& cfg, const char* key, bool def = false) {
	if (cfg.HasMember(key)) return cfg[key].GetBool();
	return def;
}

glm::vec3 ReadVec3(const rapidjson::Value& cfg, const char* key, const glm::vec3& def = { 0,0,0 }) {
	if (cfg.HasMember(key)) {
		const auto& val = cfg[key].GetArray();
		return { val[0].GetFloat(), val[1].GetFloat(), val[2].GetFloat() };
	}
	return def;
}

void Entity::Load(Scene& scene, const rapidjson::Value& cfg) {
	if (cfg.HasMember("name")) mName = cfg["name"].GetString();
	mPos = ReadVec3(cfg, "position");
	mRot = glm::quat(ReadVec3(cfg, "rotation"));
	mScale = ReadVec3(cfg, "scale", mScale);
	mGravity = ReadVec3(cfg, "gravity", mGravity);
	mUseGravity = ReadBool(cfg, "useGravity");
	mControllable = ReadBool(cfg, "controllable");
}

void ModelEntity::Load(Scene& scene, const rapidjson::Value& cfg) {
	Entity::Load(scene, cfg);
	mShaderProgram = ShaderProgram::Load("default");
	mModel = std::make_shared<Model>();
	ModelOptions modelOptions;
	if (cfg.HasMember("modelOptions")) {
		const auto& opts = cfg["modelOptions"].GetObject();
		if (opts.HasMember("scale")) {
			modelOptions.mScale = opts["scale"].GetFloat();
		}
	}
	mModel->Load(cfg["model"].GetString(), modelOptions);
	if (cfg.HasMember("animations")) {
		for (const auto& anim : cfg["animations"].GetArray()) {
			mModel->LoadAnimation(anim.GetString(), modelOptions, true);
		}
	}
	if (cfg.HasMember("attachTo")) {
		auto obj = cfg["attachTo"].GetObject();
		auto attachTo = scene.Find(obj["name"].GetString());
		assert(nullptr != attachTo);
		std::string nodeName = obj["node"].GetString();
		auto node = attachTo->mModel->mAnimationSet->GetBoneIndex(nodeName);
		assert(node != -1);
		mAttachTo = attachTo;
		mAttachToNode = node;
	}
}

void ParticleEntity::Load(Scene& scene, const rapidjson::Value& cfg) {
	Entity::Load(scene, cfg);
	mShaderProgram = ShaderProgram::Load("particles");
	mModel = std::make_shared<Model>();
	auto mesh = std::make_shared<Mesh>();
	float s = 2.0f;
	float ps = 0.25f;
	float quad[] = {
		0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
	};
	for (float r = -3.14f; r < 3.14f; r += 3.14f/8.0f) {
		float x = cos(r) * s;
		float y = sin(r) * s;
		for (int v = 0; v < 12; v += 2) {
			Vertex vertex;
			vertex.mPos.x = (quad[v] - 0.5f) * ps;
			vertex.mPos.y = (quad[v + 1] - 0.5f) * ps;
			vertex.mNormal = { x,y,0 };
			mesh->mVertices.push_back(vertex);
			mesh->mIndices.push_back(mesh->mIndices.size());
		}
	}
	mModel->mMeshes.push_back(std::make_shared<ModelMesh>(mesh, glm::identity<glm::mat4>()));
}

Scene::Scene() {
	Register("model", []() { return std::make_shared<ModelEntity>(); });
	Register("particle", []() { return std::make_shared<ParticleEntity>(); });
}

void Scene::Load(const std::string& fileName) {
	rapidjson::Document config;
	std::ifstream ifs(fileName);
	rapidjson::IStreamWrapper isw(ifs);
	config.ParseStream<kParseDefaultFlags | kParseCommentsFlag | kParseTrailingCommasFlag>(isw);

	for (const auto& cfg : config["entities"].GetArray()) {
		if (cfg.HasMember("disabled") && cfg["disabled"].GetBool()) continue;
		auto type = cfg.HasMember("type") ? cfg["type"].GetString() : "model";
		auto ctor = mTypes[type];
		auto entity = ctor();
		entity->Load(*this, cfg);

		if (cfg.HasMember("array")) {
			auto arrayObj = cfg["array"].GetArray();
			float arraySpacing = cfg.HasMember("arraySpacing") ? cfg["arraySpacing"].GetFloat() : glm::length(entity->mModel->mAABB.mHalfSize);
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