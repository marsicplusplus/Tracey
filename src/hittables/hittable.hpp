#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include "transform.hpp"
#include <memory>
#include <algorithm>

class Hittable {
public:
	Hittable(AABB aabb = AABB{ {0,0,0,0}, {0,0,0,0} }) : localBBox{ aabb }, worldBBox{ aabb } {}

	virtual bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const = 0;
	virtual bool update(float dt) { return false; }

	inline void translate(glm::fvec3 t){
		transform.translate(t);
		updateWorldBBox();
	}
	inline void scale(glm::fvec3 s){
		transform.scale(s);
		updateWorldBBox();
	}
	inline void scale(float s){
		transform.scale(s);
		updateWorldBBox();
	}
	inline void rotate(float t, glm::fvec3 a){
		transform.rotate(t, a);
		updateWorldBBox();
	}
	inline AABB getLocalAABB() const {
		return localBBox;
	}
	inline AABB getWorldAABB() const {
		return worldBBox;
	}
	inline glm::fvec4 getWorldAABBCentroid() const {
		return worldBBoxCentroid;
	}
	inline void setLocalAABB(AABB local) {
		localBBox = local;
		updateWorldBBox();
	}

protected:
	inline void updateWorldBBox() {
		std::vector<glm::fvec4> localVertices;
		localVertices.emplace_back(localBBox.min.x, localBBox.min.y, localBBox.min.z, 1.0f);
		localVertices.emplace_back(localBBox.min.x, localBBox.min.y, localBBox.max.z, 1.0f);
		localVertices.emplace_back(localBBox.min.x, localBBox.max.y, localBBox.min.z, 1.0f);
		localVertices.emplace_back(localBBox.min.x, localBBox.max.y, localBBox.max.z, 1.0f);
		localVertices.emplace_back(localBBox.max.x, localBBox.min.y, localBBox.min.z, 1.0f);
		localVertices.emplace_back(localBBox.max.x, localBBox.min.y, localBBox.max.z, 1.0f);
		localVertices.emplace_back(localBBox.max.x, localBBox.max.y, localBBox.min.z, 1.0f);
		localVertices.emplace_back(localBBox.max.x, localBBox.max.y, localBBox.max.z, 1.0f);

		worldBBox = { { INF, INF, INF, INF }, { -INF, -INF, -INF, -INF } };
		for (auto vertex : localVertices) {
			auto tranformedVertex = transform.getMatrix() * vertex;
			worldBBox.min.x = min(tranformedVertex.x, worldBBox.min.x);
			worldBBox.min.y = min(tranformedVertex.y, worldBBox.min.y);
			worldBBox.min.z = min(tranformedVertex.z, worldBBox.min.z);
			worldBBox.max.x = max(tranformedVertex.x, worldBBox.max.x);
			worldBBox.max.y = max(tranformedVertex.y, worldBBox.max.y);
			worldBBox.max.z = max(tranformedVertex.z, worldBBox.max.z);
		}

		worldBBoxCentroid = { (worldBBox.min.x + worldBBox.max.x) / 2.0f, (worldBBox.min.y + worldBBox.max.y) / 2.0f, (worldBBox.min.z + worldBBox.max.z) / 2.0f , INF };
	}

protected:
	glm::fvec3 position;
	Transform transform;
	AABB localBBox;
	AABB worldBBox;
	glm::fvec4 worldBBoxCentroid;
};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif