#ifndef __SPHERE_HPP__
#define __SPHERE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Sphere : public Hittable {
	public:
		Sphere(float r, int mat) : radius{r}, radiusSquared{r * r}, mat{mat}, bbox{-radius, -radius, -radius, radius, radius, radius} {}
		inline bool hitSelf(const Ray &ray, float tMin, float tMax, HitRecord &rec) const override {
			const auto transformedRay = ray.transformRay(transformInv);

			if (!hitAABB(transformedRay, bbox)) {
				return false;
			}

			const auto transformedRayDir = transformedRay.getDirection();
			const auto transformedOrigin = transformedRay.getOrigin();

			/* Inefficent, but needed */
			glm::fvec3 oc = transformedOrigin;
			auto a = glm::dot(transformedRayDir, transformedRayDir);
			auto b = 2.0 * glm::dot(oc, transformedRayDir);
			auto c = glm::dot(oc, oc) - radius*radius;
			auto discriminant = b*b - 4*a*c;
			if(discriminant >= 0){
				float sq = sqrt(discriminant);
				float t0 = (-b - sq ) / (2.0*a);
				float t1 = (-b + sq ) / (2.0*a);
				float cT;
				if(t0 < t1 && t0 > 0) cT = t0;
				else if(t1 > 0) cT = t1;
				else return false;
				if(cT < tMax && cT > tMin){
					auto localp = transformedRay.at(cT);
					rec.t = cT;
					rec.p = transform * glm::vec4(localp, 1.0f);
					rec.material = mat;
					glm::fvec3 normal = (localp) / radius;
					auto transformedNormal = transposeInv * glm::dvec4(normal, 0);
					rec.setFaceNormal(ray, transformedNormal);
					getUV(rec);
					return true;
				}
			}
			return false;
}

inline void getUV(HitRecord &rec) const {
			float theta = std::acos(-rec.normal.y);
			float phi = std::atan2(-rec.normal.z, rec.normal.x) + PI;
			rec.u = static_cast<float>(phi)/static_cast<float>(2*PI);
			rec.v = static_cast<float>(theta)/static_cast<float>(PI);
		}

	private:
		float radius;
		float radiusSquared;
		int mat;
		AABB bbox;
};

#endif
