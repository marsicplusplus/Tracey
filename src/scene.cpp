#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "scene_parser.hpp"
#include "glm/trigonometric.hpp"

#include <fstream>

Scene::Scene(){}
Scene::Scene(std::filesystem::path sceneFile){
	std::filesystem::path currPath = std::filesystem::current_path();
	std::filesystem::current_path(sceneFile.parent_path());
	std::ifstream i(sceneFile.filename());
	auto j = nlohmann::json::parse(i);

	auto camera = SceneParser::parseCamera(j["camera"]);
	setCamera(camera);

	for(auto t : j["textures"]){
		auto text = SceneParser::parseTexture(t);
		if (text){
			textures.push_back(std::move(text));
		}
	}
	for(auto m : j["materials"]){
		auto mat = SceneParser::parseMaterial(m, textures);
		if (mat){
			materials.push_back(mat);
		}
	}
	std::cout << "Building BVHs..." << std::endl;
	if(j.contains("instance_meshes")){
		for (auto m : j["instance_meshes"]) {
			std::string name;
			std::string materialName;
			auto mesh = SceneParser::parseMeshInstance(m, materials, textures, meshes, name);
			if (mesh){
				std::cout << name << ": done!" << std::endl;
				meshesBVH[name] = mesh;
			}
		}
	}

	for(auto l : j["lights"]){
		auto light = SceneParser::parseLight(l);
		if(light)
			lights.push_back(light);
	}
	topLevelBVH = SceneParser::parseSceneGraph(j["scenegraph"], materials, meshesBVH, BVHs, nTris);
	std::cout << "Total Number of triangles: " << nTris << std::endl;
	std::filesystem::current_path(currPath);
}

Scene::~Scene(){}

void Scene::addLight(std::shared_ptr<LightObject> light){
	this->lights.push_back(light);
}

bool Scene::traverse(const Ray &ray, float tMin, float tMax, HitRecord &rec) const {
	HitRecord tmp;
	tmp.p = {INF, INF, INF};
	bool hasHit = false;
	float closest = tMax;

	if (topLevelBVH->hit(ray, tMin, closest, tmp)) {
		hasHit = true;
		closest = tmp.t;
		rec = tmp;
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
	bool ret = false;
	for(auto &m : meshesBVH){
		if(m.second->update(dt)){
			auto instanceList = BVHs[m.first];
			for(auto &i : instanceList){
				i->constructSubBVH();
			}
			ret = true;
		}
	}
	for(auto &b : BVHs){
		auto instanceList = b.second;
		for(auto &i : instanceList){
			ret|=i->update(dt);
		}
	}
	if(ret) topLevelBVH->constructTopLevelBVH();
	return ret | this->currentCamera->update(dt);
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
		if(!traverse(shadowRay, EPS, tMax, obstruction)){
			auto contribution = light->getLight(rec, shadowRay);
			illumination += light->attenuate(contribution, rec.p);
		}
	}
	return illumination;
}
