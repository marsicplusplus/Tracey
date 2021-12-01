#ifndef __LIGHT_OBJECT_HPP__
#define __LIGHT_OBJECT_HPP__

#include "defs.hpp"
#include <memory>

// light.intensity * dot(n,l)/(length(N) * length(L))

enum class Lights {
	NILL,
	DIRECTIONAL,
	POINT,
	AMBIENT
};

class LightObject {

	public:
		LightObject(double i, Color c) : intensity{i}, color{c} {}

		virtual inline Lights getType() {return Lights::NILL; }

		virtual Ray getRay(const HitRecord &rec, double &tMax) const = 0;

		virtual Color attenuate(Color color, const glm::dvec3& p) { return color; };

		virtual inline Color getLight(const HitRecord &rec, Ray& ray) const {
			Color illumination(0.0);
			double nd = glm::dot(rec.normal,ray.getDirection());
			if(nd > 0){
				illumination += this->color * this->intensity * nd;
			}
			return illumination;
		};

	protected:
		double intensity;
		Color color;
};




class PointLight : public LightObject {

	public:
		PointLight(glm::dvec3 pos, double i, Color c) : LightObject(i, c), position{pos}{}

		virtual inline Lights getType() override {return Lights::POINT; }

		inline Ray getRay(const HitRecord &rec, double &tMax) const override {
			tMax = glm::distance(this->position, rec.p);
			return Ray(rec.p, this->position - rec.p);
		}

		inline Color attenuate(Color color, const glm::dvec3 &p) override {
			return color * 1.0 / glm::distance(this->position, p);
		}

	private:
		glm::dvec3 position;
};




class DirectionalLight : public LightObject {

	public:
		DirectionalLight(glm::dvec3 dir, double i, Color c) : LightObject(i, c), direction{dir}{}

		virtual inline Lights getType() override {return Lights::DIRECTIONAL; }

		inline Ray getRay(const HitRecord &rec, double &tMax) const override {
			tMax = INF;
			return Ray(rec.p, -this->direction);
		}

	private:
		glm::dvec3 direction;
};




class AmbientLight : public LightObject {

	public:
		AmbientLight(double i, Color c) : LightObject(i,c) {}

		virtual inline Lights getType() override {return Lights::AMBIENT; }

		virtual Ray getRay(const HitRecord &rec, double &tMax) const override {
			Ray ray;
			return ray;
		}

		virtual inline Color getLight(const HitRecord &rec, Ray& ray) const override {
			return this->intensity * this->color;
		}
};

typedef std::shared_ptr<LightObject> LightObjectPtr;

#endif
