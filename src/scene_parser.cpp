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
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "importer.hpp"

#include <iostream>
#include <fstream>

namespace SceneParser {

	std::vector<std::shared_ptr<Hittable>> createTriangleMesh(const int nTriangles, const int nVertices, 
			const glm::vec3 *p, const glm::vec3 *n, const glm::vec2 uv){
		return std::vector<std::shared_ptr<Hittable>>();
	}

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

	std::shared_ptr<BVH> parseMeshInstance(nlohmann::json& hit, std::vector<MaterialPtr>& materials, std::vector<TexturePtr>& textures, std::vector<std::shared_ptr<TriangleMesh>> meshes, std::string &name) {
		name = hit.at("name");

		if (!hit.contains("path")) {
			throw std::invalid_argument("Mesh doesn't have a valid path");
		}
	
		std::filesystem::path meshPath = hit.at("path");
		std::vector<std::shared_ptr<Hittable>> hittables;
		if(meshPath.extension() == ".obj") {
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(meshPath.string(), aiProcess_Triangulate | aiProcess_GenNormals);

			if(!scene) 
				throw std::invalid_argument("Failed parsing the obj file");

			for(int i = 0; i < scene->mNumMeshes; ++i){
				auto mesh = scene->mMeshes[i];
				std::vector<glm::fvec3> verts(mesh->mNumVertices);
				std::vector<glm::fvec3> norms;
				std::vector<glm::fvec2> uvs;
				std::vector<unsigned int> indices(mesh->mNumFaces*3);
				std::string meshName = mesh->mName.C_Str();
				if(meshName.empty()) meshName = name;
				for(int j = 0; j < verts.size(); ++j){
					verts[j] = glm::fvec3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);
				}
				if(mesh->HasNormals()){
					norms.resize(mesh->mNumVertices);
					for(int j = 0; j < norms.size(); ++j){
						norms[j] = glm::fvec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
					}
				}
				if(mesh->HasTextureCoords(0)){
					uvs.resize(mesh->mNumVertices);
					for(int j = 0; j < verts.size(); ++j){
						uvs[j] = glm::fvec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
					}
				}
				int faceIdx = 0;
				for(unsigned int j = 0; j < mesh->mNumFaces; ++j){
					indices[faceIdx++] = mesh->mFaces[j].mIndices[0];
					indices[faceIdx++] = mesh->mFaces[j].mIndices[1];
					indices[faceIdx++] = mesh->mFaces[j].mIndices[2];
				}
				auto triMesh = std::make_shared<TriangleMesh>(meshName, mesh->mNumFaces, mesh->mNumVertices, indices.data(), verts.data(), norms.data(), uvs.data());
				meshes.emplace_back(triMesh);
				/* Get Material */
				auto material = scene->mMaterials[mesh->mMaterialIndex];
				std::string matName;
				if(hit.contains("material")) matName = hit.at("material");
				else matName = material->GetName().C_Str();
				if(matName.empty()) matName = name + "_material";
				auto matIdx = findMaterial(matName, materials);
				if(matIdx == -1){
					TexturePtr text;
					int numTextures = material->GetTextureCount(aiTextureType_DIFFUSE);
					if(numTextures > 0){
						aiString textureName;
						material->GetTexture(aiTextureType_DIFFUSE, 0, &textureName);
						std::filesystem::path fp = meshPath.parent_path() / std::filesystem::path(textureName.C_Str());
						text = std::make_unique<ImageTexture>(textureName.C_Str(), fp.string());
					} else {
						aiColor4D color{0.0,0.0,0.0,0.0};
						aiGetMaterialColor(material,AI_MATKEY_COLOR_DIFFUSE,&color);
						text = std::make_unique<SolidColor>(matName, color.r, color.g, color.b);
					}
					textures.emplace_back(std::move(text));
					materials.emplace_back(std::make_shared<DiffuseMaterial>(matName, textures.size() - 1));
					matIdx = materials.size() - 1;
				}
				/* Create mesh' triangles */
				for(unsigned int k = 0; k < mesh->mNumFaces; ++k){
					auto tri = std::make_shared<Triangle>(triMesh, k, matIdx);
					hittables.emplace_back(tri);
				}
			}		
		} else if(meshPath.extension() == ".bcc") {
			std::string matName;
			int matIdx = -1;
			if(hit.contains("material")) matName = hit.at("material");
			matIdx = findMaterial(matName, materials);
			if(matName.empty() || matIdx == -1) {
				throw std::invalid_argument("Curve does not name a material!");
			}
			Importer::readBCC(meshPath, hittables, matIdx);
		} else if(meshPath.extension() == ".pbrt") {
			std::string matName;
			int matIdx = -1;
			if(hit.contains("material")) matName = hit.at("material");
			matIdx = findMaterial(matName, materials);
			if(matName.empty() || matIdx == -1) {
				throw std::invalid_argument("Curve does not name a material!");
			}

			int numSegments = (hit.contains("segments")) ? (int)hit.at("segments") : 8;

			Importer::readPBRCurve(meshPath, hittables, matIdx, numSegments);
		}

		Heuristic heuristic = Heuristic::SAH;
		if(hit.contains("bvh")){
			if(hit.at("bvh") == "MIDPOINT") heuristic = Heuristic::MIDPOINT;
		}
		bool refit = false;
		if(hit.contains("refit")){
			refit = hit.at("refit");
		}
		return std::make_shared<BVH>(hittables, heuristic, refit);
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
