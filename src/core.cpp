#include "core.hpp"

namespace Core {
	Color tracePath(Ray &ray, int bounces, ScenePtr scene, uint32_t &rng) {
		HitRecord hr;
		hr.p = {INF, INF, INF};
		if(!scene || bounces <= 0)
			return Color{0,0,0};
		if(scene->traverse(ray, 0.001f, INF, hr)) {
			/* Evaluate light equations if hitted object is not emitting light */
			/* IF it's an emissive surface, stop the recursion */
			MaterialPtr mat = scene->getMaterial(hr.material);
			Color attenuation = scene->getTextureColor(mat->getAlbedoIdx(), hr.u, hr.v, hr.p);
			if(mat->getType() == Materials::EMISSIVE){
				return mat->getIntensity() * attenuation;
			}
			if(mat->getType() == Materials::MIRROR){
				if(Random::RandomFloat(rng) < mat->getReflection()){
					Ray refl;
					mat->reflect(ray, hr, refl);
					return attenuation * tracePath(refl, bounces - 1, scene, rng);
				}
			} else if(mat->getType() == Materials::DIELECTRIC){
				if(Random::RandomFloat(rng) < mat->getFresnel(ray, hr)) {
					Ray refl;
					mat->reflect(ray, hr, refl);
					return attenuation * tracePath(refl, bounces - 1, scene, rng);
				} else {
					Ray refl;
					mat->refract(ray, hr, refl);
					return attenuation * tracePath(refl, bounces - 1, scene, rng);
				}
			} /* Else we are just diffuse */
			auto BRDF = attenuation * INVPI;
			glm::fvec3 newDir = Random::RandomInHemisphere(rng, hr.normal);
			Ray newRay(hr.p + 0.001f * newDir, newDir);
			auto Ei = tracePath(newRay, bounces-1, scene, rng) * glm::dot(hr.normal, newDir);
			return PI * 2.0f * BRDF * Ei;
		}
		return scene->getBgColor(ray);
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
			float reflectance = 1;

			if (mat->getType() == Materials::DIFFUSE || mat->getType() == Materials::EMISSIVE) {
				return attenuation * scene->traceLights(hr);
			} else if (mat->getType() == Materials::MIRROR) {

				reflectance = mat->getReflection();
				mat->reflect(ray, hr, reflectedRay);
				if (reflectance == 1.0f)
					return attenuation * (Core::traceWhitted(reflectedRay, bounces - 1, scene, rng));
				else
					return attenuation * (reflectance * Core::traceWhitted(reflectedRay, bounces - 1, scene, rng) + (1.0f - reflectance) * scene->traceLights(hr));
			} else if(mat->getType() == Materials::DIELECTRIC) {

				Color refractionColor(0.0f);
				Color reflectionColor(0.0f);
				float reflectance = mat->getFresnel(ray, hr);
				mat->reflect(ray, hr, reflectedRay);
				reflectionColor = Core::traceWhitted(reflectedRay, bounces - 1, scene, rng);

				if(reflectance < 1.0f){
					Ray refractedRay;
					float refractance;
					mat->refract(ray, hr, refractedRay);
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
				reflectance = mat->getReflection();
				mat->reflect(rayInfo.ray, hr, reflectedRay);
				if (reflectance == 1.0f) {
					rayInfo.pxColor = attenuation * (traceWhitted(reflectedRay, bounces - 1, scene, rng));
					continue;
				}
				else {
					rayInfo.pxColor = attenuation * (reflectance * traceWhitted(reflectedRay, bounces - 1, scene, rng) + (1.0f - reflectance) * scene->traceLights(hr));
					continue;
				}
			}
			else if (mat->getType() == Materials::DIELECTRIC) {

				Color refractionColor(0.0f);
				Color reflectionColor(0.0f);
				float reflectance = mat->getFresnel(rayInfo.ray, hr);
				mat->reflect(rayInfo.ray, hr, reflectedRay);
				reflectionColor = traceWhitted(reflectedRay, bounces - 1, scene, rng);

				if (reflectance < 1.0f) {
					Ray refractedRay;
					float refractance = mat->getFresnel(rayInfo.ray, hr);
					mat->refract(rayInfo.ray, hr, refractedRay);
					refractionColor = traceWhitted(refractedRay, bounces - 1, scene, rng);
				}

				mat->absorb(rayInfo.ray, hr, attenuation);

				rayInfo.pxColor = attenuation * (reflectionColor * reflectance + refractionColor * (1 - reflectance));
			}
		}
	}
}
};
