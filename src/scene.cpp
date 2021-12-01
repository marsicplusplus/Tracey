#include "scene.hpp"

#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "json.hpp"
#include "textures/texture.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "materials/material.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"
#include "materials/material_diffuse.hpp"

#include <iostream>
#include <fstream>

glm::dvec3 parseArray(nlohmann::basic_json<> arr){
	return glm::dvec3(arr[0].get<double>(), arr[1].get<double>(),arr[2].get<double>());
}

Scene::Scene(){}
Scene::Scene(std::filesystem::path sceneFile){
	std::filesystem::path currPath = std::filesystem::current_path();
	std::filesystem::current_path(sceneFile.parent_path());
	std::ifstream i(sceneFile.filename());
	auto j = nlohmann::json::parse(i);
	std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
	std::unordered_map<std::string, MaterialPtr> materials;

	glm::dvec3 camPosition(parseArray(j["camera"]["position"]));
	glm::dvec3 cameraDir(parseArray(j["camera"]["dir"]));
	glm::dvec3 cameraUp(parseArray(j["camera"]["up"]));
	double fov = j["camera"]["fov"].get<double>();
	setCamera(std::make_shared<Camera>(camPosition, cameraDir, cameraUp, fov));
	for(auto t : j["textures"]){
		std::shared_ptr<Texture> text;
		if(t["type"] == "SOLID_COLOR"){
			text = std::make_shared<SolidColor>(parseArray(t["color"]));
		} else if(t["type"] == "CHECKERED"){
			if(t.contains("color1") && t.contains("color2")){
				Color c1 = parseArray(t["color1"]);
				Color c2 = parseArray(t["color2"]);
				text = std::make_shared<Checkered>(c1,c2);
			} else text = std::make_shared<Checkered>();
		} else if(t["type"] == "IMAGE"){
			text = std::make_shared<ImageTexture>(t["path"].get<std::string>());
		}
		textures[t["name"].get<std::string>()] = text;
	}
	for(auto m : j["materials"]){
		std::string type = m["type"].get<std::string>();
		std::string textname = m["texture"].get<std::string>();
		auto texture = textures.find(textname);
		std::shared_ptr<Material> mat;
		if(type == "DIFFUSE"){
			mat = std::make_shared<DiffuseMaterial>(texture->second);
		} else if(type == "MIRROR") {
			double ref = m["reflect"].get<double>();
			mat = std::make_shared<MirrorMaterial>(texture->second, ref);
		} else if (type == "DIELECTRIC"){
			double idx = m["idx"].get<double>();
			mat = std::make_shared<DielectricMaterial>(texture->second, idx);
		}
		materials[m["name"].get<std::string>()] = mat;
	}
	for(auto p : j["primitives"]){
		std::string type = p["type"].get<std::string>();
		auto material = materials[p["material"].get<std::string>()];
		glm::dvec3 pos(parseArray(p["position"]));
		if(type == "PLANE"){
			glm::dvec3 norm(parseArray(p["normal"]));
			hittables.push_back(std::make_shared<Plane>(pos, norm, material));
		} else if(type == "SPHERE"){
			double radius = p["radius"].get<double>();
			hittables.push_back(std::make_shared<Sphere>(pos, radius, material));
		} else if(type == "MESH"){
		}
	}
	for(auto l : j["lights"]){
		std::string type = l["type"].get<std::string>();
		glm::dvec3 color(parseArray(l["color"]));
		double intensity(l["intensity"].get<double>());
		if(type == "POINT"){
			glm::dvec3 pos(parseArray(l["position"]));
			addLight(std::make_shared<PointLight>(pos, intensity, color));
		} else if(type == "DIRECTIONAL"){
			glm::dvec3 dir(parseArray(l["direction"]));
			addLight(std::make_shared<DirectionalLight>(dir, intensity, color));
		} else if(type == "AMBIENT"){
			addLight(std::make_shared<AmbientLight>(intensity, color));
		}
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
		HitRecord obstruction;
		if(!traverse(shadowRay, 0.0001, tMax, obstruction)){
			auto contribution = light->getLight(rec, shadowRay);
			illumination += light->attenuate(contribution, rec.p);
		}
	}
	return illumination;
}
