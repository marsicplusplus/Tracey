#include "scene.hpp"

#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "json.hpp"
#include "textures/texture.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "hittables/mesh.hpp"
#include "hittables/torus.hpp"
#include "materials/material.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"
#include "materials/material_diffuse.hpp"
#include "glm/trigonometric.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
#include <fstream>

static glm::fvec3 parseVec3(nlohmann::basic_json<> &arr){
	return glm::fvec3(arr[0].get<float>(), arr[1].get<float>(),arr[2].get<float>());
}

static glm::fvec4 parseVec4(nlohmann::basic_json<> &arr){
	return glm::fvec4(arr[0].get<float>(), arr[1].get<float>(),arr[2].get<float>(), arr[3].get<float>());
}

std::shared_ptr<Hittable> Scene::parseMesh(std::filesystem::path &path, int mat) const {
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate = true;
	reader_config.mtl_search_path = path.parent_path().string(); // Path to material files

	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(path.string(), reader_config)) {
		if (!reader.Error().empty()) std::cerr << "TinyObjReader: " << reader.Error();
		return nullptr;
	}
	if (!reader.Warning().empty()) std::cout << "TinyObjReader: " << reader.Warning();

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();

	std::vector<glm::fvec3> pos;
	std::vector<glm::fvec3> norm;
	std::vector<glm::fvec2> uv;
	std::vector<Triangle> triangles;
	
	float minX = INFINITY, minY = INFINITY, minZ = INFINITY, maxX = -INFINITY, maxY = -INFINITY, maxZ = -INFINITY;

	for(size_t i = 0; i < attrib.vertices.size(); i+=3) {
		auto xVal = attrib.vertices[i];
		auto yVal = attrib.vertices[i+1];
		auto zVal = attrib.vertices[i+2];

		if (xVal < minX) {
			minX = xVal;
		}

		if (yVal < minY) {
			minY = yVal;
		}

		if (zVal < minZ) {
			minZ = zVal;
		}

		if (xVal > maxX) {
			maxX = xVal;
		}

		if (yVal > maxY) {
			maxY = yVal;
		}

		if (zVal > maxZ) {
			maxZ = zVal;
		}

		pos.push_back(glm::fvec3{
				attrib.vertices[i],
				attrib.vertices[i+1],
				attrib.vertices[i+2],
				});
	}
	for(size_t i = 0; i < attrib.normals.size(); i+=3) {
		norm.push_back(glm::fvec3{
				attrib.normals[i],
				attrib.normals[i+1],
				attrib.normals[i+2],
				});
	}
	for(size_t i = 0; i < attrib.texcoords.size(); i+=2) {
		uv.push_back(glm::dvec2{
				attrib.texcoords[i],
				attrib.texcoords[i+1],
				});
	}

	for(auto &shape : shapes){
		const std::vector<tinyobj::index_t> idx = shape.mesh.indices;
		const std::vector<int> & mat_ids = shape.mesh.material_ids;
		for(size_t face_ind = 0; face_ind < mat_ids.size(); face_ind++) {
			triangles.push_back(Triangle(
						glm::ivec3{idx[3*face_ind].vertex_index, 	idx[3*face_ind+1].vertex_index, 	idx[3*face_ind+2].vertex_index},
						glm::ivec3{idx[3*face_ind].normal_index, 	idx[3*face_ind+1].normal_index, 	idx[3*face_ind+2].normal_index},
						glm::ivec3{idx[3*face_ind].texcoord_index, 	idx[3*face_ind+1].texcoord_index, 	idx[3*face_ind+2].texcoord_index},
						mat
					));
		}
	}

	AABB bbox(minX, minY, minZ, maxX, maxY, maxZ);
	return std::make_shared<Mesh>(pos, norm, uv, triangles, bbox);
}

void Scene::parseTransform(nlohmann::basic_json<> &hit, HittablePtr& primitive) const {
	if(hit.contains("transform")){
		auto trans = hit.at("transform");
		if(trans.contains("translation")) primitive->translate(parseVec3(trans.at("translation")));
		if(trans.contains("scale")){ 
			if(trans.at("scale").is_array()) primitive->scale(parseVec3(trans.at("scale")));
			else primitive->scale((float)(trans.at("scale")));
		}
		if(trans.contains("rotation")){ 
			auto rot = parseVec3(trans.at("rotation"));
			if(rot.x != 0) primitive->rotate(glm::radians(rot.x), glm::fvec3(1.0,0.0,0.0));
			if(rot.y != 0) primitive->rotate(glm::radians(rot.y), glm::fvec3(0.0,1.0,0.0));
			if(rot.z != 0) primitive->rotate(glm::radians(rot.z), glm::fvec3(0.0,0.0,1.0));
		}
	}
}

Scene::Scene(){}
Scene::Scene(std::filesystem::path sceneFile){
	std::filesystem::path currPath = std::filesystem::current_path();
	std::filesystem::current_path(sceneFile.parent_path());
	std::ifstream i(sceneFile.filename());
	auto j = nlohmann::json::parse(i);

	auto camera = parseCamera(j["camera"]);
	setCamera(camera);

	for(auto t : j["textures"]){
		auto text = parseTexture(t);
		if (text.second){
			textures.insert(text);
		}
	}
	for(auto m : j["materials"]){
		auto mat = parseMaterial(m);
		if (mat){
			materials.push_back(mat);
		}
	}
	for(auto p : j["primitives"]){
		auto hit=parseHittable(p);
		if(hit)
			hittables.push_back(hit);
	}
	for(auto l : j["lights"]){
		auto light=parseLight(l);
		if(light)
			lights.push_back(light);
	}
	std::filesystem::current_path(currPath);
}
Scene::~Scene(){}

void Scene::addHittable(HittablePtr hittable){
	this->hittables.push_back(hittable);
}

void Scene::addLight(LightObjectPtr light){
	this->lights.push_back(light);
}

bool Scene::traverse(const Ray &ray, float tMin, float tMax, HitRecord &rec) const {
	HitRecord tmp;
	bool hasHit = false;
	float closest = tMax;

	for (const auto& object : this->hittables) {
		if (object->hitSelf(ray, tMin, closest, tmp)) {
			hasHit = true;
			closest = tmp.t;
			rec = tmp;
		}
	}
	return hasHit;
}

void Scene::setCamera(CameraPtr camera){
	this->currentCamera = camera;
}

const CameraPtr Scene::getCamera() const {
	return this->currentCamera;
}

bool Scene::update(float dt){
	return this->currentCamera->update(dt);
}

Color Scene::traceLights(HitRecord &rec) const {
	Color illumination(0.0f);
	for(auto &light : lights){
		float tMax;
		Ray shadowRay = light->getRay(rec, tMax);

		// Required for Spotlights
		if (shadowRay.getDirection() == glm::fvec3(0, 0, 0)) {
			continue;
		}

		HitRecord obstruction;
		if(!traverse(shadowRay, 0.0001f, tMax, obstruction)){
			auto contribution = light->getLight(rec, shadowRay);
			illumination += light->attenuate(contribution, rec.p);
		}
	}
	return illumination;
}

CameraPtr Scene::parseCamera(nlohmann::json &cam) const {
	glm::fvec3 pos = parseVec3(cam.at("position"));
	glm::fvec3 dir = parseVec3(cam.at("dir"));
	glm::fvec3 up = parseVec3(cam.at("up"));
	float fov = (cam.at("fov"));
	return std::make_shared<Camera>(pos, dir, up, fov);	
}

std::shared_ptr<Hittable> Scene::parseHittable(nlohmann::json &hit) const {
	if(!hit.contains("type")) throw std::invalid_argument("Hittable doesn't name a type");
	std::string type = hit.at("type");
	if (!hit.contains("material")) throw std::invalid_argument("Hittable doesn't name a material");
	std::string matName = hit.at("material");
	int materialIdx = findMaterial(matName);
	if (materialIdx == -1) throw std::invalid_argument("Hittable doesn't name a valid material");
	std::shared_ptr<Hittable> primitive;
	if(type == "PLANE"){
		glm::fvec3 pos(parseVec3(hit["position"]));
		if(!hit.contains("normal")) throw std::invalid_argument("Plane doesn't specify a normal");
		glm::fvec3 norm(parseVec3(hit.at("normal")));
		primitive = (std::make_shared<Plane>(pos, norm, materialIdx));
	} else if(type == "SPHERE"){
		float radius = (hit.contains("radius")) ? hit["radius"].get<float>() : 0.5;
		primitive = (std::make_shared<Sphere>(radius, materialIdx));

	} else if (type == "TORUS"){
		float radiusMajor = (hit.contains("radiusMajor")) ? hit["radiusMajor"].get<float>() : 0.5;
		float radiusMinor = (hit.contains("radiusMinor")) ? hit["radiusMinor"].get<float>() : 0.1;
		primitive = (std::make_shared<Torus>(radiusMajor, radiusMinor, materialIdx));
	} else if(type == "ZXRect"){
		primitive = std::make_shared<ZXRect>(materialIdx);
	} else if(type == "MESH"){
		if(hit.contains("path")) {
			std::filesystem::path p = hit.at("path");
			primitive = parseMesh(p, materialIdx);
		}
	} else{
		throw std::invalid_argument("Hittable doesn't name a valid type");
	}
	parseTransform(hit, primitive);
	return primitive;
}
std::shared_ptr<LightObject> Scene::parseLight(nlohmann::json &l) const{
	if(!l.contains("type")) throw std::invalid_argument("LightObject doesn't name a type");
	std::string type = l.at("type");
	Color color = (l.contains("color")) ? (parseVec3(l["color"])) : Color(1.0f);
	float intensity = (l.contains("intensity")) ? (float)(l.at("intensity")) : 1.0f;
	if(type == "POINT"){
		glm::fvec3 pos(parseVec3(l["position"]));
		return (std::make_shared<PointLight>(pos, intensity, color));
	} else if(type == "DIRECTIONAL"){
		glm::fvec3 dir(parseVec3(l["direction"]));
		return (std::make_shared<DirectionalLight>(dir, intensity, color));
	} else if(type == "AMBIENT"){
		return (std::make_shared<AmbientLight>(intensity, color));
	} else if (type == "SPOT") {
		glm::fvec3 pos(parseVec3(l["position"]));
		glm::fvec3 dir(parseVec3(l["direction"]));
		float cutoff = l.contains("cutoffAngle") ? (float)l.at("cutoffAngle") : 45.0f;
		return (std::make_shared<SpotLight>(pos, dir, glm::radians(cutoff), intensity, color));
	} else {
		throw std::invalid_argument("LightObject doesn't name a valid type");
	}
}

std::shared_ptr<Material> Scene::parseMaterial(nlohmann::json &m) const {
	std::shared_ptr<Material> mat;
	if(!m.contains("name")) throw std::invalid_argument("Material is missing name");
	std::string name = m.at("name");
	if(!m.contains("type")) throw std::invalid_argument("Material is missing type");
	std::string type = m.at("type");
	if(!m.contains("texture")) throw std::invalid_argument("Material is missing texture");
	std::string textname = m.at("texture");
	auto texture = textures.find(textname);
	if(texture == textures.end()) throw std::invalid_argument("Material doesn't name a valid texture");
	if(type == "DIFFUSE"){
		mat = std::make_shared<DiffuseMaterial>(texture->second, name);
	} else if(type == "MIRROR") {
		float ref = m.contains("reflect") ? m["reflect"].get<float>() : 1.0f;
		mat = std::make_shared<MirrorMaterial>(texture->second, name, ref);
	}
	else if (type == "DIELECTRIC") {
		float ior = (m.contains("ior")) ? (float)m.at("ior") : 1.0;
		Color absorption = m.contains("absorption") ? parseVec3(m.at("absorption")) : Color(1.0,1.0,1.0);
		mat = std::make_shared<DielectricMaterial>(texture->second, name, ior, absorption);
	}
	return mat;
}

std::pair<std::string, std::shared_ptr<Texture>> Scene::parseTexture(nlohmann::json &text) const {
	std::pair<std::string, std::shared_ptr<Texture>> texture;
	std::string type;
	try{
		texture.first = text.at("name");
		 type = text.at("type");
	} catch (std::exception &e){
		throw std::invalid_argument("Cannot find name or type of texture");
	}
	if(type == "SOLID_COLOR"){
		texture.second = std::make_shared<SolidColor>((text.contains("color")) ? parseVec3(text["color"]) : Color(0,0,0));
	} else if(type == "CHECKERED"){
		if(text.contains("color1") && text.contains("color2")){
			Color c1 = parseVec3(text["color1"]);
			Color c2 = parseVec3(text["color2"]);
			texture.second = std::make_shared<Checkered>(c1,c2);
		} else {
			texture.second = std::make_shared<Checkered>();
		}
	} else if(type == "IMAGE"){
		if (text.contains("path"))
			texture.second = std::make_shared<ImageTexture>(text["path"].get<std::string>());
		else 
			throw std::invalid_argument("Image Texture doesn't contains a valid path");
	}
	return texture;
}

int Scene::findMaterial(std::string &name) const {
	size_t i = 0;
	while(i < materials.size()){
		if(materials[i]->getName() == name) return i;
		++i;
	}
	return -1;
}

const MaterialPtr Scene::getMaterial(int idx) const {
	if(idx > materials.size())
		return nullptr;
	else return materials[idx];
}
