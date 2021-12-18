#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "scene_parser.hpp"

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
		if (text.second){
			textures.insert(text);
		}
	}
	for(auto m : j["materials"]){
		auto mat = SceneParser::parseMaterial(m, textures);
		if (mat){
			materials.push_back(mat);
		}
	}

	if(j.contains("instance_meshes")){
		for (auto m : j["instance_meshes"]) {
			std::string name;
			auto mesh = SceneParser::parseMeshInstance(m, materials, name);
			if (mesh)
				meshes[name] = mesh;
		}
	}

	for(auto l : j["lights"]){
		auto light = SceneParser::parseLight(l);
		if(light)
			lights.push_back(light);
	}

	topLevelBVH = SceneParser::parseSceneGraph(j["scenegraph"], materials, meshes);

	std::filesystem::current_path(currPath);
}
Scene::~Scene(){}

void Scene::addLight(LightObjectPtr light){
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
