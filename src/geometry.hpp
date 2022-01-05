#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include "hittables/hittable.hpp"
#include <memory>

class Geometry {
	public:
		Geometry(const std::shared_ptr<Hittable> &shape, int material);
		
	private:	
		int materialIdx;
		std::shared_ptr<Hittable> shape;
};

#endif
