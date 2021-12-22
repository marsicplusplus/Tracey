#include "scene_parser.hpp"
#include "textures/texture.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "hittables/triangle.hpp"
#include "materials/material.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"
#include "materials/material_diffuse.hpp"
#include "animation.hpp"
#include "glm/trigonometric.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
#include <fstream>

namespace SceneParser {

	Animation parseAnimation(nlohmann::json& animation){
		bool loop = false;
		int start = 0;
		if(animation.contains("loop"))
			loop = animation.at("loop");
		if(animation.contains("start"))
			start = animation.at("start");
		auto &frames = animation.at("frames");

		std::vector<Transform> keyframes;
		std::vector<float> times;
		for(auto &f : frames){
			times.push_back(f.at("time"));
			auto& t = f.at("transform");
			Transform transformation;
			glm::fvec3 translation = parseVec3(t.at("translation"));
			transformation.translate(translation);
			glm::fvec3 rot = parseVec3(t.at("rotation"));
			if (rot.x != 0) transformation.rotate(glm::radians(rot.x), glm::fvec3(1.0, 0.0, 0.0));
			if (rot.y != 0) transformation.rotate(glm::radians(rot.y), glm::fvec3(0.0, 1.0, 0.0));
			if (rot.z != 0) transformation.rotate(glm::radians(rot.z), glm::fvec3(0.0, 0.0, 1.0));
			if (t.contains("scale")) {
				if (t.at("scale").is_array()){
					auto s = parseVec3(t.at("scale"));
					transformation.scale(s);
				} else transformation.scale((float)(t.at("scale")));
			}
			keyframes.emplace_back(transformation);
		}
		return Animation(loop, start, keyframes, times); 
	}

	glm::fvec3 parseVec3(nlohmann::basic_json<>& arr) {
		return glm::fvec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
	}

	glm::fvec4 parseVec4(nlohmann::basic_json<>& arr) {
		return glm::fvec4(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>(), arr[3].get<float>());
	}

	std::shared_ptr<BVH> parseMeshInstance(nlohmann::json& hit, const std::vector<MaterialPtr>& materials, std::string &name) {
		name = hit.at("name");

		if (!hit.contains("material")) throw std::invalid_argument("Mesh doesn't name a material");
		std::string matName = hit.at("material");
		int materialIdx = findMaterial(matName, materials);
		if (materialIdx == -1) throw std::invalid_argument("Mesh doesn't name a valid material");

		if (!hit.contains("path")) {
			throw std::invalid_argument("Mesh doesn't have a valid path");
		}
	
		std::filesystem::path meshPath = hit.at("path");

		tinyobj::ObjReaderConfig reader_config;
		reader_config.triangulate = true;
		reader_config.mtl_search_path = meshPath.parent_path().string(); // Path to material files

		tinyobj::ObjReader reader;
		if (!reader.ParseFromFile(meshPath.string(), reader_config)) {
			if (!reader.Error().empty()) std::cerr << "TinyObjReader: " << reader.Error();
			return nullptr;
		}
		if (!reader.Warning().empty()) std::cout << "TinyObjReader: " << reader.Warning();

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();

		std::vector<glm::fvec3> pos;
		std::vector<glm::fvec3> norm;
		std::vector<glm::fvec2> uv;
		std::vector<HittablePtr> triangles;

		float minX = INF, minY = INF, minZ = INF, maxX = -INF, maxY = -INF, maxZ = -INF;

		for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
			auto xVal = attrib.vertices[i];
			auto yVal = attrib.vertices[i + 1];
			auto zVal = attrib.vertices[i + 2];

			minX = min(minX, xVal);
			minY = min(minY, yVal);
			minZ = min(minZ, zVal);
			maxX = max(maxX, xVal);
			maxY = max(maxY, yVal);
			maxZ = max(maxZ, zVal);

			pos.emplace_back(
				attrib.vertices[i],
				attrib.vertices[i + 1],
				attrib.vertices[i + 2]
			);
		}

		for (size_t i = 0; i < attrib.normals.size(); i += 3) {
			norm.emplace_back(
				attrib.normals[i],
				attrib.normals[i + 1],
				attrib.normals[i + 2]
			);
		}
		for (size_t i = 0; i < attrib.texcoords.size(); i += 2) {
			uv.push_back(glm::dvec2{
				attrib.texcoords[i],
				attrib.texcoords[i + 1]
			});
		}

		for (auto& shape : shapes) {
			const std::vector<tinyobj::index_t> idx = shape.mesh.indices;
			const std::vector<int>& mat_ids = shape.mesh.material_ids;
			for (size_t face_ind = 0; face_ind < mat_ids.size(); face_ind++) {
				auto tri = std::make_shared<Triangle>(
					glm::ivec3{ idx[3 * face_ind].vertex_index, 	idx[3 * face_ind + 1].vertex_index, 	idx[3 * face_ind + 2].vertex_index },
					glm::ivec3{ idx[3 * face_ind].normal_index, 	idx[3 * face_ind + 1].normal_index, 	idx[3 * face_ind + 2].normal_index },
					glm::ivec3{ idx[3 * face_ind].texcoord_index, 	idx[3 * face_ind + 1].texcoord_index, 	idx[3 * face_ind + 2].texcoord_index },
					materialIdx,
					pos,
					norm,
					uv
				);
				triangles.push_back(tri);
			}
		}
		Heuristic heuristic = Heuristic::SAH;
		if(hit.contains("bvh")){
			if(hit.at("bvh") == "MIDPOINT") heuristic = Heuristic::MIDPOINT;
		}
		AABB bbox{ minX, minY, minZ, maxX, maxY, maxZ };
		return std::make_shared<BVH>(triangles, heuristic);
	}

	std::shared_ptr<Hittable> parseInstance(nlohmann::json& mesh, const std::vector<MaterialPtr>& materials, std::unordered_map<std::string, std::shared_ptr<BVH>> meshes, int &numTri) {
		if (!mesh.contains("name")) throw std::invalid_argument("Mesh doesn't name an instance");
		std::string name = mesh.at("name");
		auto m = meshes.find(name);
		numTri += m->second->getHittable().size();
		if(m == meshes.end()){
			throw std::invalid_argument("Mesh doesn't name a valid instance");
		}

		return m->second;
	};

	void parseTransform(nlohmann::basic_json<>& hit, HittablePtr primitive) {
		if (hit.contains("transform")) {
			auto trans = hit.at("transform");
			if (trans.contains("translation")) primitive->translate(parseVec3(trans.at("translation")));
			if (trans.contains("scale")) {
				if (trans.at("scale").is_array()) primitive->scale(parseVec3(trans.at("scale")));
				else primitive->scale((float)(trans.at("scale")));
			}
			if (trans.contains("rotation")) {
				auto rot = parseVec3(trans.at("rotation"));
				if (rot.x != 0) primitive->rotate(glm::radians(rot.x), glm::fvec3(1.0, 0.0, 0.0));
				if (rot.y != 0) primitive->rotate(glm::radians(rot.y), glm::fvec3(0.0, 1.0, 0.0));
				if (rot.z != 0) primitive->rotate(glm::radians(rot.z), glm::fvec3(0.0, 0.0, 1.0));
			}
		}
	}


	CameraPtr parseCamera(nlohmann::json& cam) {
		glm::fvec3 pos = parseVec3(cam.at("position"));
		glm::fvec3 dir = parseVec3(cam.at("dir"));
		glm::fvec3 up = parseVec3(cam.at("up"));
		float fov = (cam.at("fov"));
		return std::make_shared<Camera>(pos, dir, up, fov);
	}

	std::shared_ptr<LightObject> parseLight(nlohmann::json& l) {
		if (!l.contains("type")) throw std::invalid_argument("LightObject doesn't name a type");
		std::string type = l.at("type");
		Color color = (l.contains("color")) ? (parseVec3(l["color"])) : Color(1.0f);
		float intensity = (l.contains("intensity")) ? (float)(l.at("intensity")) : 1.0f;
		if (type == "POINT") {
			glm::fvec3 pos(parseVec3(l["position"]));
			return (std::make_shared<PointLight>(pos, intensity, color));
		}
		else if (type == "DIRECTIONAL") {
			glm::fvec3 dir(parseVec3(l["direction"]));
			return (std::make_shared<DirectionalLight>(dir, intensity, color));
		}
		else if (type == "AMBIENT") {
			return (std::make_shared<AmbientLight>(intensity, color));
		}
		else if (type == "SPOT") {
			glm::fvec3 pos(parseVec3(l["position"]));
			glm::fvec3 dir(parseVec3(l["direction"]));
			float cutoff = l.contains("cutoffAngle") ? (float)l.at("cutoffAngle") : 45.0f;
			return (std::make_shared<SpotLight>(pos, dir, glm::radians(cutoff), intensity, color));
		}
		else {
			throw std::invalid_argument("LightObject doesn't name a valid type");
		}
	}

	std::shared_ptr<Material> parseMaterial(nlohmann::json& m, const std::unordered_map<std::string, std::shared_ptr<Texture>>& textures) {
		std::shared_ptr<Material> mat;
		if (!m.contains("name")) throw std::invalid_argument("Material is missing name");
		std::string name = m.at("name");
		if (!m.contains("type")) throw std::invalid_argument("Material is missing type");
		std::string type = m.at("type");
		if (!m.contains("texture")) throw std::invalid_argument("Material is missing texture");
		std::string textname = m.at("texture");
		auto texture = textures.find(textname);
		if (texture == textures.end()) throw std::invalid_argument("Material doesn't name a valid texture");
		if (type == "DIFFUSE") {
			mat = std::make_shared<DiffuseMaterial>(texture->second, name);
		}
		else if (type == "MIRROR") {
			float ref = m.contains("reflect") ? m["reflect"].get<float>() : 1.0f;
			mat = std::make_shared<MirrorMaterial>(texture->second, name, ref);
		}
		else if (type == "DIELECTRIC") {
			float ior = (m.contains("ior")) ? (float)m.at("ior") : 1.0;
			Color absorption = m.contains("absorption") ? parseVec3(m.at("absorption")) : Color(1.0, 1.0, 1.0);
			mat = std::make_shared<DielectricMaterial>(texture->second, name, ior, absorption);
		}
		return mat;
	}

	std::pair<std::string, std::shared_ptr<Texture>> parseTexture(nlohmann::json& text) {
		std::pair<std::string, std::shared_ptr<Texture>> texture;
		std::string type;
		try {
			texture.first = text.at("name");
			type = text.at("type");
		}
		catch (std::exception& e) {
			throw std::invalid_argument("Cannot find name or type of texture");
		}
		if (type == "SOLID_COLOR") {
			texture.second = std::make_shared<SolidColor>((text.contains("color")) ? parseVec3(text["color"]) : Color(0, 0, 0));
		}
		else if (type == "CHECKERED") {
			if (text.contains("color1") && text.contains("color2")) {
				Color c1 = parseVec3(text["color1"]);
				Color c2 = parseVec3(text["color2"]);
				texture.second = std::make_shared<Checkered>(c1, c2);
			}
			else {
				texture.second = std::make_shared<Checkered>();
			}
		}
		else if (type == "IMAGE") {
			if (text.contains("path"))
				texture.second = std::make_shared<ImageTexture>(text["path"].get<std::string>());
			else
				throw std::invalid_argument("Image Texture doesn't contains a valid path");
		}
		return texture;
	}

	std::shared_ptr<BVH> parseSceneGraph(nlohmann::json& text, const std::vector<MaterialPtr>& materials, std::unordered_map<std::string, std::shared_ptr<BVH>> meshes, std::vector<HittablePtr>& BVHs, int &numTri) {

		for (auto& obj : text) {
			if (obj.contains("node"))
				for (auto& node : obj["node"]) {
					auto bvh = SceneParser::parseSceneGraph(node, materials, meshes, BVHs, numTri);
					if (bvh)
						BVHs.push_back(bvh);
				}

			if (obj.contains("meshes")) {
				for (auto& m : obj["meshes"]) {
					auto instance = SceneParser::parseInstance(m, materials, meshes, numTri);
					if (instance) {
						bool animate = false;
						std::vector<HittablePtr> hittableVec;
						hittableVec.push_back(instance);
						auto bvh = std::make_shared<BVH>(hittableVec);
						parseTransform(m, bvh);
						if(m.contains("animation")){
							Animation anim = SceneParser::parseAnimation(m.at("animation"));
							bvh->setAnimation(anim);
						}
						BVHs.push_back(bvh);
					}
				}
			}
		}

		std::shared_ptr<BVH> topLevelBVH = nullptr;
		if (!BVHs.empty()) {
			topLevelBVH = std::make_shared<BVH>(BVHs, Heuristic::SAH, /*refit*/ false, /*makeTopLevel*/ true);
		}
		return topLevelBVH;
	}


	std::shared_ptr<Hittable> getMeshBVH(nlohmann::json& hit, const std::vector<MaterialPtr>& materials) {
		if (!hit.contains("material")) throw std::invalid_argument("Mesh doesn't name a material");
		std::string matName = hit.at("material");
		int materialIdx = findMaterial(matName, materials);
		if (materialIdx == -1) throw std::invalid_argument("Mesh doesn't name a valid material");

		std::shared_ptr<Hittable> meshBVH;

		//if (hit.contains("path")) {
		//	std::filesystem::path p = hit.at("path");
		//	meshBVH = parseMesh(p, materialIdx);
		//}



		return meshBVH;
	}

	static int findMaterial(std::string& name, const std::vector<MaterialPtr>& materials) {
		size_t i = 0;
		while (i < materials.size()) {
			if (materials[i]->getName() == name) return i;
			++i;
		}
		return -1;
	}
}
