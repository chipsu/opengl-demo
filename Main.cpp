#include "Main.h"
#include "Scene.h"
#include "Shader.h"
#include "Input.h"
#include "UI.h"

Input* Input::sInstance = nullptr;

Scene_ CreateScene(const int argc, const char** argv) {
	auto scene = std::make_shared<Scene>();
	bool loadModel = true;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		
		if (arg == "-s") {
			scene->Load(argv[++i]);
		} else if (arg == "-m") {
			loadModel = true;
		} else if (arg == "-a") {
			loadModel = false;
		} else {
			if (loadModel) {
				auto model = std::make_shared<Model>();
				model->Load(arg);
				scene->mEntities.push_back(std::make_shared<Entity>(model));
			} else {
				scene->mEntities.back()->mModel->LoadAnimation(arg, true);
			}
		}
	}
	scene->Init();
	return scene;
}

ShaderProgram_ CreateShaderProgram() {
	std::vector<Shader_> shaders;
	shaders.push_back(std::make_shared<Shader>("default.vert.glsl", GL_VERTEX_SHADER));
	shaders.push_back(std::make_shared<Shader>("default.geom.glsl", GL_GEOMETRY_SHADER));
	shaders.push_back(std::make_shared<Shader>("default.frag.glsl", GL_FRAGMENT_SHADER));
	return std::make_shared<ShaderProgram>(shaders);
}

float get_deque(void* data, int idx) {
	auto deque = (std::deque<float>*)data;
	return deque->at(idx);
}

struct Camera {
	glm::vec3 mPos = { 0,0,0 };
	glm::vec3 mFront = { 0,0,1 };
	glm::vec3 mUp = { 0,1,0 };
	glm::vec3 mLeft = { 1,0,0 };

	float mFov = glm::radians(45.0f);
	float mAspect = 1.0f;
	float mNear = 0.1f;
	float mFar = 1000.0f;

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
		mFov = width / (float)height;
	}

	void UpdateView() {
		mView = glm::lookAt(mPos, mPos + mFront, mUp);
	}

	void UpdateProjection() {
		mProjection = glm::perspective(mFov, mAspect, mNear, mFar);
	}
};

void RenderNode(GLint uniformModel, ModelNode_ node, const glm::mat4& parentTransform) {
	glm::mat4 transform = parentTransform * node->mTransform;
	for (auto& mesh : node->mMeshes) {
		if (mesh->mHidden) continue;
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, (GLfloat*)&transform[0]);
		mesh->Bind();
		glDrawElements(GL_TRIANGLES, mesh->mIndices.size(), GL_UNSIGNED_INT, 0);
	}
	for (auto& childNode : node->mChildren) {
		RenderNode(uniformModel, childNode, transform);
	}
};

int main(const int argc, const char **argv) {
	if (!glfwInit()) {
		std::cerr << "glfwInit failed" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	const std::string windowTitle = "OpenGL Animation Demo";

	auto window = glfwCreateWindow(1280, 720, windowTitle.c_str(), NULL, NULL);
	if (!window) {
		std::cerr << "glfwCreateWindow failed" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	auto scene = CreateScene(argc, argv);
	auto input = std::make_shared<Input>(window, scene);

	int windowWidth, windowHeight;
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);

	auto ui = std::make_shared<UI>(window);

	auto program = CreateShaderProgram();
	glUseProgram(program->mID);

	glEnable(GL_DEPTH_TEST);

	const GLuint uniformProj = glGetUniformLocation(program->mID, "uProj");
	const GLuint uniformView = glGetUniformLocation(program->mID, "uView");
	const GLuint uniformModel = glGetUniformLocation(program->mID, "uModel");
	const GLuint uniformBones = glGetUniformLocation(program->mID, "uBones");
	const GLuint uLightPos = glGetUniformLocation(program->mID, "uLightPos");
	const GLuint uViewPos = glGetUniformLocation(program->mID, "uViewPos");
	const GLuint uLightColor = glGetUniformLocation(program->mID, "uLightColor");
	
	bool useBlender = false;
	bool autoBlend = false;
	bool animDetails = false;
	bool modelDetails = true;

	std::vector<float> selectedWeights;

	glm::vec3 lightPos = { 100.0f, 100.0f, 100.0f };
	glm::vec3 lightColor = { 1.0f, 1.0f, 1.0f };

	Camera cam;
	cam.SetAspect(windowWidth, windowHeight);
	float movementSpeed = 1.0f;
	float camSpeed = 10.0f;

	FrameCounter<float> fps;
	fps.mHistoryLimit = 30;

	Timer<float> timer;

	while (!glfwWindowShouldClose(window)) {
		timer.Update();

		if (fps.Tick(timer.mNow)) {
			glfwSetWindowTitle(window, (windowTitle + " - FPS: " + std::to_string(fps.mValue)).c_str());
		}

		glfwSwapBuffers(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}

		if (nullptr != scene->mSelected) {
			movementSpeed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) ? 20.0f : 10.0f;

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				scene->mSelected->Walk(timer.mDelta * movementSpeed);
				// TODO: Rotate entity towards camera front perpendicular to ground plane (if rotating with MB2)
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				scene->mSelected->Walk(timer.mDelta * -movementSpeed);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				scene->mSelected->Strafe(timer.mDelta * -movementSpeed);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				scene->mSelected->Strafe(timer.mDelta * movementSpeed);
			if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && scene->mSelected->mPos.y < 1.0f)
				scene->mSelected->mVelocity.y = 100.0f;
			//if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			//	cam.Crounch();

			auto selectedCenter = scene->mSelected->mPos + scene->mSelected->mUp * scene->mSelected->mModel->mAABB.mHalfSize.y;
			const auto camOffset = selectedCenter + scene->mSelected->mFront * -scene->mCameraDistance;
			const auto camCenter = selectedCenter;
			const auto rotX = glm::rotate(glm::identity<glm::mat4>(), scene->mCameraRotationX, cam.mUp);
			const auto rotY = glm::rotate(glm::identity<glm::mat4>(), scene->mCameraRotationY, cam.mLeft);
			const auto posRot = rotX * rotY * glm::vec4(camOffset - camCenter, 1.0f);
			const auto targetPos = glm::vec3(posRot) + camCenter;

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
				cam.mPos = targetPos;
				cam.mFront = glm::normalize(selectedCenter - cam.mPos);
			} else {
				cam.mPos = glm::lerp(cam.mPos, targetPos, timer.mDelta * camSpeed);
				cam.mFront = glm::lerp(cam.mFront, glm::normalize(selectedCenter - cam.mPos), timer.mDelta * camSpeed);
				//cam.mFront = glm::lerp(cam.mFront, scene->mSelected->mFront, timer.mDelta * camSpeed);
			}
		}

		cam.UpdateView();
		cam.UpdateProjection();

		scene->Update(timer.mNow, timer.mDelta);

		ui->NewFrame();

		//ImGui::ShowDemoWindow();
		ImGui::PlotHistogram("FPS", get_deque, (void*)&fps.mHistory, fps.mHistory.size());

		auto selectedModel = scene->mSelected ? scene->mSelected->mModel : nullptr;
		if (selectedModel) {
			ImGui::Checkbox("Model info", &modelDetails);

			if (modelDetails) {
				ImGui::Text("Name: %s", selectedModel->mName.c_str());
				ImGui::Text("Model: c=%s, s=%s | length=%f",
					glm::to_string(selectedModel->mAABB.mCenter).c_str(),
					glm::to_string(selectedModel->mAABB.mHalfSize).c_str(),
					glm::length(selectedModel->mAABB.mHalfSize) * 2.0f
				);
				/*for (const auto& mesh : selectedModel->mMeshes) {
					ImGui::Text("Mesh: h=%d, v=%d, i=%d, c=%s, s=%s",
						mesh->mHidden,
						mesh->mVertices.size(),
						mesh->mIndices.size(),
						glm::to_string(mesh->mAABB.mCenter).c_str(),
						glm::to_string(mesh->mAABB.mHalfSize).c_str()
					);
				}*/
			}
		}

		if (scene->mSelected && scene->mSelected->mAnimationController) {
			const auto ac = scene->mSelected->mAnimationController;
			const auto as = ac->mAnimationSet;
			ImGui::Begin("Animations");
			ImGui::Text(ac->GetAnimationEnabled() ? ac->GetAnimation()->mName.c_str() : "DISABLED");

			ImGui::Checkbox("Animation info", &animDetails);
			if (animDetails) {
				for (const auto& anim : as->mAnimations) {
					ImGui::Text("Animation: %s, %f", anim->mName.c_str(), anim->mDuration);
				}
			}

			//ac->mHeadRot = glm::quatLookAt(cam.mFront, { 0, 0, 1 });
			double mx = 0;
			glfwGetCursorPos(window, &mx, nullptr);
			ac->mHeadRot = glm::rotate(glm::identity<glm::quat>(), (float)mx * 0.025f, { 0, 1, 0 });

			ImGui::Checkbox("Blend", &useBlender);
			if (useBlender) {
				ImGui::Checkbox("autoBlend", &autoBlend);

				// FIXME
				if (selectedWeights.size() != ac->GetAnimationCount()) {
					selectedWeights.resize(ac->GetAnimationCount());
					if (!selectedWeights.empty()) {
						std::fill(selectedWeights.begin(), selectedWeights.end(), 0);
						selectedWeights[0] = 1.0f;
					}
				}

				// FIXME
				if (autoBlend) {
					const auto animIdle = as->GetAnimationIndex("CharacterArmature|Idle");
					const auto animWalk = as->GetAnimationIndex("CharacterArmature|Walk");
					const auto animJump = as->GetAnimationIndex("CharacterArmature|Victory");
					const float animFadeIn = 20.0f;
					const float animFadeOut = 10.0f;

					for (auto& w : selectedWeights) {
						w = glm::clamp(w - timer.mDelta * animFadeOut, 0.0f, 1.0f);
					}

					// TODO: Move to ? & use velocity to choose animation
					if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
						selectedWeights[animWalk] = glm::clamp(selectedWeights[animWalk] + timer.mDelta * animFadeIn, 0.0f, 1.0f);
					} else {
						selectedWeights[animIdle] = glm::clamp(selectedWeights[animIdle] + timer.mDelta * animFadeIn, 0.0f, 1.0f);
					}

					if (scene->mSelected->mPos.y > 1.0f) {
						selectedWeights[animJump] = glm::clamp(selectedWeights[animJump] + timer.mDelta * animFadeIn, 0.0f, 1.0f);
					}
				}

				size_t animIndex = 0;
				for (const auto& anim : as->mAnimations) {
					ImGui::SliderFloat(("#" + std::to_string(animIndex) + " " + anim->mName).c_str(), &selectedWeights[animIndex], 0.0f, 1.0f);
					ac->BlendAnimation(animIndex, selectedWeights[animIndex]);
					animIndex++;
				}

				ac->UpdateBlended(timer.mNow); // FIXME
			}

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		glUniformMatrix4fv(uniformProj, 1, GL_FALSE, (GLfloat*)&cam.mProjection[0]);
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, (GLfloat*)&cam.mView[0]);
		glUniform3fv(uViewPos, 1, (GLfloat*)&cam.mPos[0]);
		glUniform3fv(uLightPos, 1, (GLfloat*)&lightPos[0]);
		glUniform3fv(uLightColor, 1, (GLfloat*)&lightColor[0]);

		for (auto& entity : scene->mEntities) {
			const auto& model = entity->mModel;
			if (!model) continue;
			if (entity->mAnimationController) {
				const auto& bones = entity->mAnimationController->mFinalTransforms;
				glUniformMatrix4fv(uniformBones, bones.size(), GL_FALSE, (GLfloat*)&bones[0]);
			}
			glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), entity->mPos);
			transform *= glm::mat4_cast(entity->mRot);
			transform = glm::scale(transform, entity->mScale);
			RenderNode(uniformModel, model->mRootNode, transform);
		}

		ui->Render();
	}

	input.reset();
	scene.reset();
	program.reset();

	glfwTerminate();
	return 0;
}
