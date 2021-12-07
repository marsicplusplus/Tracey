#ifndef __MESH_HPP__
#define __MESH_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Mesh : public Hittable {
	public:
		Mesh(std::string meshPath);
		inline bool hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const override;

	private:
		MaterialPtr mat;
};

#endif
