#include "scene_parser.hpp"
#include "textures/texture.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "hittables/triangle.hpp"
#include "materials/material.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"
#include "materials/material_diffuse.hpp"
#include "materials/material_emissive.hpp"
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
		std::vector<EasingType> easings;
		for(auto &f : frames){
			EasingType easing;
			if(!f.contains("easing")) easing = EasingType::LINEAR;
			else {
				std::string e = f.at("easing");
				if(e == "easein_linear") easing = EasingType::LINEAR;
				else if(e == "easein_cubic") easing = EasingType::EASIN_CUBIC;
				else if(e == "easeout_cubic") easing = EasingType::EASOUT_CUBIC;
				else if(e == "easein_expo") easing = EasingType::EASIN_EXPO;
				else if(e == "easeout_expo") easing = EasingType::EASOUT_EXPO;
				else if(e == "easein_elastic") easing = EasingType::EASIN_ELASTIC;
				else if(e == "easeout_elastic") easing = EasingType::EASOUT_ELASTIC;
				else if(e == "easein_sin") easing = EasingType::EASIN_SIN;
				else if(e == "easeout_sin") easing = EasingType::EASOUT_SIN;
				else if(e == "easein_back") easing = EasingType::EASIN_BACK;
				else if(e == "easeout_back") easing = EasingType::EASOUT_BACK;
			}
			easings.push_back(easing);
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
		return Animation(loop, start, keyframes, times, easings); 
	}

	glm::fvec3 parseVec3(nlohmann::basic_json<>& arr) {
		return glm::fvec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
	}

	glm::fvec4 parseVec4(nlohmann::basic_json<>& arr) {
		return glm::fvec4(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>(), arr[3].get<float>());
	}

	std::shared_ptr<BVH> parseMeshInstance(nlohmann::json& hit, std::vector<MaterialPtr>& materials, std::vector<TexturePtr>& textures, std::string &name) {
		name = hit.at("name");

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
		auto& mats = reader.GetMaterials();

		std::vector<glm::fvec3> pos;
		std::vector<glm::fvec3> norm;
		std::vector<glm::fvec2> uv;
		std::vector<HittablePtr> triangles;
		std::vector<DiffuseMaterial> diffuseMats;

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

		int materialIdx = -1;
		if (hit.contains("material")){
			std::string matName = hit.at("material");
			materialIdx = findMaterial(matName, materials);
		} else if(!mats.empty()) {
			for (size_t i = 0; i < mats.size(); ++i) {
				TexturePtr t;
				if(mats[i].diffuse_texname.empty()){
					t = std::make_unique<SolidColor>(meshPath / ("Diffuse"), mats[i].diffuse[0], mats[i].diffuse[1], mats[i].diffuse[2]);
				} else {
					std::filesystem::path texturePath = meshPath.parent_path();
					texturePath /= mats[i].diffuse_texname;
					t = std::make_unique<ImageTexture>(mats[i].diffuse_texname, texturePath);
				}
				textures.emplace_back(std::move(t));
				materials.emplace_back(std::make_shared<DiffuseMaterial>(mats[i].name, textures.size() - 1));
			}
		} else throw std::invalid_argument("Mesh doesn't name a valid material");

		for (auto& shape : shapes) {
			const std::vector<tinyobj::index_t> idx = shape.mesh.indices;
			const std::vector<int>& mat_ids = shape.mesh.material_ids;
			for (size_t face_ind = 0; face_ind < mat_ids.size(); face_ind++) {
				int material;
				if(materialIdx == -1){
					auto name = mats[mat_ids[face_ind]].name;
					material = findMaterial(name, materials);
				} else {
					material = materialIdx;
				}
				auto tri = std::make_shared<Triangle>(
					glm::ivec3{ idx[3 * face_ind].vertex_index, 	idx[3 * face_ind + 1].vertex_index, 	idx[3 * face_ind + 2].vertex_index },
					glm::ivec3{ idx[3 * face_ind].normal_index, 	idx[3 * face_ind + 1].normal_index, 	idx[3 * face_ind + 2].normal_index },
					glm::ivec3{ idx[3 * face_ind].texcoord_index, 	idx[3 * face_ind + 1].texcoord_index, 	idx[3 * face_ind + 2].texcoord_index },
					material,
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
		bool refit = false;
		if(hit.contains("refit")){
			refit = hit.at("refit");
		}
		AABB bbox{ minX, minY, minZ, maxX, maxY, maxZ };
		return std::make_shared<BVH>(triangles, heuristic, refit);
	}

	std::pair<std::string, BVHPtr> parseInstance(nlohmann::json& mesh, const std::vector<MaterialPtr>& materials, std::unordered_map<std::string, BVHPtr> meshes, int &numTri) {
		if (!mesh.contains("name")) throw std::invalid_argument("Mesh doesn't name an instance");
		std::string name = mesh.at("name");
		auto m = meshes.find(name);
		numTri += m->second->getHittable().size();
		if(m == meshes.end()){
			throw std::invalid_argument("Mesh doesn't name a valid instance");
		}
		return std::make_pair(m->first, m->second);
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

	LightObjectPtr parseLight(nlohmann::json& l) {
		if (!l.contains("type")) throw std::invalid_argument("LightObject doesn't name a type");
		std::string type = l.at("type");
		Color color = (l.contains("color")) ? (parseVec3(l["color"])) : Color(1.0f);
		float intensity = (l.contains("intensity")) ? (float)(l.at("intensity")) : 1.0f;
		if (type == "POINT") {
			glm::fvec3 pos(parseVec3(l["position"]));
			return (std::make_unique<PointLight>(pos, intensity, color));
		}
		else if (type == "DIRECTIONAL") {
			glm::fvec3 dir(parseVec3(l["direction"]));
			return (std::make_unique<DirectionalLight>(dir, intensity, color));
		}
		else if (type == "AMBIENT") {
			return (std::make_unique<AmbientLight>(intensity, color));
		}
		else if (type == "SPOT") {
			glm::fvec3 pos(parseVec3(l["position"]));
			glm::fvec3 dir(parseVec3(l["direction"]));
			float cutoff = l.contains("cutoffAngle") ? (float)l.at("cutoffAngle") : 45.0f;
			return (std::make_unique<SpotLight>(pos, dir, glm::radians(cutoff), intensity, color));
		}
		else {
			throw std::invalid_argument("LightObject doesn't name a valid type");
		}
	}

	std::shared_ptr<Material> parseMaterial(nlohmann::json& m, std::vector<TexturePtr>& textures) {
		std::shared_ptr<Material> mat;
		if (!m.contains("name")) throw std::invalid_argument("Material is missing name");
		std::string name = m.at("name");
		if (!m.contains("type")) throw std::invalid_argument("Material is missing type");
		std::string type = m.at("type");
		if (!m.contains("texture")) throw std::invalid_argument("Material is missing texture");
		std::string textname = m.at("texture");
		auto texture = findTexture(textname, textures);
		if (texture == -1) throw std::invalid_argument("Material doesn't name a valid texture");
		if (type == "DIFFUSE") {
			mat = std::make_shared<DiffuseMaterial>(name, texture);
		}
		if (type == "EMISSIVE") {
			float intensity = 1.0f;
			if(m.contains("intensity")) intensity = m.at("intensity");
			std::cout << intensity;
			mat = std::make_shared<EmissiveMaterial>(name, texture, intensity);
		}
		else if (type == "MIRROR") {
			float ref = m.contains("reflect") ? m["reflect"].get<float>() : 1.0f;
			mat = std::make_shared<MirrorMaterial>(name, texture, ref);
		}
		else if (type == "DIELECTRIC") {
			float ior = (m.contains("ior")) ? (float)m.at("ior") : 1.0;
			Color absorption = m.contains("absorption") ? parseVec3(m.at("absorption")) : Color(1.0, 1.0, 1.0);
			mat = std::make_shared<DielectricMaterial>(name, texture, ior, absorption);
		}
		return mat;
	}

	TexturePtr parseTexture(nlohmann::json& text) {
		TexturePtr texture;
		std::string type;
		std::string name;
		try {
			name = text.at("name");
			type = text.at("type");
		}
		catch (std::exception& e) {
			throw std::invalid_argument("Cannot find name or type of texture");
		}
		if (type == "SOLID_COLOR") {
			texture = std::make_unique<SolidColor>(name, (text.contains("color")) ? parseVec3(text["color"]) : Color(0, 0, 0));
		}
		else if (type == "CHECKERED") {
			if (text.contains("color1") && text.contains("color2")) {
				Color c1 = parseVec3(text["color1"]);
				Color c2 = parseVec3(text["color2"]);
				texture = std::make_unique<Checkered>(name, c1, c2);
			}
			else {
				texture = std::make_unique<Checkered>(name);
			}
		}
		else if (type == "IMAGE") {
			if (text.contains("path"))
				texture = std::make_unique<ImageTexture>(name, text["path"].get<std::string>());
			else
				throw std::invalid_argument("Image Texture doesn't contains a valid path");
		}
		return texture;
	}

	std::shared_ptr<BVH> parseSceneGraph(nlohmann::json& text, const std::vector<MaterialPtr>& materials, std::unordered_map<std::string, BVHPtr>& meshes, std::unordered_map<std::string, std::list<BVHPtr>>& BVHs, int &numTri) {

		for (auto& obj : text) {
			//if (obj.contains("node"))
				//for (auto& node : obj["node"]) {
					//auto bvh = SceneParser::parseSceneGraph(node, materials, meshes, BVHs, numTri);
					//if (bvh)
						//BVHs.push_back(bvh);
				//}

			if (obj.contains("meshes")) {
				for (auto& m : obj["meshes"]) {
					auto instance = SceneParser::parseInstance(m, materials, meshes, numTri);
						bool animate = false;
						std::vector<HittablePtr> hittableVec;
						hittableVec.push_back(instance.second);
						auto bvh = std::make_shared<BVH>(hittableVec);
						parseTransform(m, bvh);
						if(m.contains("animation")){
							Animation anim = SceneParser::parseAnimation(m.at("animation"));
							bvh->setAnimation(anim);
						}
						BVHs[instance.first].push_back(bvh);
				}
			}
		}

		std::shared_ptr<BVH> topLevelBVH = nullptr;
		// TODO: my god. This is really ugly. Stop passing pointers around. 
		// Start using indices.
		// This is awful and slow.
		// Rework this when we implement back meshes as an abstraction, right now this is needed as we need to rebuild the BVH
		// of the instances associated with refitting meshes;
		if (!BVHs.empty()) {
			std::vector<HittablePtr> bvhs;
			for(auto const& imap: BVHs){
				for(auto const& i : imap.second){
					bvhs.push_back(i);
				}
			}
			topLevelBVH = std::make_shared<BVH>(bvhs, Heuristic::SAH, /*refit*/ false, /*makeTopLevel*/ true);
		}
		return topLevelBVH;
	}

	static int findTexture(std::string& name, std::vector<TexturePtr>& textures) {
		size_t i = 0;
		while (i < textures.size()) {
			if (textures[i]->getName() == name) return i;
			++i;
		}
		return -1;
	}
	static int findMaterial(std::string& name, std::vector<MaterialPtr>& materials) {
		size_t i = 0;
		while (i < materials.size()) {
			if (materials[i]->getName() == name) return i;
			++i;
		}
		return -1;
	}
}
