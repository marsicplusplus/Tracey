#ifndef __RAY_HPP__
#define __RAY_HPP__

#include "glm/vec3.hpp"
#include "glm/gtx/norm.hpp"

class Ray{
	public:
		Ray() {};
		Ray(glm::dvec3 _origin, glm::dvec3 _direction) : origin{_origin}, direction{glm::normalize(_direction)} {};
		inline glm::dvec3 getOrigin() const {return origin; }
		inline glm::dvec3 getDirection() const {return direction; }
		inline glm::dvec3 at(double t) const {
			return this->origin + t * this->direction;
		};

		double u,v;
	private:
		glm::dvec3 origin;
		glm::dvec3 direction;
};

#endif
