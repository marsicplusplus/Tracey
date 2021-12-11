#ifndef __BVH_HPP__
#define __BVH_HPP__

#include "hittables/hittable.hpp"

struct BVHNode {
	AABB aabb;
	// If this is a leaf, we want to know the first hittable in the array;
	// Otherwise we want to know the index of the left child. The right child will be left + 1;
	int leftFirst;
	int count;
};
/* We could also combine the first 3 floats with the first int and the second set of 3 floats with the second int to use SIMD operations and gain more STONKS */

class BVH {
	public:
		BVH(std::vector<Hittable> &h);
		~BVH();

		void construct();

	private:
		bool subdivide(BVHNode *node);
		bool computeBounding(BVHNode *node);

		std::vector<Hittable> &hittables;
		std::vector<int> hittableIdxs;

		BVHNode *nodePool;
		BVHNode *root;
		size_t poolPtr;
};


#endif
