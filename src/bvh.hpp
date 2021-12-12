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


struct Bin {
	AABB aabb = AABB{ INF,INF,INF,-INF,-INF,-INF };
	int count = 0;
};

class BVH {
	public:
		BVH(std::vector<HittablePtr> &h);
		~BVH();

		bool traverse(const Ray& ray, float tMin, float tMax, HitRecord& rec);

	private:
		void construct();
		void subdivide(BVHNode *node);
		void partition(BVHNode* node);
		bool computeBounding(BVHNode *node);
		float calculateSurfaceArea(AABB bbox);
		float calculateBinID(AABB primAABB, float k1, float k0, int longestAxisIdx);
		bool traverseInternal(const Ray& ray, BVHNode* node, float& tMin, float& tMax, HitRecord& rec);

		std::vector<HittablePtr> &hittables;
		std::vector<int> hittableIdxs;
		
		BVHNode* nodePool;
		BVHNode *root;
		size_t poolPtr;
};


#endif
