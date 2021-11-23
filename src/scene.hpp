#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include "camera.hpp"

#include <vector>
#include <hittables/hittable.hpp>
#include <string>

class Scene {
	public:
		Scene(std::string sceneFile);
		Scene();
		~Scene();

		bool traverse(const Ray &ray, double tMin, double tMax, HitRecord &rec) const;
		void addHittable(HittablePtr hittable);
		void setCamera(CameraPtr camera);
		CameraPtr getCamera() const;
		bool update(double dt);
		//void addLight(LightObject light);

	private:
		std::vector<HittablePtr> hittables;
		CameraPtr currentCamera;
		//std::vector<LightObjectPtr> lights;
};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
