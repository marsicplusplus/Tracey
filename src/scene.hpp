#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include "camera.hpp"
#include "light_object.hpp"
#include "hittables/hittable.hpp"
#include "hittables/mesh.hpp"
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

		const CameraPtr getCamera() const;
		void setCamera(CameraPtr camera);

		bool traverse(const Ray &ray, float tMin, float tMax, HitRecord &rec) const;
		Color traceLights(HitRecord &rec) const;
		bool update(float dt);

		void addLight(LightObjectPtr light);

		inline const MaterialPtr getMaterial(int idx) {
			if (idx > materials.size())
				return nullptr;
			else return materials[idx];
		}

	private:
		CameraPtr currentCamera;
		std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
		std::vector<BVHPtr> BVHs;
		std::vector<LightObjectPtr> lights;
		std::vector<MaterialPtr> materials;
		std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
		BVHPtr topLevelBVH;

};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
