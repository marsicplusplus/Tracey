#ifndef __CORE_HPP__
#define __CORE_HPP__

#include "defs.hpp"
#include "scene.hpp"

enum class CoreType {
	WHITTED,
	PATH_TRACER,
};

namespace Core {
	Color traceWhitted(Ray &ray, int bounces, ScenePtr scene, uint32_t &rng);
	Color tracePath(Ray &ray, int bounces, ScenePtr scene, uint32_t &rng);
};

#endif
