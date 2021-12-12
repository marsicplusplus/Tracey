#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include <memory>
#include <algorithm>

class Hittable {
public:
	Hittable(AABB aabb = AABB{ 0,0,0,0,0,0 }) : localBBox{aabb}, worldBBox{aabb}, transform{ glm::fmat4x4(1.0) }, transformInv{ glm::fmat4x4(1.0) }, transposeInv{ glm::fmat4x4(1.0) } {}

	virtual bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const = 0;

	inline void translate(glm::fvec3 t){
		transform = glm::translate(transform, t);
		transformInv = glm::inverse(transform);
		updateTranspose();
		updateWorldBBox();
	}
	inline void scale(glm::fvec3 s){
		transform = glm::scale(transform, s);
		transformInv = glm::inverse(transform);
		updateTranspose();
		updateWorldBBox();
	}
	inline void scale(float s){
		transform = glm::scale(transform, glm::fvec3(s,s,s));
		transformInv = glm::inverse(transform);
		updateTranspose();
		updateWorldBBox();
	}
	inline void rotate(float t, glm::fvec3 a){
		transform = glm::rotate(transform, t, a);
		transformInv = glm::inverse(transform);
		updateTranspose();
		updateWorldBBox();
	}
	inline AABB getLocalAABB() const {
		return localBBox;
	}
	inline AABB getWorldAABB() const {
		return worldBBox;
	}

protected:
	inline void updateTranspose() {
		transposeInv = glm::transpose(transformInv);
	}

	inline void updateWorldBBox() {
		std::vector<glm::fvec4> localVertices;
		localVertices.push_back({ localBBox.minX, localBBox.minY, localBBox.minZ, 1.0f });
		localVertices.push_back({ localBBox.minX, localBBox.minY, localBBox.maxZ, 1.0f });
		localVertices.push_back({ localBBox.minX, localBBox.maxY, localBBox.minZ, 1.0f });
		localVertices.push_back({ localBBox.minX, localBBox.maxY, localBBox.maxZ, 1.0f });
		localVertices.push_back({ localBBox.maxX, localBBox.minY, localBBox.minZ, 1.0f });
		localVertices.push_back({ localBBox.maxX, localBBox.minY, localBBox.maxZ, 1.0f });
		localVertices.push_back({ localBBox.maxX, localBBox.maxY, localBBox.minZ, 1.0f });
		localVertices.push_back({ localBBox.maxX, localBBox.maxY, localBBox.maxZ, 1.0f });

		worldBBox = { INF, INF, INF, -INF, -INF, -INF };
		for (auto vertex : localVertices) {
			auto tranformedVertex = transform * vertex;
			worldBBox.minX = min(tranformedVertex.x, worldBBox.minX);
			worldBBox.minY = min(tranformedVertex.y, worldBBox.minY);
			worldBBox.minZ = min(tranformedVertex.z, worldBBox.minZ);
			worldBBox.maxX = max(tranformedVertex.x, worldBBox.maxX);
			worldBBox.maxY = max(tranformedVertex.y, worldBBox.maxY);
			worldBBox.maxZ = max(tranformedVertex.z, worldBBox.maxZ);
		}
	}

protected:
	glm::fvec3 position;
	glm::fmat4x4 transformInv;
	glm::fmat4x4 transform;
	glm::fmat4x4 transposeInv;
	AABB localBBox;
	AABB worldBBox;
};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif
