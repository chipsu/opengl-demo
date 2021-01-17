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
#include <list>

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
		std::string message = "Could not open file: " + path;
		throw new std::runtime_error(message);
	}
	std::stringstream buffer;
	buffer << stream.rdbuf();
	const auto source = buffer.str();
	stream.close();
	return source;
}

template<typename T>
size_t SplitString(const std::string& str, const std::string& delim, T &result) {
	size_t current = str.find(delim);
	size_t previous = 0;
	while (current != std::string::npos) {
		result.push_back(str.substr(previous, current - previous));
		previous = current + 1;
		current = str.find(delim, previous);
	}
	result.push_back(str.substr(previous, current - previous));
	return result.size();
}
