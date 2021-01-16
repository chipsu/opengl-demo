#include "Main.h"
#include "Scene.h"
#include "Shader.h"
#include "Input.h"
#include "UI.h"

Input* Input::sInstance = nullptr;

Scene_ CreateScene() {
	auto scene = std::make_shared<Scene>();
	auto model = std::make_shared<Model>();
	model->Load("witch.fbx");
	if (model->mAnimationController && model->mAnimationController->GetAnimationCount()) {
		model->mAnimationController->SetAnimationIndex(0);
	}
	scene->mEntities.push_back(std::make_shared<Entity>(model));
	scene->mCameraCenter = scene->mEntities[0]->mModel->mAABB.mCenter;
	scene->mCameraDistance = glm::length(scene->mEntities[0]->mModel->mAABB.mHalfSize) * 2.0f;
	return scene;
}

ShaderProgram_ CreateShaderProgram() {
	std::vector<Shader_> shaders;
	shaders.push_back(std::make_shared<Shader>("default.vert.glsl", GL_VERTEX_SHADER));
	shaders.push_back(std::make_shared<Shader>("default.geom.glsl", GL_GEOMETRY_SHADER));
	shaders.push_back(std::make_shared<Shader>("default.frag.glsl", GL_FRAGMENT_SHADER));
	return std::make_shared<ShaderProgram>(shaders);
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

	auto window = glfwCreateWindow(1280, 720, "OpenGL Animation Demo", NULL, NULL);
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

	auto scene = CreateScene();;
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
	
	float timer = GetTime();
	bool useBlender = false;
	bool animDetails = false;
	bool meshDetails = false;

	auto selectedModel = scene->mEntities[0]->mModel;
	std::vector<float> selectedWeights;

	while (!glfwWindowShouldClose(window)) {
		float now = GetTime();
		float deltaTime = now - timer;

		glfwSwapBuffers(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}

		scene->Update(now, deltaTime);

		ui->NewFrame();

		//ImGui::ShowDemoWindow();

		if (selectedModel) {
			ImGui::Checkbox("Model info", &meshDetails);

			if (meshDetails) {
				for (const auto& mesh : selectedModel->mMeshes) {
					ImGui::Text("Mesh: h=%d, v=%d, i=%d, c=%s, s=%s",
						mesh->mHidden,
						mesh->mVertices.size(),
						mesh->mIndices.size(),
						glm::to_string(mesh->mAABB.mCenter).c_str(),
						glm::to_string(mesh->mAABB.mHalfSize).c_str()
					);
				}
			}
		}

		if (selectedModel && selectedModel->mAnimationController) {
			const auto ac = selectedModel->mAnimationController;
			ImGui::Begin(ac->GetAnimationEnabled() ? ac->GetAnimation()->mName.c_str() : "Animations");

			ImGui::Checkbox("Animation info", &animDetails);
			if (animDetails) {
				for (const auto& anim : ac->mAnimations) {
					ImGui::Text("Animation: %s, %f", anim->mName.c_str(), anim->mDuration);
				}
			}

			ImGui::Checkbox("Blend", &useBlender);

			if (useBlender) {
				ac->UpdateBlended(now); // FIXME
				// FIXME
				if (selectedWeights.size() != ac->GetAnimationCount()) {
					selectedWeights.resize(ac->GetAnimationCount());
					std::fill(selectedWeights.begin(), selectedWeights.end(), 0);
					selectedWeights[0] = 1.0f;
				}

				size_t animIndex = 0;
				for (const auto& anim : ac->mAnimations) {
					ImGui::SliderFloat(("#" + std::to_string(animIndex) + " " + anim->mName).c_str(), &selectedWeights[animIndex], 0.0f, 1.0f);
					ac->BlendAnimation(animIndex, selectedWeights[animIndex]);
					animIndex++;
				}
			}

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		glm::mat4 modl = glm::rotate(glm::identity<glm::mat4>(), scene->mCameraRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 view = glm::lookAt(glm::vec3(scene->mCameraDistance), scene->mCameraCenter, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), windowWidth / (float)windowHeight, 0.1f, 10000.0f);

		glUniformMatrix4fv(uniformProj, 1, GL_FALSE, (GLfloat*)&proj[0]);
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, (GLfloat*)&view[0]);
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, (GLfloat*)&modl[0]);

		for (auto& entity : scene->mEntities) {
			const auto& model = entity->mModel;
			if (!model) continue;
			if (model->mAnimationController) {
				const auto& bones = useBlender ? model->mAnimationController->mBlendedTransforms : model->mAnimationController->mFinalTransforms;
				glUniformMatrix4fv(uniformBones, bones.size(), GL_FALSE, (GLfloat*)&bones[0]);
			}
			for (auto& mesh : model->mMeshes) {
				if (mesh->mHidden) continue;
				mesh->Bind();
				glDrawElements(GL_TRIANGLES, mesh->mIndices.size(), GL_UNSIGNED_INT, 0);
			}
		}

		ui->Render();

		timer = now;
	}

	input.reset();
	scene.reset();
	program.reset();

	glfwTerminate();
	return 0;
}
