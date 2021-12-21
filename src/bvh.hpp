#ifndef __BVH_HPP__
#define __BVH_HPP__

#include "hittables/hittable.hpp"
#include <list>
#include <array>
#include <deque>
struct BVHNode {

	// If this is a leaf, we want to know the first hittable in the array;
// Otherwise we want to know the index of the left child. The right child will be left + 1;

	glm::fvec4 minAABBLeftFirst;
	glm::fvec4 maxAABBCount;
};
/* We could also combine the first 3 floats with the first int and the second set of 3 floats with the second int to use SIMD operations and gain more STONKS */


struct Bin {
	AABB aabb = AABB{ INF,INF,INF,-INF,-INF,-INF };
	int count = 0;
};

struct Split {
	int idx;
	int leftCount;
	int rightCount;
};

struct BinningJob{
	std::vector<Bin> bins;
	std::vector<int> nLeft;
	std::vector<int> nRight;
};

class BVH : public Hittable {
	public:
		BVH(std::vector<HittablePtr> h, bool makeTopLevel = false, bool anim = false);
		~BVH();

		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;
		bool update(float dt) override;
		const std::vector<HittablePtr>& getHittable() const {
			return hittables;
		};

	private:
		void constructTopLevelBVH();
		void constructSubBVH();
		void subdivideBinVert(BVHNode* node, size_t& poolPtr);
		void subdivideBinHoriz(BVHNode* node);
		void subdivideBin(BVHNode* node);
		void partitionBinSingle(BVHNode* node, size_t& poolPtr);
		void partitionBinMulti(BVHNode* node);

		void subdivideHQ(BVHNode* node);
		void partitionHQ(BVHNode* node);

		bool computeBounding(BVHNode *node);
		float calculateSurfaceArea(AABB bbox);
		float calculateBinID(glm::fvec3 centroid, float k1, float k0, int longestAxisIdx);
		bool traverse(const Ray& ray, const BVHNode* node, float& tMin, float& tMax, HitRecord& rec) const;
		BVHNode* findBestMatch(BVHNode* target, std::list<BVHNode*> nodes);

		std::vector<HittablePtr> hittables;
		std::vector<int> hittableIdxs;
		std::array<Bin, 16> bins;
		std::array<int, 16> cumulativeLeftElemCount;
		std::array<float, 16> cumulativeLeftSurfaceArea;
		std::deque<std::pair<BVHNode*, int>> treeTraversal;

		float numOfBins = 16.0f;
		float numSplits = 15.0f;
		BVHNode* nodePool;
		BVHNode* root;
		size_t globalPoolPtr;
		float surfaceArea;
		bool animate;

};

typedef std::shared_ptr<BVH> BVHPtr;

#endif
