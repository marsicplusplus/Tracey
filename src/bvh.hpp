#ifndef __BVH_HPP__
#define __BVH_HPP__

#include "hittables/hittable.hpp"
#include <list>
#include <stack>

struct BVHNode {

	// If this is a leaf, we want to know the first hittable in the array;
// Otherwise we want to know the index of the left child. The right child will be left + 1;

	glm::fvec4 minAABBLeftFirst;
	glm::fvec4 maxAABBCount;
};
/* We could also combine the first 3 floats with the first int and the second set of 3 floats with the second int to use SIMD operations and gain more STONKS */

struct StackNode
{
	BVHNode* node;
	int first;
};

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
		void packetHit(std::vector<RayInfo>& rays, Frustum frustum, float tMin, int first, int last) override;
		void packetTraverse(std::vector<RayInfo>& rays, Frustum frustum, BVHNode* node, float& tMin, int first);
		bool frustumIntersectsAABB(Frustum frustum, const glm::fvec4& minBBox, const glm::fvec4& maxBBox);
		void getFirstHit(std::vector<RayInfo> packet, Frustum F, const glm::fvec4& minBBox, const glm::fvec4& maxBBox, int& first);
		int getLastHit(std::vector<RayInfo> packet, const glm::fvec4& minBBox, const glm::fvec4& maxBBox, int first);
		bool update(float dt) override;
		const std::vector<HittablePtr>& getHittable() const {
			return hittables;
		};

	private:
		void constructTopLevelBVH();
		void constructSubBVH();
		void subdivideBin(BVHNode* node);
		void partitionBinSingle(BVHNode* node);
		void partitionBinMulti(BVHNode* node);

		void subdivideHQ(BVHNode* node);
		void partitionHQ(BVHNode* node);

		bool computeBounding(BVHNode *node);
		float calculateSurfaceArea(AABB bbox);
		float calculateBinID(AABB primAABB, float k1, float k0, int longestAxisIdx);
		bool traverse(const Ray& ray, BVHNode* node, float& tMin, float& tMax, HitRecord& rec) const;
		BVHNode* findBestMatch(BVHNode* target, std::list<BVHNode*> nodes);

		std::vector<HittablePtr> hittables;
		std::vector<int> hittableIdxs;
		
		BVHNode* nodePool;
		BVHNode* root;
		size_t poolPtr;
		float surfaceArea;
		bool animate;
};

typedef std::shared_ptr<BVH> BVHPtr;

#endif
