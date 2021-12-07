#ifndef __PLANE_HPP__
#define __PLANE_HPP__

#include <iostream>
#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Plane : public Hittable {
	public:
		Plane(glm::fvec3 pos, glm::fvec3 norm, MaterialPtr mat) : pos{pos}, norm{norm}, mat{mat} {
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
		MaterialPtr mat;
};

class ZXRect : public Hittable {
	public:
		ZXRect(float y, glm::fvec4 size, MaterialPtr mat) : pos{y}, size{size}, mat{mat} {}
		inline bool hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const override {
			float t = (pos - ray.getOrigin().y)/static_cast<float>(ray.getDirection().y);
			if(t < tMin || t > tMax) return false;
			float x = ray.getOrigin().x + t * ray.getDirection().x;
			float z = ray.getOrigin().z + t * ray.getDirection().z;
			if(x < size.x || x > size.y || z < size.z || z > size.w) return false;
			rec.t = t;
			rec.setFaceNormal(ray, glm::fvec3(0.0f, 1.0f, 0.0f));
			rec.material = mat;
			rec.p = ray.at(t);
			getUV(rec);
			return true;
		}

		inline void getUV(HitRecord &rec) const {
			rec.u = (rec.p.x - size.x)/static_cast<float>(size.y - size.x);
			rec.v = (rec.p.z - size.z)/static_cast<float>(size.z - size.w);
		}

	private:
		float pos;
		glm::fvec4 size;
		MaterialPtr mat;
};

#endif
