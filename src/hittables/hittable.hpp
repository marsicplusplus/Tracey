#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include "transform.hpp"
#include <memory>
#include <algorithm>

class Hittable {
public:
	Hittable(AABB aabb = AABB{ 0,0,0,0,0,0 }) : localBBox{aabb}, worldBBox{aabb} {}

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
	inline glm::fvec3 getWorldAABBcentroid() const {
		return worldBBoxCentroid;
	}
	inline void setLocalAABB(AABB local) {
		localBBox = local;
		updateWorldBBox();
	}

protected:
	inline void updateWorldBBox() {
		std::vector<glm::fvec4> localVertices;
		localVertices.emplace_back( localBBox.minX, localBBox.minY, localBBox.minZ, 1.0f );
		localVertices.emplace_back( localBBox.minX, localBBox.minY, localBBox.maxZ, 1.0f );
		localVertices.emplace_back( localBBox.minX, localBBox.maxY, localBBox.minZ, 1.0f );
		localVertices.emplace_back( localBBox.minX, localBBox.maxY, localBBox.maxZ, 1.0f );
		localVertices.emplace_back( localBBox.maxX, localBBox.minY, localBBox.minZ, 1.0f );
		localVertices.emplace_back( localBBox.maxX, localBBox.minY, localBBox.maxZ, 1.0f );
		localVertices.emplace_back( localBBox.maxX, localBBox.maxY, localBBox.minZ, 1.0f );
		localVertices.emplace_back( localBBox.maxX, localBBox.maxY, localBBox.maxZ, 1.0f );

		worldBBox = { INF, INF, INF, -INF, -INF, -INF };
		for (auto vertex : localVertices) {
			auto tranformedVertex = transform.getMatrix() * vertex;
			worldBBox.minX = min(tranformedVertex.x, worldBBox.minX);
			worldBBox.minY = min(tranformedVertex.y, worldBBox.minY);
			worldBBox.minZ = min(tranformedVertex.z, worldBBox.minZ);
			worldBBox.maxX = max(tranformedVertex.x, worldBBox.maxX);
			worldBBox.maxY = max(tranformedVertex.y, worldBBox.maxY);
			worldBBox.maxZ = max(tranformedVertex.z, worldBBox.maxZ);
		}

		float cx = (worldBBox.minX + worldBBox.maxX) / 2.0f;
		float cy = (worldBBox.minY + worldBBox.maxY) / 2.0f;
		float cz = (worldBBox.minZ + worldBBox.maxZ) / 2.0f;
		worldBBoxCentroid = glm::fvec3(cx, cy, cz);
	}

protected:
	glm::fvec3 position;
	Transform transform;
	AABB localBBox;
	AABB worldBBox;
	glm::fvec3 worldBBoxCentroid;
};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif
