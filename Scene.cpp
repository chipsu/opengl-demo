#include "Scene.h"
#include "Asset.h"

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


void Entity::Init(Scene& scene) {
	if (mModel && mModel->mAnimationSet) {
		mAnimationController = std::make_shared<AnimationController>(mModel->mAnimationSet);
	}
	mPrevPos = mPos;
	mPrevRot = mRot;
}

struct EntityMotionState : public virtual btMotionState {
	Entity* mEntity = nullptr;
	EntityMotionState(Entity* entity) : mEntity(entity) {}
	virtual void getWorldTransform(btTransform& worldTrans) const {
		worldTrans.setFromOpenGLMatrix((const btScalar*)&mEntity->mTransform[0]);
	}
	virtual void setWorldTransform(const btTransform& worldTrans) {
		worldTrans.getOpenGLMatrix((btScalar*)&mEntity->mTransform[0]);
		//auto inverted = glm::inverse(mEntity->mTransform);
		//mEntity->mFront = glm::normalize(glm::vec3(inverted[2]));
		mEntity->mRot = glm::quat_cast(mEntity->mTransform);
		mEntity->mPos = glm::vec3(mEntity->mTransform[3]);
		mEntity->mFront = glm::rotate(mEntity->mRot, glm::vec3(0, 0, 1));
		//mEntity->mFront = glm::rotate(glm::inverse(mEntity->mRot), glm::vec3(0, 0, -1));
		//mEntity->mRight = glm::rotate(glm::inverse(mEntity->mRot), glm::vec3(1, 0, 0));
		//mEntity->mUp = glm::vec3(0.0, 1.0, 0.0);
	}
};

void Entity::Load(Scene& scene, const rapidjson::Value& cfg) {
	if (cfg.HasMember("name")) mName = cfg["name"].GetString();
	mPos = ReadVec3(cfg, "position");
	mRot = glm::quat(glm::radians(ReadVec3(cfg, "rotation")));
	mScale = ReadVec3(cfg, "scale", mScale);
	mGravity = ReadVec3(cfg, "gravity", mGravity);
	mUseGravity = ReadBool(cfg, "useGravity");
	mControllable = ReadBool(cfg, "controllable");
	if (cfg.HasMember("attachTo")) {
		auto obj = cfg["attachTo"].GetObject();
		std::string entityName = obj["name"].GetString();
		mAttachTo = scene.Find(entityName);
		if (!mAttachTo) {
			std::cerr << "Warning: Entity " << entityName << " not found" << std::endl;
		}
		if (mAttachTo && obj.HasMember("node")) {
			std::string nodeName = obj["node"].GetString();
			mAttachToNode = mAttachTo->mModel->mAnimationSet->GetBoneIndex(nodeName);
			if (mAttachToNode == -1) {
				std::cerr << "Warning: Node " << nodeName << " not found" << std::endl;
			}
		}
	}
	if (cfg.HasMember("rigidBody")) {
		auto ms = new EntityMotionState(this);
		btCollisionShape* cs = nullptr;
		if (cfg["rigidBody"].IsArray()) {
			auto obj = cfg["rigidBody"].GetArray();
			std::string shape = obj[0].GetString();
			if (shape == "box") {
				cs = new btBoxShape(btVector3(obj[1].GetFloat(), obj[2].GetFloat(), obj[3].GetFloat()));
			} else if(shape == "sphere") {
				cs = new btSphereShape(obj[1].GetFloat());
			} else if(shape == "capsule") {
				cs = new btCapsuleShape(obj[1].GetFloat(), obj[2].GetFloat());
			} else {
				throw new std::invalid_argument("invalid rigidBody cfg");
			}
		} else {
			cs = new btCapsuleShape(0.5f, 1.0f);
		}
		mRigidBody = new btRigidBody(10.0f, ms, cs);
		scene.mDynamicsWorld->addRigidBody(mRigidBody);
	}
}

AssetMgr<Model>* gModelMgr = nullptr; // FIXXXXXXX

void ModelEntity::Load(Scene& scene, const rapidjson::Value& cfg) {
	Entity::Load(scene, cfg);
	mShaderProgram = ShaderProgram::Load("default");
	if (nullptr == gModelMgr) gModelMgr = new AssetMgr<Model>();
	std::string name = cfg["model"].GetString();
	mModel = gModelMgr->Load(name);
}

void ParticleEntity::Init(Scene& scene) {
	Entity::Init(scene);

	mShaderProgram = ShaderProgram::Load("particles");
	mModel = std::make_shared<Model>();
	auto mesh = std::make_shared<Mesh>();
	float s = 2.0f;
	float ps = 0.25f;
	float quad[] = {
		0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
	};
	for (float r = -3.14f; r < 3.14f; r += 3.14f / 8.0f) {
		float x = cos(r) * s;
		float y = sin(r) * s;
		for (int v = 0; v < 12; v += 2) {
			Vertex vertex;
			vertex.mPos = { x, y, 0 };
			vertex.mNormal.x = (quad[v] - 0.5f) * ps;
			vertex.mNormal.y = (quad[v + 1] - 0.5f) * ps;
			vertex.mNormal.z = 1.0f;
			mesh->mVertices.push_back(vertex);
			mesh->mIndices.push_back(mesh->mIndices.size());
		}
	}
	mModel->mMeshes.push_back(std::make_shared<ModelMesh>(mesh, glm::identity<glm::mat4>()));
}

void ParticleEntity::Load(Scene& scene, const rapidjson::Value& cfg) {
	Entity::Load(scene, cfg);
}

void ParticleEntity::Update(float absoluteTime, float deltaTime) {
	Entity::Update(absoluteTime, deltaTime);
	auto& mesh = mModel->mMeshes[0]->mMesh;
	float s = 2.0f * sin(absoluteTime);
	int a = 0;
	for (float r = -3.14f; r < 3.14f; r += 3.14f / 8.0f) {
		float x = cos(r + absoluteTime) * s;
		float y = sin(r + absoluteTime) * s;
		for (int v = 0; v < 6; ++v) {
			auto& vertex = mesh->mVertices[a + v];
			vertex.mPos = { x, y, 0 };
		}
		a += 6;
	}
	mesh->mVertexBufferDirty = true;
}

Scene::Scene() {
	Register("model", []() { return std::make_shared<ModelEntity>(); });
	Register("particle", []() { return std::make_shared<ParticleEntity>(); });

	auto cc = new btDefaultCollisionConfiguration();
	auto dispatcher = new btCollisionDispatcher(cc);
	auto opc = new btDbvtBroadphase();
	auto solver = new btSequentialImpulseConstraintSolver();
	mDynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, opc, solver, cc);
	mDynamicsWorld->setGravity(btVector3(0, -10, 0));

	auto groundShape = new btBoxShape(btVector3(100.0f, 0.1f, 100.0f));
	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0, -0.1f, 0));
	auto groundMS = new btDefaultMotionState(groundTransform);
	auto ground = new btRigidBody(0.0f, groundMS, groundShape);
	ground->forceActivationState(DISABLE_DEACTIVATION);
	ground->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT);
	mDynamicsWorld->addRigidBody(ground);
}

void Scene::Load(const std::string& fileName) {
	rapidjson::Document config;
	LoadJson(config, fileName);

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