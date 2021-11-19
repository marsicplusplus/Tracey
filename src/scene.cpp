#include "scene.hpp"

Scene::Scene(){}
Scene::Scene(std::string sceneFile){
	// TODO: Parse file
}
Scene::~Scene(){}

void Scene::addHittable(HittablePtr hittable){
	hittables.push_back(hittable);
}

bool Scene::traverse(const Ray &ray, double tMin, double tMax, HitRecord &rec){
	HitRecord tmp;
	bool hasHit = false;
	double closest = tMax;

	for (const auto& object : hittables) {
		if (object->hit(ray, tMin, closest, tmp)) {
			hasHit = true;
			closest = tmp.t;
			rec = tmp;
		}
	}
	return hasHit;
}
