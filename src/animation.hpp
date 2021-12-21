#ifndef __ANIMATION_HPP__
#define __ANIMATION_HPP__

#include "transform.hpp"
#include <vector>

class Animation {
	public:
		Animation() {};
		Animation(bool _loop, float start, std::vector<Transform> _frames, std::vector<float> _times);
		
		void update(float dt);

		inline void setInitial(const Transform transform){
			initial = transform;
			current = initial;
		}

		Transform getNextTransform();

		inline bool isStarted() const {
			return started;
		}
		inline bool isEnded() const {
			return stopped;
		}

	private:
		Transform initial;
		Transform current;
		std::vector<Transform> frames;
		std::vector<float> times;
		int currentFrame;
		bool loop;
		bool started;
		bool stopped;
		float accumulator;
		float start;
};

#endif
