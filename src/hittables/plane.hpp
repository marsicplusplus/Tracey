#ifndef __PLANE_HPP__
#define __PLANE_HPP__

#include <iostream>
#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Plane : public Hittable {
	public:
		Plane(glm::fvec3 pos, glm::fvec3 norm, int mat) : Hittable{}, pos{pos}, norm{norm}, mat{mat} {
			glm::fvec3 a = cross(norm, glm::fvec3(1, 0, 0));
			glm::fvec3 b = cross(norm, glm::fvec3(0, 1, 0));
			glm::fvec3 maxAB = glm::dot(a, a) < glm::dot(b, b) ? b : a;
			glm::fvec3 c = glm::cross(norm, glm::fvec3(0, 0, 1));
			uAxis = (glm::dot(maxAB, maxAB) < glm::dot(c, c)) ? c : maxAB;
			vAxis  = glm::cross(norm, uAxis);
		}

		inline bool hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const override {
			float d = glm::dot(this->norm, ray.getDirection());
			if(std::abs(d) > 1e-6){
				glm::fvec3 p = pos - ray.getOrigin();
				float t = glm::dot(p, norm) / static_cast<float>(d);
				if(t >= 1e-6 && t > tMin && t < tMax) {
					rec.t = t;
					rec.setFaceNormal(ray, norm);
					rec.material = mat;
					rec.p = ray.at(t);
					getUV(rec);
					return true;
				}
			}
			return false;
		}

		inline void getUV(HitRecord &rec) const {
			rec.u = glm::dot(rec.p, uAxis);
			rec.v = glm::dot(rec.p, vAxis);
		}

	private:
		glm::fvec3 pos;
		glm::fvec3 uAxis;
		glm::fvec3 vAxis;
		glm::fvec3 norm;
		int mat;
};

class ZXRect : public Hittable {
	public:
		ZXRect(int mat) : Hittable{}, mat{mat} {}
		inline bool hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const override {
			const auto transformedRay = ray.transformRay(transformInv);
			const auto transformedRayDir = transformedRay.getDirection();
			const auto transformedOrigin = transformedRay.getOrigin();

			if (rec.p != glm::fvec3{ INF, INF, INF }) {
				tMax = glm::distance(transformedRay.getOrigin(), glm::fvec3(transformInv * glm::fvec4(rec.p, 1.0f)));
			}

			float t = (pos - transformedOrigin.y)/static_cast<float>(transformedRayDir.y);
			if(t < tMin || t > tMax) return false;
			auto localp = transformedRay.at(t);
			if(localp.x < -0.5f || localp.x > 0.5f || localp.z < -0.5f || localp.z > 0.5f) return false;
			rec.t = glm::distance(rec.p, ray.getOrigin());
			rec.p = transposeInv * glm::fvec4(localp, 1.0);
			rec.setFaceNormal(ray, glm::normalize(transform * glm::fvec4(0.0f, 1.0f, 0.0f, 0.0f)));
			rec.material = mat;
			getUV(rec, localp);
			return true;
		}

		inline void getUV(HitRecord &rec, glm::fvec3 p) const {
			rec.u = (p.x + 0.5f);
			rec.v = -(p.z + 0.5);
		}

	private:
		float pos;
		int mat;
};

#endif
