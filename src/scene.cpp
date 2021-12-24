#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "materials/material_diffuse.hpp"
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

	if(j.contains("instance_meshes")){
		for (auto m : j["instance_meshes"]) {
			std::string name;
			auto mesh = SceneParser::parseMeshInstance(m, materials, textures, name);
			if (mesh)
				meshes[name] = mesh;
		}
	}

	for(auto l : j["lights"]){
		auto light = SceneParser::parseLight(l);
		if(light)
			lights.push_back(light);
	}
	topLevelBVH = SceneParser::parseSceneGraph(j["scenegraph"], materials, meshes, BVHs, nTris);
	std::cout << "Total Number of triangles: " << nTris << std::endl;
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

void Scene::packetTraverse(std::vector<Ray>& corners, std::vector<RayInfo>& packet, float tMin) const {
	Frustum frustum;
	auto O = corners[0].getOrigin();
	frustum.normals[0] = glm::cross(corners[0].getDirection() - O, corners[1].getDirection() - corners[0].getDirection()); 
	frustum.normals[1] = glm::cross(corners[1].getDirection() - O, corners[2].getDirection() - corners[1].getDirection()); 
	frustum.normals[2] = glm::cross(corners[2].getDirection() - O, corners[3].getDirection() - corners[2].getDirection()); 
	frustum.normals[3] = glm::cross(corners[3].getDirection() - O, corners[0].getDirection() - corners[3].getDirection()); 
	frustum.offsets[0] = glm::dot(frustum.normals[0], O);
	frustum.offsets[1] = glm::dot(frustum.normals[1], O);
	frustum.offsets[2] = glm::dot(frustum.normals[2], O);
	frustum.offsets[3] = glm::dot(frustum.normals[3], O);
	topLevelBVH->packetHit(packet, frustum, tMin, 0, 0);
}


void Scene::setCamera(CameraPtr camera){
	this->currentCamera = camera;
}

const CameraPtr Scene::getCamera() const {
	return this->currentCamera;
}

bool Scene::update(float dt){
	bool ret = false;
	for(auto &m : meshes){
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
		if(!traverse(shadowRay, 0.0001f, tMax, obstruction)){
			auto contribution = light->getLight(rec, shadowRay);
			illumination += light->attenuate(contribution, rec.p);
		}
	}
	return illumination;
}
