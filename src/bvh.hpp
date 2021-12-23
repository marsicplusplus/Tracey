#ifndef __BVH_HPP__
#define __BVH_HPP__

#include "animation.hpp"
#include "hittables/hittable.hpp"
#include <list>

enum class Heuristic {
	SAH,
	MIDPOINT,
};

struct BVHNode {
	glm::fvec4 minAABBLeftFirst;
	glm::fvec4 maxAABBCount;
};

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
		BVH(std::vector<HittablePtr> h, Heuristic heur = Heuristic::SAH, bool _refit = false, bool makeTopLevel = false);
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

		void constructTopLevelBVH();
		void constructSubBVH();

		void setAnimation(const Animation anim){
			animate = true;
			animationManager = anim;
			animationManager.setInitial(transform);
		}

	private:
		void midpointSplit(BVHNode* node);
		void subdivideBin(BVHNode* node);
		void partitionBinSingle(BVHNode* node);
		void partitionBinMulti(BVHNode* node);
		void refit();
		void refitNode(BVHNode* node);
		bool updateNode(BVHNode* node, float dt);

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

		Heuristic heuristic;
		bool animate;
		bool mustRefit;
		int refitCounter;
		Animation animationManager;
};

typedef std::shared_ptr<BVH> BVHPtr;

#endif
