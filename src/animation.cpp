#include "animation.hpp"
#include <algorithm>


Animation::Animation(bool _loop, float _start, std::vector<Transform> _frames, std::vector<float> _times) : loop(_loop), start(_start), frames(_frames), times(_times){
	assert(frames.size() == times.size());
	currentFrame = -1;
	started = false;
	stopped = false;
}

void Animation::update(float dt){
	this->accumulator += dt;
	if(!started && accumulator >= start){
		this->accumulator = 0;
		started = true;
	}
}

Transform Animation::getNextTransform(){
	Transform *prev;
	Transform *next;
	int nextIdx = currentFrame+1;
	if(currentFrame == -1){
		prev = &initial;
		next = &frames[0];
	} else {
		prev = &frames[currentFrame];
		next = &frames[nextIdx];
	}

	current = Transform::lerp(prev, next, std::clamp(accumulator/(float)times[nextIdx], 0.0f, 1.0f));

	if(accumulator >= times[nextIdx]){
		accumulator = 0;
		currentFrame += 1;
		if(currentFrame >= frames.size()){
			if(loop){
				current = initial;
				currentFrame = -1;
			} else {
				stopped = true;
				current = frames[currentFrame-1];
			}
		} 
	}
	return current;
}
