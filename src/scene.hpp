#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include <vector>
#include <hittables/hittable.hpp>
#include <string>

class Scene {
	public:
		Scene(std::string sceneFile);
		Scene();
		~Scene();

		bool traverse(const Ray &ray, double tMin, double tMax, HitRecord &rec);
		void addHittable(HittablePtr hittable);
		//void addLight(LightObject light);

	private:
		std::vector<HittablePtr> hittables;
		//std::vector<LightObjectPtr> lights;
};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
