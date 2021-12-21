#ifndef __SCENEPARSER_HPP__
#define __SCENEPARSER_HPP__

#include "camera.hpp"
#include "light_object.hpp"
#include "hittables/hittable.hpp"
#include "hittables/mesh.hpp"
#include "json.hpp"
#include "bvh.hpp"
#include "GLFW/glfw3.h"

#include <vector>
#include <string>
#include <filesystem>

namespace SceneParser {

	glm::fvec3 parseVec3(nlohmann::basic_json<>& arr);

	glm::fvec4 parseVec4(nlohmann::basic_json<>& arr);

	std::shared_ptr<Mesh> parseMesh(nlohmann::json& mesh, const std::vector<MaterialPtr>& materials);
	std::shared_ptr<BVH> parseMeshInstance(nlohmann::json& mesh, const std::vector<MaterialPtr>& materials, std::string& name);
	std::shared_ptr<Hittable> parseInstance(nlohmann::json& mesh, const std::vector<MaterialPtr>& materials, std::unordered_map<std::string, std::shared_ptr<BVH>> meshes, int &numTri);
	Animation parseAnimation(nlohmann::json& animation);

	void parseTransform(nlohmann::basic_json<>& hit, HittablePtr primitive);

	CameraPtr parseCamera(nlohmann::json& cam);

	std::shared_ptr<LightObject> parseLight(nlohmann::json& l);

	std::shared_ptr<Material> parseMaterial(nlohmann::json& m, const std::unordered_map<std::string, std::shared_ptr<Texture>>& textures);

	std::pair<std::string, std::shared_ptr<Texture>> parseTexture(nlohmann::json& text);

	std::shared_ptr<Hittable> getMeshBVH(nlohmann::json& hit, const std::vector<MaterialPtr>& materials);

	std::shared_ptr<BVH> parseSceneGraph(nlohmann::json& text, const std::vector<MaterialPtr>& materials, std::unordered_map<std::string, std::shared_ptr<BVH>> meshes, std::vector<HittablePtr>& BVHs, int& numTri);

	static int findMaterial(std::string& name, const std::vector<MaterialPtr>& materials);

};

#endif
