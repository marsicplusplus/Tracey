#include "scene.hpp"

#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "json.hpp"
#include "textures/texture.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "hittables/torus.hpp"
#include "materials/material.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"
#include "materials/material_diffuse.hpp"

#include <iostream>
#include <fstream>

static glm::dvec3 parseVec3(nlohmann::basic_json<> &arr){
	return glm::dvec3(arr[0].get<double>(), arr[1].get<double>(),arr[2].get<double>());
}

static glm::dvec4 parseVec4(nlohmann::basic_json<> &arr){
	return glm::dvec4(arr[0].get<double>(), arr[1].get<double>(),arr[2].get<double>(), arr[3].get<double>());
}

void Scene::parseTransform(nlohmann::basic_json<> &hit, HittablePtr& primitive) const {
	if(hit.contains("transform")){
		auto trans = hit.at("transform");
		if(trans.contains("scale")){ 
			if(trans.at("scale").is_array()) primitive->scale(parseVec3(trans.at("scale")));
			else primitive->scale((double)(trans.at("scale")));
		}
		if(trans.contains("rotation")){ 
			auto rot = parseVec3(trans.at("rotation"));
			if(rot.x != 0) primitive->rotate(rot.x, glm::dvec3(1.0,0.0,0.0));
			if(rot.y != 0) primitive->rotate(rot.y, glm::dvec3(0.0,1.0,0.0));
			if(rot.z != 0) primitive->rotate(rot.z, glm::dvec3(0.0,0.0,1.0));
		}
		if(trans.contains("translation")) primitive->translate(parseVec3(trans.at("translation")));
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
		if (mat.second){
			materials.insert(mat);
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

bool Scene::traverse(const Ray &ray, double tMin, double tMax, HitRecord &rec) const {
	HitRecord tmp;
	bool hasHit = false;
	double closest = tMax;

	for (const auto& object : this->hittables) {
		if (object->hit(ray, tMin, closest, tmp)) {
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

CameraPtr Scene::getCamera() const {
	return this->currentCamera;
}

bool Scene::update(double dt){
	return this->currentCamera->update(dt);
}

Color Scene::traceLights(HitRecord &rec) const {
	Color illumination(0.0);
	for(auto &light : lights){
		double tMax;
		Ray shadowRay = light->getRay(rec, tMax);

		// Required for Spotlights
		if (shadowRay.getDirection() == glm::dvec3(0, 0, 0)) {
			continue;
		}

		HitRecord obstruction;
		if(!traverse(shadowRay, 0.0001, tMax, obstruction)){
			auto contribution = light->getLight(rec, shadowRay);
			illumination += light->attenuate(contribution, rec.p);
		}
	}
	return illumination;
}

CameraPtr Scene::parseCamera(nlohmann::json &cam) const {
	glm::dvec3 pos = parseVec3(cam.at("position"));
	glm::dvec3 dir = parseVec3(cam.at("dir"));
	glm::dvec3 up = parseVec3(cam.at("up"));
	double fov = (cam.at("fov"));
	return std::make_shared<Camera>(pos, dir, up, fov);	
}

std::shared_ptr<Hittable> Scene::parseHittable(nlohmann::json &hit) const {
	if(!hit.contains("type")) throw std::invalid_argument("Hittable doesn't name a type");
	std::string type = hit.at("type");
	if(!hit.contains("material")) throw std::invalid_argument("Hittable doesn't name a material");
	auto material = materials.find(hit.at("material"));
	if(material == materials.end()) throw std::invalid_argument("Hittable doesn't name a valid material");
	std::shared_ptr<Hittable> primitive = nullptr;
	if(type == "PLANE"){
		if(!hit.contains("normal")) throw std::invalid_argument("Plane doesn't specify a normal");
		glm::dvec3 norm(parseVec3(hit.at("normal")));
		glm::dvec3 pos(parseVec3(hit["position"]));
		primitive = (std::make_shared<Plane>(pos, norm, material->second));
	} else if(type == "SPHERE"){
		double radius = (hit.contains("radius")) ? hit["radius"].get<double>() : 0.5;
		primitive = (std::make_shared<Sphere>(radius, material->second));

	} else if (type == "TORUS"){
		double radiusMajor = (hit.contains("radiusMajor")) ? hit["radiusMajor"].get<double>() : 0.5;
		double radiusMinor = (hit.contains("radiusMinor")) ? hit["radiusMinor"].get<double>() : 0.1;
		primitive = (std::make_shared<Torus>(radiusMajor, radiusMinor, material->second));
	} else if(type == "ZXRect"){
		if(!hit.contains("size")) throw std::invalid_argument("ZXRect doesn't specify a correct size [x0, x1, z0, z1]");
		glm::dvec4 size(parseVec4(hit.at("size")));
		if(!hit.contains("y")) throw std::invalid_argument("ZXRect doesn't specify a correct y");
		double yCoord(hit.at("y"));
		primitive = std::make_shared<ZXRect>(yCoord, size, material->second);
	} else if(type == "MESH"){
		primitive = nullptr;
	} else{
		throw std::invalid_argument("Hittable doesn't name a valid type");
	}
	parseTransform(hit, primitive);
	return primitive;
}
std::shared_ptr<LightObject> Scene::parseLight(nlohmann::json &l) const{
	if(!l.contains("type")) throw std::invalid_argument("LightObject doesn't name a type");
	std::string type = l.at("type");
	Color color = (l.contains("color")) ? (parseVec3(l["color"])) : Color(1.0);
	double intensity = (l.contains("intensity")) ? (double)(l.at("intensity")) : 1.0;
	if(type == "POINT"){
		glm::dvec3 pos(parseVec3(l["position"]));
		return (std::make_shared<PointLight>(pos, intensity, color));
	} else if(type == "DIRECTIONAL"){
		glm::dvec3 dir(parseVec3(l["direction"]));
		return (std::make_shared<DirectionalLight>(dir, intensity, color));
	} else if(type == "AMBIENT"){
		return (std::make_shared<AmbientLight>(intensity, color));
	} else if (type == "SPOT") {
		glm::dvec3 pos(parseVec3(l["position"]));
		glm::dvec3 dir(parseVec3(l["direction"]));
		double cutoff = l.contains("cutoffAngle") ? (double)l.at("cutoffAngle") : 45.0;
		return (std::make_shared<SpotLight>(pos, dir, glm::radians(cutoff), intensity, color));
	} else {
		throw std::invalid_argument("LightObject doesn't name a valid type");
	}
}

std::pair<std::string, std::shared_ptr<Material>> Scene::parseMaterial(nlohmann::json &m) const {
	std::pair<std::string, std::shared_ptr<Material>> mat;
	if(!m.contains("name")) throw std::invalid_argument("Material is missing name");
	mat.first = m.at("name");
	if(!m.contains("type")) throw std::invalid_argument("Material is missing type");
	std::string type = m.at("type");
	if(!m.contains("texture")) throw std::invalid_argument("Material is missing texture");
	std::string textname = m.at("texture");
	auto texture = textures.find(textname);
	if(texture == textures.end()) throw std::invalid_argument("Material doesn't name a valid texture");
	if(type == "DIFFUSE"){
		mat.second = std::make_shared<DiffuseMaterial>(texture->second);
	} else if(type == "MIRROR") {
		double ref = m.contains("reflect") ? m["reflect"].get<double>() : 1.0;
		mat.second = std::make_shared<MirrorMaterial>(texture->second, ref);
	}
	else if (type == "DIELECTRIC") {
		double ior = (m.contains("ior")) ? (double)m.at("ior") : 1.0;
		Color absorption = m.contains("absorption") ? parseVec3(m.at("absorption")) : Color(1.0,1.0,1.0);
		mat.second = std::make_shared<DielectricMaterial>(texture->second, ior, absorption);
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
