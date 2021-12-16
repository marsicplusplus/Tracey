#ifndef __RAY_HPP__
#define __RAY_HPP__

#include "glm/vec3.hpp"
#include "glm/gtx/norm.hpp"

class Ray{
	public:
		Ray() {};
		Ray(glm::fvec3 _origin, glm::fvec3 _direction, float idx = 1.0f) : origin{_origin}, currentRefraction{idx} {
			if (_direction != glm::fvec3(0, 0, 0)) {
				_direction = glm::normalize(_direction);
			}

			direction = _direction;
			directionInv = 1.0f / direction;
		};

		inline glm::fvec3 getOrigin() const {return origin; }
		inline glm::fvec3 getDirection() const { return direction; }
		inline glm::fvec3 getInverseDirection() const {return directionInv; }
		inline glm::fvec3 at(float t) const {
			return this->origin + t * this->direction;
		};
		inline float getCurrentRefraction() const {
			return this->currentRefraction;
		}
		inline void setCurrentRefraction(const float idx) {
			this->currentRefraction = idx;
		}

		inline Ray transformRay(glm::fmat4x4 transform) const {
			auto newDir = transform * glm::fvec4(direction, 0);
			auto newOg = transform * glm::fvec4(origin, 1);
			Ray ret;
			ret.origin = newOg;
			ret.direction = newDir;
			ret.directionInv = 1.0f/ret.direction;
			return ret;
		}


	private:
		glm::fvec3 origin;
		glm::fvec3 direction;
		glm::fvec3 directionInv;
		float currentRefraction;
};

#endif
