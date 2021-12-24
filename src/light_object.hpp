#ifndef __LIGHT_OBJECT_HPP__
#define __LIGHT_OBJECT_HPP__

#include "defs.hpp"
#include <memory>

// light.intensity * dot(n,l)/(length(N) * length(L))

enum class Lights {
	NILL,
	DIRECTIONAL,
	POINT,
	SPOT,
	AMBIENT
};

class LightObject {

	public:
		LightObject(float i, Color c) : intensity{i}, color{c} {}
		virtual ~LightObject() {};

		virtual inline Lights getType() {return Lights::NILL; }

		virtual Ray getRay(const HitRecord &rec, float &tMax) const = 0;

		virtual Color attenuate(Color color, const glm::fvec3& p) { return color; };

		virtual inline Color getLight(const HitRecord &rec, Ray& ray) const {
			Color illumination(0.0f);
			float nd = glm::dot(rec.normal, ray.getDirection());
			if(nd > 0.0f){
				illumination += this->color * this->intensity * nd;
			}
			return illumination;
		};

	protected:
		float intensity;
		Color color;
};




class PointLight : public LightObject {

	public:
		PointLight(glm::fvec3 pos, float i, Color c) : LightObject(i, c), position{pos}{}

		inline Lights getType() override {return Lights::POINT; }

		inline Ray getRay(const HitRecord &rec, float &tMax) const override {
			tMax = glm::distance(this->position, rec.p);
			auto dir = this->position - rec.p;
			return Ray(rec.p + 0.001f * dir, dir);
		}

		inline Color attenuate(Color color, const glm::fvec3 &p) override {
			return color * 1.0f / glm::distance(this->position, p);
		}

	private:
		glm::fvec3 position;
};




class SpotLight : public LightObject {

public:
	SpotLight(glm::fvec3 pos, glm::fvec3 dir, float cutoff, float i, Color c) : LightObject(i, c), position{pos}, direction{dir}, cutoffAngle{cutoff} {}

	inline Lights getType() override { return Lights::SPOT; }

	inline Ray getRay(const HitRecord& rec, float& tMax) const override {
		tMax = glm::distance(this->position, rec.p);
		auto dir = this->position - rec.p;
		auto hitToLight = Ray(rec.p + 0.001f * dir, dir);

		auto angle = std::acos(glm::dot(direction, glm::normalize(rec.p - this->position)));
		if (angle > cutoffAngle) {
			return Ray(rec.p, glm::fvec3(0, 0, 0));
		} else {
			return hitToLight;
		}
	}

	inline Color attenuate(Color color, const glm::fvec3& p) override {
		return color * 1.0f / glm::distance(this->position, p);
	}

private:
	glm::fvec3 position;
	glm::fvec3 direction;
	float cutoffAngle;
};


class DirectionalLight : public LightObject {

	public:
		DirectionalLight(glm::fvec3 dir, float i, Color c) : LightObject(i, c), direction{dir}{}

		inline Lights getType() override {return Lights::DIRECTIONAL; }

		inline Ray getRay(const HitRecord &rec, float &tMax) const override {
			tMax = INF;
			return Ray(rec.p + 0.001f*(-this->direction), -this->direction);
		}

	private:
		glm::fvec3 direction;
};




class AmbientLight : public LightObject {

	public:
		AmbientLight(float i, Color c) : LightObject(i,c) {}

		inline Lights getType() override {return Lights::AMBIENT; }

		Ray getRay(const HitRecord &rec, float &tMax) const override {
			tMax = 0.0001f;
			Ray ray(rec.p + 0.001f * rec.normal, rec.normal);
			return ray;
		}

		inline Color getLight(const HitRecord &rec, Ray& ray) const override {
			return this->intensity * this->color;
		}
};

typedef std::unique_ptr<LightObject> LightObjectPtr;

#endif
