#include "scene.hpp"

#include "GLFW/glfw3.h"
#include "input_manager.hpp"

Scene::Scene(){}
Scene::Scene(std::string sceneFile){
	// TODO: Parse file
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
			i+=light->getLight(rec, ray);
			light->attenuate(i, rec.p);
		}
	}
	return i;
}
