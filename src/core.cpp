#include "core.hpp"

namespace Core {
	Color tracePath(Ray &ray, int bounces, ScenePtr scene, uint32_t &rng) {
		return Color{0,0,0};
	}

	Color traceWhitted(Ray &ray, int bounces, ScenePtr scene, uint32_t &rng) {
		HitRecord hr;
		hr.p = {INF, INF, INF};
		if(!scene || bounces <= 0)
			return Color{0,0,0};
		if (scene->traverse(ray, 0.001f, INF, hr)) {
			Ray reflectedRay;
			MaterialPtr mat = scene->getMaterial(hr.material);
			Color attenuation = scene->getTextureColor(mat->getAlbedoIdx(), hr.u, hr.v, hr.p);
			float reflectance = 1.0f;

			if (mat->getType() == Materials::DIFFUSE) {
				return attenuation * scene->traceLights(hr);
			} else if (mat->getType() == Materials::MIRROR) {

				mat->reflect(ray, hr, reflectedRay, reflectance);
				if (reflectance == 1.0f)
					return attenuation * (Core::traceWhitted(reflectedRay, bounces - 1, scene, rng));
				else
					return attenuation * (reflectance * Core::traceWhitted(reflectedRay, bounces - 1, scene, rng) + (1.0f - reflectance) * scene->traceLights(hr));
			} else if(mat->getType() == Materials::DIELECTRIC) {

				Color refractionColor(0.0f);
				Color reflectionColor(0.0f);
				float reflectance;
				mat->reflect(ray, hr, reflectedRay, reflectance);
				reflectionColor = Core::traceWhitted(reflectedRay, bounces - 1, scene, rng);

				if(reflectance < 1.0f){
					Ray refractedRay;
					float refractance;
					mat->refract(ray, hr, refractedRay, refractance);
					refractionColor = Core::traceWhitted(refractedRay, bounces-1, scene, rng);
				}

				mat->absorb(ray, hr, attenuation);

				return attenuation * (reflectionColor * reflectance + refractionColor * (1 - reflectance));
			}
			return Color{0,0,0};
		}
		return Color(0.0,0.0,0.0);
	}
};
