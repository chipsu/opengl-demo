#pragma once

#define MAX_VERTEX_WEIGHTS 4
#define AI_LMW_MAX_WEIGHTS MAX_VERTEX_WEIGHTS

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include "glm/ext.hpp"

// assimp
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>

inline glm::vec3 make_vec3(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
inline glm::vec2 make_vec2(const aiVector2D& v) { return glm::vec2(v.x, v.y); }
inline glm::quat make_quat(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
inline glm::mat4 make_mat4(const aiMatrix4x4& m) { return glm::transpose(glm::make_mat4(&m.a1)); }

inline float GetTime() {
	return (float)glfwGetTime();
}

inline glm::vec3 RandomColor() {
	return {
		(float)(rand()) / (float)(RAND_MAX),
		(float)(rand()) / (float)(RAND_MAX),
		(float)(rand()) / (float)(RAND_MAX)
	};
}

inline std::string ReadFile(const std::string& path) {
	std::ifstream stream(path, std::ios::in);
	if (!stream.is_open()) {
		std::string message = "Could not open shader file: " + path;
		throw new std::runtime_error(message);
	}
	std::stringstream buffer;
	buffer << stream.rdbuf();
	const auto source = buffer.str();
	stream.close();
	return source;
}
