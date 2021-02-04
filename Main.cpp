#include "Main.h"
#include "Scene.h"
#include "Shader.h"
#include "Input.h"
#include "UI.h"

//extern "C" {
//	_declspec(dllexport) unsigned long NvOptimusEnablement = 1;
//	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
//}

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
	//shaders.push_back(std::make_shared<Shader>("default.geom.glsl", GL_GEOMETRY_SHADER));
	shaders.push_back(std::make_shared<Shader>("default.frag.glsl", GL_FRAGMENT_SHADER));
	return std::make_shared<ShaderProgram>(shaders);
}

ShaderProgram_ CreateDebugShaderProgram() {
	std::vector<Shader_> shaders;
	shaders.push_back(std::make_shared<Shader>("debug.vert.glsl", GL_VERTEX_SHADER));
	//shaders.push_back(std::make_shared<Shader>("debug.geom.glsl", GL_GEOMETRY_SHADER));
	shaders.push_back(std::make_shared<Shader>("debug.frag.glsl", GL_FRAGMENT_SHADER));
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
		mFov = width / (float)height;
	}

	void UpdateView() {
		mView = glm::lookAt(mPos, mPos + mFront, mUp);
	}

	void UpdateProjection() {
		mProjection = glm::perspective(mFov, mAspect, mNear, mFar);
	}
};

size_t gMeshCounter = 0;

void RenderNode(GLint uniformModel, ModelNode_ node, const glm::mat4& parentTransform) {
	glm::mat4 transform = parentTransform * node->mTransform;
	bool transformSet = false;
	for (auto& mesh : node->mMeshes) {
		if (mesh->mHidden) continue;
		if (!transformSet) {
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, (GLfloat*)&transform[0]);
			transformSet = true;
		}
		mesh->Bind();
		glDrawElements(GL_TRIANGLES, mesh->mIndices.size(), GL_UNSIGNED_INT, 0);
		gMeshCounter++;
	}
	for (auto& childNode : node->mChildren) {
		RenderNode(uniformModel, childNode, transform);
	}
};

struct DebugLine {
	glm::vec3 mStart;
	glm::vec3 mEnd;
	glm::vec3 mColor;
	float mScale = 1.0f;
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
	
	auto fullscreen = false;

	auto monitor = fullscreen ? glfwGetPrimaryMonitor() : NULL;
	auto window = glfwCreateWindow(1280, 720, windowTitle.c_str(), monitor, NULL);
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
	glfwSwapInterval(0);

	auto ui = std::make_shared<UI>(window);

	auto program = CreateShaderProgram();
	auto debugProgram = CreateDebugShaderProgram();

	const GLuint uniformProj = glGetUniformLocation(program->mID, "uProj");
	const GLuint uniformView = glGetUniformLocation(program->mID, "uView");
	const GLuint uniformModel = glGetUniformLocation(program->mID, "uModel");
	const GLuint uniformBones = glGetUniformLocation(program->mID, "uBones");
	const GLuint uLightPos = glGetUniformLocation(program->mID, "uLightPos");
	const GLuint uViewPos = glGetUniformLocation(program->mID, "uViewPos");
	const GLuint uLightColor = glGetUniformLocation(program->mID, "uLightColor");
	
	bool autoBlend = false;
	bool animDetails = false;
	bool modelDetails = true;
	bool enableDebug = true;
	bool headRot = false;

	std::vector<float> selectedWeights;

	glm::vec3 lightPos = { 100.0f, 100.0f, 100.0f };
	glm::vec3 lightColor = { 1.0f, 1.0f, 1.0f };

	std::vector<DebugLine> debugLines;

	Camera cam;
	cam.mRight = glm::normalize(glm::cross(cam.mUp, cam.mFront));
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
		glEnable(GL_DEPTH_TEST);

		glUseProgram(program->mID);

		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}

		auto selected = scene->mSelected;
		if (nullptr != selected) {
			movementSpeed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) ? 100.0f : 40.0f;

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				auto targetFront = glm::cross(cam.mRight, selected->mUp);
				targetFront = glm::normalize(targetFront);
				selected->mFront = targetFront;
				// FIXME
				float turnSpeed = 10.0f;
				selected->mRot = glm::slerp(selected->mRot, glm::quatLookAt(-targetFront, selected->mUp), timer.mDelta * turnSpeed);
				//selected->mFront = selected->mDebugFront;
				scene->mSelected->Walk(timer.mDelta * movementSpeed);

				// SELECTED->mFront !?!?!?
				// TODO: Rotate entity towards camera front perpendicular to ground plane (if rotating with MB2)

				//selected->mFront = targetFront;
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

			bool rot = false;
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { rot = true; scene->mCameraRotationX = -0.1f; }
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {rot=true;scene->mCameraRotationX = 0.1f;}
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {rot=true;scene->mCameraRotationY = 0.1f;
}			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {rot=true;scene->mCameraRotationY = -0.1f;}
			
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS ||rot) {
				auto rotY = glm::angleAxis(scene->mCameraRotationX * timer.mDelta, glm::vec3(0, 1, 0));
				auto rotX = glm::angleAxis(scene->mCameraRotationY * timer.mDelta, cam.mRight);
				// TODO: limit X
				auto targetPos2 = glm::normalize(rotY * rotX * cam.mFront);
				targetPos2 = selectedCenter + targetPos2 * -scene->mCameraDistance;

				// FIXME
				scene->mCameraRotationX = 0;
				scene->mCameraRotationY = 0;

				cam.mPos = targetPos2;
				debugLines.push_back({ selectedCenter , cam.mPos, {1,1,1} });
				cam.mFront = glm::normalize(selectedCenter - cam.mPos);
				cam.mRight = glm::normalize(glm::cross(cam.mUp, cam.mFront));
			} else {
				auto targetPos = selectedCenter + cam.mFront * -scene->mCameraDistance;
				cam.mPos = glm::lerp(cam.mPos, targetPos, timer.mDelta * camSpeed);
				cam.mFront = glm::lerp(cam.mFront, glm::normalize(selectedCenter - cam.mPos), timer.mDelta * camSpeed); // TODO norm
				cam.mRight = glm::normalize(glm::cross(cam.mUp, cam.mFront));
			}
		}

		if (enableDebug && selected) {
			debugLines.push_back({ selected->mPos, selected->mPos + cam.mFront, { 1, 1, 1 } });
			debugLines.push_back({ selected->mPos, selected->mPos + selected->mFront, { 1, 0, 0 } });
			debugLines.push_back({ selected->mPos, selected->mPos + selected->mUp, { 0, 1, 0 } });
		}

		ui->NewFrame();

		//ImGui::ShowDemoWindow();
		//ImGui::PlotHistogram("FPS", get_deque, (void*)&fps.mHistory, fps.mHistory.size());

		auto selectedModel = scene->mSelected ? scene->mSelected->mModel : nullptr;
		if (selectedModel) {
			ImGui::Checkbox("debug", &enableDebug);
			ImGui::Checkbox("Model info", &modelDetails);
			ImGui::Text(std::to_string(scene->mCameraRotationX).c_str());
			ImGui::Text(std::to_string(scene->mCameraRotationY).c_str());

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
			if (ImGui::TreeNode("ModelNodes")) {
				auto renderTree = [](ModelNode_ node, auto renderTree) -> void {
					if (ImGui::TreeNode(node->mName.c_str())) {
						ImGui::Text(glm::to_string(node->mTransform).c_str());
						for (auto& c : node->mChildren) {
							renderTree(c, renderTree);
						}
						ImGui::TreePop();
					}
				};
				renderTree(selectedModel->mRootNode, renderTree);
				ImGui::TreePop();
			}
		}

		ImGui::Text("GL_VENDOR: %s", glGetString(GL_VENDOR));
		ImGui::Text("GL_RENDERER: %s", glGetString(GL_RENDERER));
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (selected && selected->mAnimationController) {
			const auto ac = selected->mAnimationController;
			const auto as = ac->mAnimationSet;
			ImGui::Begin("Animations");
			ImGui::Text(ac->GetAnimationEnabled() ? ac->GetAnimation()->mName.c_str() : "DISABLED");

			ImGui::Checkbox("Animation info", &animDetails);
			if (animDetails) {
				for (const auto& anim : as->mAnimations) {
					ImGui::Text("Animation: %s, %f", anim->mName.c_str(), anim->mDuration);
				}
			}

			ImGui::Checkbox("Blend", &ac->mBlended);
			if (ac->mBlended) {
				ImGui::Checkbox("HeadRot", &headRot);
				if (headRot) {
					//ac->mHeadRot = glm::quatLookAt(cam.mFront, { 0, 0, 1 });
					double mx = 0;
					glfwGetCursorPos(window, &mx, nullptr);
					ac->mHeadRot = glm::rotate(glm::identity<glm::quat>(), (float)mx * 0.025f, { 0, 1, 0 });
				}
				else {
					ac->mHeadRot = glm::identity<glm::quat>();
				}

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
			}

			if (ImGui::CollapsingHeader("Stuff")) {
				ImGui::End();
			}

			if (ImGui::TreeNode("AnimationNodes")) {
				auto renderTree = [](AnimationNode_ node, auto renderTree) -> void {
					if (ImGui::TreeNode(node->mName.c_str())) {
						for (auto& c : node->mChildren) {
							renderTree(c, renderTree);
						}
						ImGui::TreePop();
					}
				};
				renderTree(as->mAnimations[0]->mRootNode, renderTree);
				ImGui::TreePop();
			}

			ImGui::End();
		}

		cam.UpdateView();
		cam.UpdateProjection();

		scene->Update(timer.mNow, timer.mDelta);

		glUniformMatrix4fv(uniformProj, 1, GL_FALSE, (GLfloat*)&cam.mProjection[0]);
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, (GLfloat*)&cam.mView[0]);
		glUniform3fv(uViewPos, 1, (GLfloat*)&cam.mPos[0]);
		glUniform3fv(uLightPos, 1, (GLfloat*)&lightPos[0]);
		glUniform3fv(uLightColor, 1, (GLfloat*)&lightColor[0]);

		gMeshCounter = 0;
		for (auto& entity : scene->mEntities) {
			const auto& model = entity->mModel;
			if (!model) continue;
			if (entity->mAnimationController) {
				const auto& bones = entity->mAnimationController->mFinalTransforms;
				glUniformMatrix4fv(uniformBones, bones.size(), GL_FALSE, (GLfloat*)&bones[0]);
			}
			for (auto& modelMesh : model->mMeshes) {
				if (modelMesh->mMesh->mHidden) continue;
				glm::mat4 meshTransform = entity->mTransform * modelMesh->mTransform;
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, (GLfloat*)&meshTransform[0]);
				modelMesh->mMesh->Bind();
				glDrawElements(GL_TRIANGLES, modelMesh->mMesh->mIndices.size(), GL_UNSIGNED_INT, 0);
			}
		}

		if (enableDebug) {
			glDisable(GL_DEPTH_TEST);
			glUseProgram(debugProgram->mID);
			glUniformMatrix4fv(glGetUniformLocation(debugProgram->mID, "uProj"), 1, GL_FALSE, (GLfloat*)&cam.mProjection[0]);
			glUniformMatrix4fv(glGetUniformLocation(debugProgram->mID, "uView"), 1, GL_FALSE, (GLfloat*)&cam.mView[0]);
			for (auto& debugLine : debugLines) {
				auto debugMesh = std::make_shared<Mesh>();
				debugMesh->mIndices.push_back(0);
				debugMesh->mIndices.push_back(1);
				debugMesh->mVertices.push_back({
					debugLine.mStart,
					{},
					debugLine.mColor
					});
				debugMesh->mVertices.push_back({
					debugLine.mEnd + (debugLine.mEnd - debugLine.mStart) * debugLine.mScale,
					{},
					debugLine.mColor
					});
				debugMesh->Bind();
				glDrawElements(GL_LINES, debugMesh->mIndices.size(), GL_UNSIGNED_INT, 0);
			}
		}
		debugLines.clear();

		ui->Render();
	}

	input.reset();
	scene.reset();
	program.reset();

	glfwTerminate();
	return 0;
}
