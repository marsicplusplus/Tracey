#ifndef __EASING_HPP__
#define __EASING_HPP__
#include <iostream>

#include "defs.hpp"

enum class EasingType {
	LINEAR,
	EASIN_SIN,
	EASOUT_SIN,
	EASIN_CUBIC,
	EASOUT_CUBIC,
	EASIN_EXPO,
	EASOUT_EXPO,
	EASIN_ELASTIC,
	EASOUT_ELASTIC,
	EASIN_BACK,
	EASOUT_BACK,
};

class Easing {
	public:
		static inline float ease(EasingType t, float x) {
			switch(t){
				case EasingType::EASIN_SIN:
					return easeInSin(x);
					break;
				case EasingType::EASOUT_SIN:
					return easeOutSin(x);
					break;

				case EasingType::EASIN_BACK:
					return easeInBack(x);
					break;
				case EasingType::EASOUT_BACK:
					return easeOutBack(x);
					break;

				case EasingType::EASIN_CUBIC:
					return easeInCubic(x);
					break;
				case EasingType::EASOUT_CUBIC:
					return easeOutCubic(x);
					break;

				case EasingType::EASIN_EXPO:
					return easeInExpo(x);
					break;
				case EasingType::EASOUT_EXPO:
					return easeOutExpo(x);
					break;

				case EasingType::EASIN_ELASTIC:
					return easeInElastic(x);
					break;
				case EasingType::EASOUT_ELASTIC:
					return easeOutElastic(x);
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

		static inline float easeInSin(float t) {
			return 1.0f - cos((t * PI) / 2.0f);
		}

		static inline float easeInCubic(float t) {
			return t*t*t;
		}

		static inline float easeInElastic(float t) {
			const float c4 = (2.0f * PI) / 3.0f;
			return t == 0.0f
				? 0.0f
				: t == 1.0f
				? 1.0f
				: -pow(2.0f, 10.0f * t - 10.0f) * sin((t * 10.0f - 10.75f) * c4);
		}
		static inline float easeInExpo(float t) {
			return t == 0.0f ? 0.0f : pow(2.0f, 10.0f * t - 10.0f);
		}
		static inline float easeInBack(float t){
			const float c1 = 1.70158f;
			const float c3 = c1 + 1.0f;
			return c3 * t * t * t - c1 * t * t;
		}
		static inline float easeOutSin(float t) {
  			return sin((t * PI) / 2.0f);
		}
		static inline float easeOutCubic(float t) {
			return 1.0f - pow(1.0f - t, 3.0f);
		}
		static inline float easeOutElastic(float t) {
			const float c4 = (2.0f * PI) / 3.0f;
			return t == 0.0f
				? 0.0f
				: t == 1
				? 1
				: pow(2.0f, -10.0f * t) * sin((t * 10.0f - 0.75f) * c4) + 1.0f;
		}
		static inline float easeOutExpo(float t) {
			return t == 1.0f ? 1.0f : 1.0f - pow(2.0f, -10.0f * t);
		}
		static inline float easeOutBack(float t) {
			const float c1 = 1.70158f;
			const float c3 = c1 + 1.0f;
			return 1.0f + c3 * pow(t - 1.0f, 3.0f) + c1 * pow(t - 1.0f, 2.0f);
		}
};

#endif
