#ifndef __MESH_HPP__
#define __MESH_HPP__
#include "hittables/hittable.hpp"
#include <iostream>

class Mesh : public Hittable {
public:
	Mesh(std::vector<glm::fvec3>& p, std::vector<glm::fvec3>& n, std::vector<glm::fvec2>& t, std::vector<HittablePtr>& triangles, AABB box) : Hittable{ box }, pos{ p }, norm{ n }, uvs{ t }, tris{ triangles } {};

	bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override {
		bool ret = false;

		if (!hitAABB(ray, localBBox)) {
			return false;
		}

		for (auto& tri : this->tris) {
			if (tri->hit(ray, tMin, tMax, rec)) {
				tMax = rec.t;
				ret = true;
			}
		}

		return ret;
	};

	std::vector<HittablePtr> tris;

private:
	std::vector<glm::fvec3> pos;
	std::vector<glm::fvec3> norm;
	std::vector<glm::fvec2> uvs;

};
#endif
