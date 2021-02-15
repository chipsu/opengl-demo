#pragma once

#include "Main.h"

struct Shader {
	GLuint mID = 0;
	GLenum mType;

	Shader(const std::string& path, GLenum type) : mType(type) {
		mID = glCreateShader(type);
		Load(path);
	}

	~Shader() {
		glDeleteShader(mID);
	}

	void Load(const std::string& path) {
		std::cout << "Loading shader: " << path << "..." << std::endl;

		const auto source = ReadFile(path);

		const char* sourcePtr = source.c_str();
		glShaderSource(mID, 1, &sourcePtr, NULL);
		glCompileShader(mID);

		GLint status = 0;
		GLint infoLogLength = 0;
		glGetShaderiv(mID, GL_COMPILE_STATUS, &status);
		glGetShaderiv(mID, GL_INFO_LOG_LENGTH, &infoLogLength);

		if (infoLogLength > 0) {
			std::string message;
			message.resize(infoLogLength + 1);
			glGetShaderInfoLog(mID, infoLogLength, NULL, &message[0]);
			throw new std::runtime_error(message);
		}

		std::cout << "Shader loaded: " << path << std::endl;
	}
};
typedef std::shared_ptr<Shader> Shader_;

struct ShaderProgram {
	typedef std::shared_ptr<ShaderProgram> ShaderProgram_;
	static std::map<std::string, ShaderProgram_> sShaderPrograms;
	GLuint mID = 0;
	GLuint uProj;
	GLuint uView;
	GLuint uModel;
	GLuint uBones;
	GLuint uLightPos;
	GLuint uViewPos;
	GLuint uLightColor;
	GLuint uTime;

	ShaderProgram(const std::vector<Shader_>& shaders) {
		mID = glCreateProgram();
		for (auto& shader : shaders) {
			glAttachShader(mID, shader->mID);
		}
		glLinkProgram(mID);

		GLint status = 0;
		GLint infoLogLength = 0;
		glGetProgramiv(mID, GL_LINK_STATUS, &status);
		glGetProgramiv(mID, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0) {
			std::string message;
			message.resize(infoLogLength + 1);
			glGetProgramInfoLog(mID, infoLogLength, NULL, &message[0]);
			throw new std::runtime_error(message);
		}

		for (auto& shader : shaders) {
			glDetachShader(mID, shader->mID);
		}

		uProj = glGetUniformLocation(mID, "uProj");
		uView = glGetUniformLocation(mID, "uView");
		uModel = glGetUniformLocation(mID, "uModel");
		uBones = glGetUniformLocation(mID, "uBones");
		uLightPos = glGetUniformLocation(mID, "uLightPos");
		uViewPos = glGetUniformLocation(mID, "uViewPos");
		uLightColor = glGetUniformLocation(mID, "uLightColor");
		uTime = glGetUniformLocation(mID, "uTime");
	}
	~ShaderProgram() {
		glDeleteProgram(mID);
	}

	static ShaderProgram_ Load(const std::string& name) {
		auto it = sShaderPrograms.find(name);
		if (it != sShaderPrograms.end()) return it->second;
		auto shaderProgram = Create(name);
		sShaderPrograms[name] = shaderProgram;
		return shaderProgram;
	}

	static ShaderProgram_ Create(const std::string& name) {
		auto getType = [](const auto& type) {
			if (type == "vert") return GL_VERTEX_SHADER;
			if (type == "frag") return GL_FRAGMENT_SHADER;
			if (type == "geom") return GL_GEOMETRY_SHADER;
			throw new std::invalid_argument("Unknown shader type");
		};
		std::vector<Shader_> shaders;
		for (const auto& type : { "vert", "geom", "frag" }) {
			const auto file = "shaders/" + name + "." + type + ".glsl";
			if (!std::filesystem::exists(file)) continue;
			shaders.push_back(std::make_shared<Shader>(file, getType(type)));
		}
		assert(!shaders.empty());
		return std::make_shared<ShaderProgram>(shaders);
	}

};
typedef ShaderProgram::ShaderProgram_ ShaderProgram_;
