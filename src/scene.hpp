#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include "camera.hpp"
#include "light_object.hpp"
#include "hittables/hittable.hpp"

#include <vector>
#include <string>
#include <filesystem>

class Scene {
	public:
		Scene(std::filesystem::path sceneFile);
		Scene();
		~Scene();

		bool traverse(const Ray &ray, double tMin, double tMax, HitRecord &rec) const;
		Color traceLights(HitRecord &rec) const;
		void addHittable(HittablePtr hittable);
		void setCamera(CameraPtr camera);
		CameraPtr getCamera() const;
		bool update(double dt);
		void addLight(LightObjectPtr light);

	private:
		std::vector<HittablePtr> hittables;
		CameraPtr currentCamera;
		std::vector<LightObjectPtr> lights;
};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
