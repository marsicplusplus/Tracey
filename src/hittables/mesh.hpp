#ifndef __MESH_HPP__
#define __MESH_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"
#include "tiny_obj_loader.h"

class Mesh : public Hittable {
	public:
		Mesh(std::vector<glm::dvec3> &p, std::vector<glm::dvec3> &n, std::vector<glm::dvec2> &t, std::vector<tinyobj::index_t> &idx, MaterialPtr m) : pos{p}, norm{n}, uvs{t}, indices{idx}, mat{m} {};
		bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override;

	private:
		std::vector<glm::dvec3> pos;
		std::vector<glm::dvec3> norm;
		std::vector<glm::dvec2> uvs;
		std::vector<tinyobj::index_t> indices;
		MaterialPtr mat;

		bool intersect(const glm::dvec3 &v0, const glm::dvec3 &v1, const glm::dvec3 &v2, const Ray& ray, double &t) const;
};

#endif
