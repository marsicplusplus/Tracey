#ifndef __TRIANGLE_HPP__
#define __TRIANGLE_HPP__

#include "defs.hpp"
#include "bvh.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "hittables/triangle_mesh.hpp"
#include "glm/gtx/norm.hpp"

class Triangle : public Hittable {
	public:
		Triangle(const std::shared_ptr<TriangleMesh> &mesh, unsigned int triangleNumber, int material);
		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;

	private:
		void getUV(glm::vec2 uv[3]) const;

		std::shared_ptr<TriangleMesh> mesh;
		const unsigned int *vIdx; // Store the first of the 3 indices of the triangle
		const int mat;
};

#endif
