#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include "camera.hpp"
#include "light_object.hpp"
#include "hittables/hittable.hpp"
#include "json.hpp"
#include "bvh.hpp"

#include <vector>
#include <string>
#include <filesystem>

class Scene {
	public:
		Scene(std::filesystem::path sceneFile);
		Scene();
		~Scene();

		const MaterialPtr getMaterial(int idx) const;
		const CameraPtr getCamera() const;
		void setCamera(CameraPtr camera);

		bool traverse(const Ray &ray, float tMin, float tMax, HitRecord &rec) const;
		Color traceLights(HitRecord &rec) const;
		bool update(float dt);

		void addLight(LightObjectPtr light);
		void addHittable(HittablePtr hittable);

	private:
		CameraPtr currentCamera;
		std::vector<HittablePtr> hittables;
		std::vector<LightObjectPtr> lights;
		std::vector<MaterialPtr> materials;
		std::unordered_map<std::string, std::shared_ptr<Texture>> textures;


		CameraPtr parseCamera(nlohmann::json &cam) const;
		std::pair<std::string, std::shared_ptr<Texture>> parseTexture(nlohmann::json &text) const;
		std::shared_ptr<Material> parseMaterial(nlohmann::json &text) const;
		std::shared_ptr<Hittable> parseHittable(nlohmann::json &text) const;
		std::shared_ptr<LightObject> parseLight(nlohmann::json &text) const;
		std::shared_ptr<Hittable> parseMesh(std::filesystem::path &path, int mat) const;
		
		void parseTransform(nlohmann::basic_json<> &hit, HittablePtr& primitive) const;
		
		int findMaterial(std::string &name) const;
};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
