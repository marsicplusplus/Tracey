#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include "camera.hpp"
#include "light_object.hpp"
#include "hittables/hittable.hpp"
#include "hittables/triangle.hpp"
#include "hittables/triangle_mesh.hpp"
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

		void addLight(std::shared_ptr<LightObject> light);

		inline const MaterialPtr getMaterial(int idx) {
			if (idx > materials.size())
				return nullptr;
			else return materials[idx];
		}

		inline Color getTextureColor(int idx, float u, float v, glm::fvec3 &p){
			if(idx == -1) return Color(0.5, 0.5, 1.0);
			return textures[idx]->color(u, v, p);
		}

		inline const int getNTris() const {
			return nTris;
		}

		void getTextureBuffer(std::vector<CompactTexture> &textures, std::vector<unsigned char> &imgs);
		void getMeshBuffer(std::vector<CompactTriangle> &tris, std::vector<BVHNode> &bvhs, std::vector<CompactMesh> &meshes);
	private:
		CameraPtr currentCamera;
		std::unordered_map<std::string, BVHPtr> meshesBVH;
		std::unordered_map<std::string, std::list<BVHPtr>> BVHs;
		std::vector<std::shared_ptr<LightObject>> lights;
		std::vector<MaterialPtr> materials;
		std::vector<TexturePtr> textures;
		std::vector<std::shared_ptr<TriangleMesh>> meshes;
		BVHPtr topLevelBVH;
		int nTris = 0;

};

typedef std::shared_ptr<Scene> ScenePtr;

#endif
