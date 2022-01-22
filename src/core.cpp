#include "core.hpp"

namespace Core {
	Color tracePath(Ray &ray, int bounces, ScenePtr scene, uint32_t &rng) {
		return Color{0,0,0};
	}

	Color traceWhitted(Ray &ray, int bounces, ScenePtr scene, uint32_t &rng) {
		HitRecord hr;
		hr.p = {INF, INF, INF};
		Color c(0.0,0.0,0.0);
		Ray current = ray;
		float frac = 1.0f;
		while(scene && bounces > 0){
			if (scene->traverse(current, 0.001f, INF, hr)) {
				Ray reflectedRay;
				MaterialPtr mat = scene->getMaterial(hr.material);
				Color attenuation = scene->getTextureColor(mat->getAlbedoIdx(), hr.u, hr.v, hr.p);
				float reflectance = 1.0f;
				if (mat->getType() == Materials::DIFFUSE || mat->getType() == Materials::DIELECTRIC) {
					c += attenuation * scene->traceLights(hr) * frac;
					break;
				} else if(mat->getType() == Materials::MIRROR) {
					mat->reflect(current, hr, reflectedRay, reflectance);
					current = reflectedRay;
					bounces--;

					c += attenuation * scene->traceLights(hr) * (1.0f - reflectance) * frac;
					frac *= reflectance;
					if(reflectance == 0.0f) break;
					continue;
				} else {
					break;
				}
			}else{
				break;
			}
		}
		return c;
	}
};
