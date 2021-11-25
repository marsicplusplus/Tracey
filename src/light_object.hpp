#ifndef __LIGHT_OBJECT_HPP__
#define __LIGHT_OBJECT_HPP__

#include "defs.hpp"
#include <memory>

// light.intensity * dot(n,l)/(length(N) * length(L))

class LightObject {
	public:
		LightObject(double i, Color c) : intensity{i}, color{c} {}
		virtual Ray getRay(const HitRecord &rec, double &tMax) const = 0;
		virtual inline Color getLight(const HitRecord &rec, Ray& ray, glm::dvec3 cameraPos) const {
			Color i(0.0);
			double nd = glm::dot(rec.normal,ray.getDirection());
			if( nd>0 )
				i += color * intensity * nd/static_cast<double>(rec.normal.length() * ray.getDirection().length());
			if(rec.material->getSpecular() > 0){
				glm::dvec3 R = 2.0 * rec.normal * glm::dot(rec.normal, ray.getDirection()) - ray.getDirection();
					double rv = glm::dot(R, cameraPos);
					if (rv > 0) {
						i += intensity * pow(rv/(glm::length(R) * glm::length(cameraPos)), rec.material->getSpecular());
					}

			}
			return i;
		};
	protected:
		double intensity;
		Color color;
};

class PointLight : public LightObject {
	public:
		PointLight(glm::dvec3 pos, double i, Color c) : LightObject(i, c), position{pos}{}
		inline Ray getRay(const HitRecord &rec, double &tMax) const override {
			tMax = 1;
			return Ray(rec.p, position - rec.p);
		}

	private:
		glm::dvec3 position;
};

class DirectionalLight : public LightObject {
	public:
		DirectionalLight(glm::dvec3 dir, double i, Color c) : LightObject(i, c), direction{dir}{}
		inline Ray getRay(const HitRecord &rec, double &tMax) const override {
			tMax = INF;
			return Ray(rec.p, -direction);
		}

	private:
		glm::dvec3 direction;
};

class AmbientLight : public LightObject {
	public:
		AmbientLight(double i, Color c) : LightObject(i,c) {}
		virtual Ray getRay(const HitRecord &rec, double &tMax) const override {
			Ray ray;
			return ray;
		}
		virtual inline Color getLight(const HitRecord &rec, Ray& ray, glm::dvec3 camPos) const override {
			return intensity * color;
		}
};

typedef std::shared_ptr<LightObject> LightObjectPtr;

#endif
