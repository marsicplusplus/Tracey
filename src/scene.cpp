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

Scene::Scene(){}
Scene::Scene(std::string sceneFile){
		std::ifstream i(sceneFile);
		auto j = nlohmann::json::parse(i);
		std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
		std::unordered_map<std::string, MaterialPtr> materials;
		glm::dvec3 camPosition(j["camera"]["position"][0].get<double>(), j["camera"]["position"][1].get<double>(),j["camera"]["position"][2].get<double>());
		glm::dvec3 lookAt(j["camera"]["lookat"][0].get<double>(), j["camera"]["lookat"][1].get<double>(),j["camera"]["lookat"][2].get<double>());
		glm::dvec3 up(j["camera"]["up"][0].get<double>(), j["camera"]["up"][1].get<double>(),j["camera"]["up"][2].get<double>());
		double fov = j["camera"]["fov"].get<double>();
		setCamera(std::make_shared<Camera>(camPosition, lookAt, up, fov));
		for(auto t : j["textures"]){
			std::shared_ptr<Texture> text;
			if(t["type"] == "SOLID_COLOR"){
				text = std::make_shared<SolidColor>(Color(t["color"][0].get<double>(), t["color"][1].get<double>(), t["color"][2].get<double>()));
			} else if(t["type"] == "CHECKERED"){
				if(t.contains("color1") && t.contains("color2")){
					Color c1 = Color(t["color1"][0].get<double>(), t["color1"][1].get<double>(), t["color1"][2].get<double>());
					Color c2 = Color(t["color2"][0].get<double>(), t["color2"][1].get<double>(), t["color2"][2].get<double>());
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
			if(texture == textures.end()) std::cout << "ERROR WITH TEXTURE " << textname << std::endl;
			std::shared_ptr<Material> mat;
			if(type == "DIFFUSE"){
				mat = std::make_shared<DiffuseMaterial>(texture->second);
			} else if(type == "MIRROR") {
				double ref = m["reflect"].get<double>();
				mat = std::make_shared<MirrorMaterial>(texture->second, ref);
			} else if (type == "DIELECTRIC"){

			}
			materials[m["name"].get<std::string>()] = mat;
		}
		for(auto p : j["primitives"]){
			std::string type = p["type"].get<std::string>();
			auto material = materials[p["material"].get<std::string>()];
			glm::dvec3 pos(p["position"][0].get<double>(), p["position"][1].get<double>(), p["position"][2].get<double>());
			if(type == "PLANE"){
				glm::dvec3 norm(p["normal"][0].get<double>(), p["normal"][1].get<double>(), p["normal"][2].get<double>());
				hittables.push_back(std::make_shared<Plane>(pos, norm, material));
			} else if(type == "SPHERE"){
				double radius = p["radius"].get<double>();
				hittables.push_back(std::make_shared<Sphere>(pos, radius, material));
			//} else if(type == "MESH"){
			}
		}
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
	Color i(0.0);
	for(auto &light : lights){
		double tMax;
		Ray ray = light->getRay(rec, tMax);
		HitRecord shadow;
		if(!traverse(ray, 0.0001, tMax, shadow)){
			i+=light->getLight(rec, ray, this->currentCamera->getPosition());
		}
	}
	return i;
}
