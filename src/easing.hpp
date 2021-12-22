#ifndef __EASING_HPP__
#define __EASING_HPP__
#include <iostream>

#include "defs.hpp"

enum class EasingType {
	LINEAR,
	EASIN_SIN,
	EASIN_CUBIC,
	EASIN_EXPO,
	EASIN_ELASTIC,
};

class Easing {
	public:
		static inline float ease(EasingType t, float x) {
			switch(t){
				case EasingType::EASIN_SIN:
					return easinSin(x);
					break;
				case EasingType::EASIN_CUBIC:
					return easinCubic(x);
					break;
				case EasingType::EASIN_EXPO:
					return easinExpo(x);
					break;
				case EasingType::EASIN_ELASTIC:
					return easinElastic(x);
					break;
				case EasingType::LINEAR:
				default:
					return linear(x);
			}
		}

	private:
		static inline float linear(float t) {
			return t;
		}

		static inline float easinSin(float t) {
			return 1.0f - cos((t * PI) / 2.0f);
		}

		static inline float easinCubic(float t) {
			return t*t*t;
		}

		static inline float easinElastic(float t) {
			const float c4 = (2.0f * PI) / 3.0f;
			return t == 0.0f
				? 0.0f
				: t == 1.0f
				? 1.0f
				: -pow(2.0f, 10.0f * t - 10.0f) * sin((t * 10.0f - 10.75f) * c4);
		}

		static inline float easinExpo(float t) {
			return t == 0.0f ? 0.0f : pow(2.0f, 10.0f * t - 10.0f);
		}
};

#endif
