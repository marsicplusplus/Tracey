#ifndef __RAY_HPP__
#define __RAY_HPP__

#include "glm/vec3.hpp"
#include "glm/gtx/norm.hpp"

class Ray{
	public:
		Ray() {};
		Ray(glm::dvec3 _origin, glm::dvec3 _direction, double idx = 1.0) : origin{_origin}, currentRefraction{idx} {
			if (_direction != glm::dvec3(0, 0, 0)) {
				_direction = glm::normalize(_direction);
			}

			direction = _direction;
		};

		inline glm::dvec3 getOrigin() const {return origin; }
		inline glm::dvec3 getDirection() const {return direction; }
		inline glm::dvec3 at(double t) const {
			return this->origin + t * this->direction;
		};
		inline double getCurrentRefraction() const {
			return this->currentRefraction;
		}
		inline void setCurrentRefraction(const double idx) {
			this->currentRefraction = idx;
		}

		inline Ray transformRay(glm::dmat4x4 transform) const {
			return Ray(transform * glm::dvec4(origin, 1), transform * glm::dvec4(direction, 0));
		}


	private:
		glm::dvec3 origin;
		glm::dvec3 direction;
		double currentRefraction;
};

#endif
