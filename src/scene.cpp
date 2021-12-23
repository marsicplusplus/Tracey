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
				meshes[name] = std::make_shared<BVH>(mesh->tris);
		}
	}

	for(auto l : j["lights"]){
		auto light = SceneParser::parseLight(l);
		if(light)
			lights.push_back(light);
	}
	topLevelBVH = SceneParser::parseSceneGraph(j["scenegraph"], materials, meshes, nTris);
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

void Scene::packetTraverse(std::vector<RayInfo>& packet, float tMin) const {
	Frustum frustum;

	auto corner = (packet.size() - 1) / 3;
	const auto p1 = packet[0 * corner];
	const auto p0 = packet[1 * corner];
	const auto p2 = packet[2 * corner];
	const auto p3 = packet[3 * corner];

	auto origin = p0.ray.getOrigin();
	frustum.normals[0] = glm::cross(p0.ray.getDirection() - origin, p1.ray.getDirection() - p0.ray.getDirection());
	frustum.normals[1] = glm::cross(p1.ray.getDirection() - origin, p2.ray.getDirection() - p1.ray.getDirection());
	frustum.normals[2] = glm::cross(p2.ray.getDirection() - origin, p3.ray.getDirection() - p2.ray.getDirection());
	frustum.normals[3] = glm::cross(p3.ray.getDirection() - origin, p1.ray.getDirection() - p3.ray.getDirection());

	frustum.offsets[0] = glm::dot(frustum.normals[0], origin);
	frustum.offsets[1] = glm::dot(frustum.normals[1], origin);
	frustum.offsets[2] = glm::dot(frustum.normals[2], origin);
	frustum.offsets[3] = glm::dot(frustum.normals[3], origin);

	topLevelBVH->packetHit(packet, frustum, tMin, 0, 0);
}


void Scene::setCamera(CameraPtr camera){
	this->currentCamera = camera;
}

const CameraPtr Scene::getCamera() const {
	return this->currentCamera;
}

bool Scene::update(float dt){
	return topLevelBVH->update(dt) || this->currentCamera->update(dt);
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
