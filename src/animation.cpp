#include "animation.hpp"
#include <algorithm>
#include <utility>

Animation::Animation(bool _loop, float _start, std::vector<Transform> _frames, std::vector<float> _times, std::vector<EasingType> _easings) : loop(_loop), start(_start), frames(std::move(_frames)), times(std::move(_times)), easings(std::move(_easings)){
	assert(frames.size() == times.size());
	assert((frames.size()) == easings.size()); // One extra easing method to go from the last frame back to the first;
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
	} else {
		prev = &frames[currentFrame];
	}
	next = &frames[nextIdx];
	EasingType nextEase = easings[nextIdx];

	current = Transform::lerp(prev, next, Easing::ease(nextEase, (accumulator/(float)times[nextIdx])));

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
