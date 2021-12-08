#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include "camera.hpp"
#include "light_object.hpp"
#include "hittables/hittable.hpp"
#include "json.hpp"

#include <vector>
#include <string>
#include <filesystem>

class Scene {
	public:
		Scene(std::filesystem::path sceneFile);
		Scene();
		~Scene();

		bool traverse(const Ray &ray, float tMin, float tMax, HitRecord &rec) const;
		Color traceLights(HitRecord &rec) const;
		void addHittable(HittablePtr hittable);
		void setCamera(CameraPtr camera);
		CameraPtr getCamera() const;
		bool update(float dt);
		void addLight(LightObjectPtr light);

	private:
		std::vector<HittablePtr> hittables;
		CameraPtr currentCamera;
		std::vector<LightObjectPtr> lights;
		std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
		std::unordered_map<std::string, MaterialPtr> materials;

		CameraPtr parseCamera(nlohmann::json &cam) const;
		std::pair<std::string, std::shared_ptr<Texture>> parseTexture(nlohmann::json &text) const;
		std::pair<std::string, std::shared_ptr<Material>> parseMaterial(nlohmann::json &text) const;
		std::shared_ptr<Hittable> parseHittable(nlohmann::json &text) const;
		std::shared_ptr<LightObject> parseLight(nlohmann::json &text) const;
		std::shared_ptr<Hittable> parseMesh(std::filesystem::path &path, std::shared_ptr<Material> mat) const;
		void parseTransform(nlohmann::basic_json<> &hit, HittablePtr& primitive) const;
};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
