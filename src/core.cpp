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
		return Color(0.4,0.4,0.4);
	}

	void packetTrace(std::vector<Ray> &corners, std::vector<RayInfo>& packet, int bounces, const ScenePtr scene, uint32_t &rng) {
	if (!scene || bounces <= 0)
		return;

	scene->packetTraverse(corners, packet, 0.001f);
	for (auto& rayInfo : packet) {
		if (rayInfo.rec.t != INF) {
			HitRecord hr = rayInfo.rec;
			Ray reflectedRay;
			MaterialPtr mat = scene->getMaterial(hr.material);
			Color attenuation = scene->getTextureColor(mat->getAlbedoIdx(), hr.u, hr.v, hr.p);
			float reflectance = 1;

			if (mat->getType() == Materials::DIFFUSE) {
				rayInfo.pxColor = attenuation * scene->traceLights(hr);
				continue;
			}
			else if (mat->getType() == Materials::MIRROR) {

				mat->reflect(rayInfo.ray, hr, reflectedRay, reflectance);
				if (reflectance == 1.0f) {
					rayInfo.pxColor = attenuation * (Core::traceWhitted(reflectedRay, bounces - 1, scene, rng));
					continue;
				}
				else {
					rayInfo.pxColor = attenuation * (reflectance * Core::traceWhitted(reflectedRay, bounces - 1, scene, rng) + (1.0f - reflectance) * scene->traceLights(hr));
					continue;
				}
			}
			else if (mat->getType() == Materials::DIELECTRIC) {

				Color refractionColor(0.0f);
				Color reflectionColor(0.0f);
				float reflectance;
				mat->reflect(rayInfo.ray, hr, reflectedRay, reflectance);
				reflectionColor = traceWhitted(reflectedRay, bounces - 1, scene, rng);

				if (reflectance < 1.0f) {
					Ray refractedRay;
					float refractance;
					mat->refract(rayInfo.ray, hr, refractedRay, refractance);
					refractionColor = traceWhitted(refractedRay, bounces - 1, scene, rng);
				}

				mat->absorb(rayInfo.ray, hr, attenuation);

				rayInfo.pxColor = attenuation * (reflectionColor * reflectance + refractionColor * (1 - reflectance));
			}
		}
	}
}
};
