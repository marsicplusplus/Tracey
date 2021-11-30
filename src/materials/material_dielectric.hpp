#ifndef __MATERIAL_DIELETRIC_HPP__
#define __MATERIAL_DIELETRIC_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"
#include "glm/gtx/norm.hpp"
#include <iostream>

class DielectricMaterial : public Material {
	public: 
		DielectricMaterial(Color color, double _idx = 1.0) : albedo(std::make_shared<SolidColor>(color)), idx{_idx} {}
		DielectricMaterial(std::shared_ptr<Texture> t, double _idx = 1.0) : albedo(t) , idx{_idx} {}
		inline Materials getType() const override { return Materials::DIELECTRIC; }
		inline bool reflect(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &kd) const override {
#if 0
			attenuation = albedo->color(r.u, r.v, r.p);
			double n1 = (r.frontFace) ? 1.0 : idx;
			double n2 = (r.frontFace) ? idx : 1.0;
			double cosTheta = glm::dot(-in.getDirection(), r.normal);
			double ratio = n1/n2;
			double k = 1 - (ratio*ratio)*(1-(cosTheta * cosTheta));
			glm::dvec3 newDir;
			if (k < 0){
				/* TIR */
				kd=1.0;
			} else {
				/* Fresnel please help me */
				double sint = sqrt(1 - (cosTheta * cosTheta));
				double cost = sqrt(1.0 - ((ratio * sint)*(ratio * sint)));
				double rs = ((n1 * cosTheta - n2 * cost)/(n1 * cosTheta + n2 * cost));
				double rp = ((n1 * cost - n2 * cosTheta)/(n1 * cost + n2 * cosTheta));
				kd = (rs*rs+rp*rp)/2.0;
			}
			newDir = glm::reflect(in.getDirection(), r.normal);
			scattered = Ray(r.p + 0.0001 * newDir, newDir);
			return true;
#endif
#if 0
			double n1 = r.frontFace ? 1.0 : idx;
			double n2 = r.frontFace ? idx : 1.0;
			double ratio = n1 / n2;

			double cosTheta = dot(-in.getDirection(), r.normal);
			double k = ratio * sqrtf(std::max(0.0, 1.0 - cosTheta * cosTheta));
			glm::dvec3 newDir;
			// TIR
			if (k >= 1.0){
				newDir = glm::reflect(normDir, r.normal);
				kd = 1.0;
			} else {
				double cost = sqrtf(std::max(0.0, 1.0 - ratio * ratio)); 
				cosTheta = std::fabs(cosTheta); 
				float Rs = ((internalIdx * cosTheta) - (nextIdx * cost)) / ((internalIdx * cosTheta) + (nextIdx * cost)); 
				float Rp = ((nextIdx * cosTheta) - (internalIdx * cost)) / ((nextIdx * cosTheta) + (internalIdx * cost)); 
				kd = (Rs * Rs + Rp * Rp) / 2; 
				newDir = glm::reflect(normDir, r.normal);
			}
			scattered = Ray(r.p + 0.0001 * newDir, glm::normalize(newDir));
			return true;
#endif
		}
		inline virtual bool refract(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &s) const override {
			attenuation = albedo->color(r.u, r.v, r.p);
			double cosi = glm::dot(r.normal, in.getDirection());
			double n1 = r.frontFace ? 1.0 : idx;
			double n2 = r.frontFace ? idx : 1.0;
			double ratio = n1/n2;
			double k = 1.0 - ratio * ratio * (1.0-(cosi * cosi));
			if(k<0){
				s = 0;
				return false;
			} else {
				glm::dvec3 dir = ratio * in.getDirection() - (ratio * cosi + sqrt(k))*r.normal;
				scattered = Ray(r.p + 0.001 * dir, dir);
				return true;
			}
#if 0
			attenuation = albedo->color(r.u, r.v, r.p);
			double n1 = (r.frontFace) ? 1.0 : idx;
			double n2 = (r.frontFace) ? idx : 1.0;
			double cosTheta = glm::dot(-in.getDirection(), r.normal);
			double ratio = n1/n2;
			glm::dvec3 newDir = glm::refract(in.getDirection(), r.normal, ratio);
			scattered = Ray(r.p + 0.0001 * newDir, newDir);
			return true;
#endif
#if 0
			attenuation = albedo->color(r.u, r.v, r.p);
			double internalIdx = r.frontFace ? 1.0 : idx;
			double nextIdx = r.frontFace ? idx : 1.0;
			double ratio = internalIdx / (double)nextIdx;
			glm::dvec3 normDir = glm::normalize(in.getDirection());
			double cosTheta = dot(-normDir, r.normal);

			glm::dvec3 newDir = glm::refract(in.getDirection(), r.normal, ratio);
			scattered = Ray(r.p + 0.0001 * newDir, glm::normalize(newDir), nextIdx);
			return true;
#endif
		}

	private: 
		std::shared_ptr<Texture> albedo;
		double idx;
};

#endif
