#ifndef __TRIANGLE_HPP__
#define __TRIANGLE_HPP__

#include "defs.hpp"
#include "bvh.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"
#include "tiny_obj_loader.h"

struct Triangle : Hittable {
	public:
		Triangle(
			glm::ivec3 fIdx,
			glm::ivec3 nIdx, 
			glm::ivec3 tIdx, 
			int m,
			std::vector<glm::fvec3>& pos, 
			std::vector<glm::fvec3>& norm,
			std::vector<glm::fvec2>& uvs
		);

		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;

		glm::fvec3 v0;
		glm::fvec3 v1;
		glm::fvec3 v2;

		glm::fvec3 n0;
		glm::fvec3 n1;
		glm::fvec3 n2;

		glm::fvec2 st0;
		glm::fvec2 st1;
		glm::fvec2 st2;

		int mat;
};

#endif
