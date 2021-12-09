#ifndef __MESH_HPP__
#define __MESH_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"
#include "tiny_obj_loader.h"

struct Triangle {
	public:
		Triangle(glm::ivec3 fIdx, glm::ivec3 nIdx, glm::ivec3 tIdx, int m) : face{fIdx}, normal{nIdx}, texture{tIdx}, mat{m} {}

		glm::ivec3 face;
		glm::ivec3 normal;
		glm::ivec3 texture;
		int mat;
};


class Mesh : public Hittable {
	public:
		Mesh(std::vector<glm::fvec3> &p, std::vector<glm::fvec3> &n, std::vector<glm::fvec2> &t, std::vector<Triangle> &triangles, AABB box) : Hittable{box}, pos{p}, norm{n}, uvs{t}, tris{triangles} {};
		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;

	private:
		std::vector<glm::fvec3> pos;
		std::vector<glm::fvec3> norm;
		std::vector<glm::fvec2> uvs;
		std::vector<Triangle> tris;

		bool intersectTri(const Triangle &tri,const Ray& transformedRay, const Ray& ray, HitRecord &rec, float tMin, float tMax) const;
};

#endif
