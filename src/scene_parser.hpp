#ifndef __SCENEPARSER_HPP__
#define __SCENEPARSER_HPP__

#include "camera.hpp"
#include "light_object.hpp"
#include "hittables/hittable.hpp"
#include "json.hpp"
#include "bvh.hpp"
#include "GLFW/glfw3.h"
#include "json.hpp"

#include <vector>
#include <string>
#include <filesystem>

namespace SceneParser {

	glm::fvec3 parseVec3(nlohmann::basic_json<>& arr);

	glm::fvec4 parseVec4(nlohmann::basic_json<>& arr);

	BVHPtr parseMeshInstance(nlohmann::json& mesh, const std::vector<MaterialPtr>& materials, std::string& name);
	BVHPtr parseMeshInstance(nlohmann::json& hit, std::vector<MaterialPtr>& materials, std::vector<TexturePtr>& textures, std::string &name);
	Animation parseAnimation(nlohmann::json& animation);

	void parseTransform(nlohmann::basic_json<>& hit, HittablePtr primitive);

	CameraPtr parseCamera(nlohmann::json& cam);

	std::shared_ptr<LightObject> parseLight(nlohmann::json& l);

	std::shared_ptr<Material> parseMaterial(nlohmann::json& m, std::vector<TexturePtr>& textures);

	TexturePtr parseTexture(nlohmann::json& text);

	//std::shared_ptr<Hittable> getMeshBVH(nlohmann::json& hit, const std::vector<MaterialPtr>& materials);

	std::shared_ptr<BVH> parseSceneGraph(nlohmann::json& text, const std::vector<MaterialPtr>& materials, std::unordered_map<std::string, BVHPtr>& meshes, std::unordered_map<std::string, std::list<BVHPtr>>& BVHs, int& numTri);

	static int findMaterial(std::string& name, std::vector<MaterialPtr>& materials);

	static int findTexture(std::string& name, std::vector<TexturePtr>& textures);
};

#endif
