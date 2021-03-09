#include "Main.h"
#include "Scene.h"
#include "Shader.h"
#include "Input.h"
#include "UI.h"
#include "Debug.h"
#include "Camera.h"

#ifdef USE_HIGH_PERFORMANCE_GPU
extern "C" {
	_declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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

float get_deque(void* data, int idx) {
	auto deque = (std::deque<float>*)data;
	return deque->at(idx);
}

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

	bool autoBlend = false;
	bool animDetails = false;
	bool modelDetails = true;
	bool enableDebug = true;
	bool drawDebug = true;
	bool drawPersistentDebug = true;
	bool headRot = false;

	std::vector<float> selectedWeights;

	glm::vec3 lightPos = { 100.0f, 100.0f, 100.0f };
	glm::vec3 lightColor = { 1.0f, 1.0f, 1.0f };

	DebugRenderer debugRenderer;
	DebugRenderer persistentDebugRenderer;

	persistentDebugRenderer.AddGrid(1.0f, 10.0f, { .5f, .5f, .5f });

	Camera cam;
	cam.mRight = glm::normalize(glm::cross(cam.mUp, cam.mFront));
	cam.SetAspect(windowWidth, windowHeight);
	float movementSpeed = 1.0f;
	float camSpeed = 10.0f;

	FrameCounter<float> fps;
	fps.mHistoryLimit = 600;

	Timer<float> timer;
	while (!glfwWindowShouldClose(window)) {
		timer.Update();
		input->mNow = timer.mNow;
		input->mDelta = timer.mDelta;

		if (fps.Tick(timer.mNow, timer.mDelta)) {
			glfwSetWindowTitle(window, (windowTitle + " - FPS: " + std::to_string(fps.mValue)).c_str());
		}

		glfwSwapBuffers(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}

		auto selected = scene->mSelected;
		if (nullptr != selected) {
			movementSpeed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) ? 10.0f : 2.0f;
			float walk = 0;
			float strafe = 0;

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				walk = movementSpeed;
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				walk = -movementSpeed;
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				strafe = -movementSpeed;
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				strafe = movementSpeed;

			if (walk != 0) {
				scene->mSelected->Move(selected->mFront * movementSpeed);
				debugRenderer.AddLine({ selected->mPos, selected->mPos + selected->mFront * 3.0f, {1,0,1} });
			}

			if (strafe != 0) {
				scene->mSelected->Strafe(strafe);
			}

			if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
				scene->mSelected->Jump();
			//if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			//	cam.Crounch();

			auto selectedCenter = scene->mSelected->mPos + scene->mSelected->mUp * scene->mSelected->mModel->mAABB.mHalfSize.y;
			
			bool rotPlayer = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS;
			bool rot = rotPlayer || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { rot = true; scene->mCameraRotationX = -0.1f; }
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {rot=true;scene->mCameraRotationX = 0.1f;}
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {rot=true;scene->mCameraRotationY = 0.1f;
}			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {rot=true;scene->mCameraRotationY = -0.1f;}
			
			if (rot) {
				auto rotY = glm::angleAxis(scene->mCameraRotationX * timer.mDelta, glm::vec3(0, 1, 0));
				auto rotX = glm::angleAxis(scene->mCameraRotationY * timer.mDelta, cam.mRight);
				// TODO: limit X
				auto targetPos2 = glm::normalize(rotY * rotX * cam.mFront);
				targetPos2 = selectedCenter + targetPos2 * -scene->mCameraDistance;

				// FIXME
				scene->mCameraRotationX = 0;
				scene->mCameraRotationY = 0;

				cam.mPos = targetPos2;
				debugRenderer.AddLine({ selectedCenter , cam.mPos, {1,1,1} });
				cam.mFront = glm::normalize(selectedCenter - cam.mPos);
				cam.mRight = glm::normalize(glm::cross(cam.mUp, cam.mFront));
			}
			else {
				// TODO: Fixed pos & rot from player
				auto targetPos = selectedCenter + cam.mFront * -scene->mCameraDistance;
				cam.mPos.y = targetPos.y;
				cam.mPos = glm::lerp(cam.mPos, targetPos, timer.mDelta * camSpeed);
				cam.mFront = glm::lerp(cam.mFront, glm::normalize(selectedCenter - cam.mPos), timer.mDelta * camSpeed); // TODO norm
				cam.mRight = glm::normalize(glm::cross(cam.mUp, cam.mFront));
			}

			if (rotPlayer) {
				selected->mTargetFront = glm::normalize(glm::cross(cam.mRight, selected->mUp));
				selected->mTargetFrontEnable = true;
			}
		}

		if (enableDebug && selected) {
			debugRenderer.AddLine({ selected->mPos, selected->mPos + cam.mFront, { 1, 1, 1 } });
			debugRenderer.AddLine({ selected->mPos, selected->mPos + selected->mFront, { 1, 0, 0 } });
			debugRenderer.AddLine({ selected->mPos, selected->mPos + selected->mUp, { 0, 1, 0 } });
			debugRenderer.AddPoint(selected->mPos);
			if (selected->mModel) {
				debugRenderer.AddCube(selected->mPos, selected->mModel->mAABB, { 1, 1, 0 });
			}
		}

		ui->NewFrame();

		//ImGui::ShowDemoWindow();
		ImGui::PlotHistogram("FPS", get_deque, (void*)&fps.mHistory, fps.mHistory.size(), 0, NULL, FLT_MAX, FLT_MAX, ImVec2(600, 100));
		//ImGui::PlotHistogram("DT", get_deque, (void*)&fps.mFrameTimeHistory, fps.mFrameTimeHistory.size(), 0, NULL, FLT_MAX, FLT_MAX, ImVec2(600, 100));

		if (selected) {
			ImGui::PlotHistogram("Y", get_deque, (void*)&selected->mHistoryY, selected->mHistoryY.size(), 0, NULL, FLT_MAX, FLT_MAX, ImVec2(600, 100));
		}

		ImGui::Checkbox("debug", &enableDebug);
		if (enableDebug) {
			ImGui::Checkbox("drawDebug", &drawDebug);
			ImGui::Checkbox("drawPersistentDebug", &drawPersistentDebug);
		}

		auto selectedModel = scene->mSelected ? scene->mSelected->mModel : nullptr;
		if (selectedModel) {
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
			if (selectedModel->HasAnimations() && ImGui::TreeNode("AnimationNodes")) {
				auto renderTree = [](AnimationNode_ node, auto renderTree) -> void {
					if (ImGui::TreeNode(node->mName.c_str())) {
						ImGui::Text(glm::to_string(node->mTransform).c_str());
						for (auto& c : node->mChildren) {
							renderTree(c, renderTree);
						}
						ImGui::TreePop();
					}
				};
				renderTree(selectedModel->mAnimationSet->mAnimations[0]->mRootNode, renderTree);
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

		//cam.mView = glm::lookAt(glm::vec3(4.0f,4.0f,4.0f), glm::vec3(0.0f,2.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
		cam.UpdateView();
		cam.UpdateProjection();

		scene->Update(timer.mNow, timer.mDelta);

		ShaderProgram_ shaderProgram = nullptr;
		for (auto& entity : scene->mEntities) {
			const auto& model = entity->mModel;
			if (!model) continue;
			if (entity->mShaderProgram != shaderProgram) {
				shaderProgram = entity->mShaderProgram;
				glUseProgram(shaderProgram->mID);
				glUniformMatrix4fv(shaderProgram->uProj, 1, GL_FALSE, (GLfloat*)&cam.mProjection[0]);
				glUniformMatrix4fv(shaderProgram->uView, 1, GL_FALSE, (GLfloat*)&cam.mView[0]);
				glUniform3fv(shaderProgram->uViewPos, 1, (GLfloat*)&cam.mPos[0]);
				glUniform3fv(shaderProgram->uLightPos, 1, (GLfloat*)&lightPos[0]);
				glUniform3fv(shaderProgram->uLightColor, 1, (GLfloat*)&lightColor[0]);
				glUniform1f(shaderProgram->uTime, timer.mNow);
			}
			if (entity->mAnimationController && shaderProgram->uBones) {
				const auto& bones = entity->mAnimationController->mFinalTransforms;
				glUniformMatrix4fv(shaderProgram->uBones, bones.size(), GL_FALSE, (GLfloat*)&bones[0]);
			}
			for (auto& modelMesh : model->mMeshes) {
				if (modelMesh->mMesh->mHidden) continue;
				glm::mat4 meshTransform = entity->mTransform * modelMesh->mTransform;
				glUniformMatrix4fv(shaderProgram->uModel, 1, GL_FALSE, (GLfloat*)&meshTransform[0]);
				modelMesh->mMesh->Bind();
				glDrawElements(modelMesh->mMesh->mMode, modelMesh->mMesh->mIndices.size(), GL_UNSIGNED_INT, 0);
			}
		}

		if (enableDebug) {
			if (drawDebug) {
				glDisable(GL_DEPTH_TEST);
				debugRenderer.Render(cam);
				glEnable(GL_DEPTH_TEST);
			}
			if (drawPersistentDebug) {
				persistentDebugRenderer.Render(cam);
			}
		}
		debugRenderer.Clear();

		ui->Render();
	}

	input.reset();
	scene.reset();

	glfwTerminate();
	return 0;
}
