#pragma once

#include "Main.h"

template<typename T>
struct AssetMgr {
	typedef std::shared_ptr<T> T_;
	std::map<std::string, T_> mAssets;
	std::map<std::string, std::string> mKeyCache;
	rapidjson::Document mDefaultOptions;
	bool mUseCache = true;

	AssetMgr() {
		mDefaultOptions.Parse("{}");
	}

	// TODO: hash file content
	// TODO: options
	std::string CreateKey(const std::string& name) {
		auto it = mKeyCache.find(name);
		if (it != mKeyCache.end()) {
			return it->second;
		}
		auto id = std::hash<std::string>{}(name);
		auto key = "cache/" + std::to_string(id) + ".dat";
		mKeyCache[name] = key;
		std::cout << name << " -> " << key << std::endl;
		return key;
	}

	T_ Load(const std::string& name) {
		auto key = CreateKey(name);
		auto it = mAssets.find(key);
		if (it != mAssets.end()) return it->second;
		auto asset = std::make_shared<T>();
		if (mUseCache && std::filesystem::exists(key)) {
			asset->Import(key);
		} else {
			auto pos = name.find_first_of("?");
			if (pos != -1) {
				auto fileName = name.substr(0, pos);
				auto opts = name.substr(pos + 1);
				rapidjson::Document options;
				options.Parse(opts.c_str());
				asset->Load(fileName, options.GetObject());
			} else {
				asset->Load(name, mDefaultOptions);
			}
			if (mUseCache) {
				asset->Export(key);
			}
		}
		mAssets[key] = asset;
		return asset;
	}

	T_ Load(const std::string& name, const rapidjson::Value& options) {
		rapidjson::Document doc;
		doc.CopyFrom(options, doc.GetAllocator());
		std::ostringstream oss;
		rapidjson::OStreamWrapper osw(oss);
		rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
		doc.Accept(writer);
		return Load(name + "?" + oss.str());
	}

	T_ Load(const std::string& name, const rapidjson::Value& options, const std::string& optionsKey) {
		if (cfg.HasMember(optionsKey)) {
			return Load(name, cfg[optionsKey]);
		}
		return Load(name);
	}
};
