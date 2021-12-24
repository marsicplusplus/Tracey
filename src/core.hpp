#ifndef __CORE_HPP__
#define __CORE_HPP__

#include "defs.hpp"
#include "scene.hpp"

namespace Core {
	Color traceWhitted(Ray &ray, int bounces, ScenePtr scene);
	Color tracePath(Ray &ray, int bounces, ScenePtr scene);
	void packetTrace(std::vector<Ray> &corners, std::vector<RayInfo>& packet, int bounces, const ScenePtr scene);
};

#endif
