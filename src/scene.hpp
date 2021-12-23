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

		const CameraPtr getCamera() const;
		void setCamera(CameraPtr camera);

		bool traverse(const Ray &ray, float tMin, float tMax, HitRecord &rec) const;
		void packetTraverse(std::vector<Ray> &corners, std::vector<RayInfo>& packet, float tMin) const;
		Color traceLights(HitRecord &rec) const;
		bool update(float dt);

		void addLight(LightObjectPtr light);

		inline const MaterialPtr getMaterial(int idx) {
			if (idx > materials.size())
				return nullptr;
			else return materials[idx];
		}

		inline const int getNTris() const {
			return nTris;
		}

	private:
		CameraPtr currentCamera;
		std::unordered_map<std::string, BVHPtr> meshes;
		std::unordered_map<std::string, std::list<BVHPtr>> BVHs;
		std::vector<LightObjectPtr> lights;
		std::vector<MaterialPtr> materials;
		std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
		BVHPtr topLevelBVH;
		int nTris = 0;

};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
