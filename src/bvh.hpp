#ifndef __BVH_HPP__
#define __BVH_HPP__

#include "animation.hpp"
#include "hittables/hittable.hpp"
#include <list>

struct BVHNode {
	glm::fvec4 minAABBLeftFirst;
	glm::fvec4 maxAABBCount;
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
		BVH(std::vector<HittablePtr> h, bool makeTopLevel = false);
		~BVH();

		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;
		bool update(float dt) override;
		const std::vector<HittablePtr>& getHittable() const {
			return hittables;
		};

		void constructTopLevelBVH();
		void setAnimation(const Animation anim){
			animate = true;
			animationManager = anim;
			animationManager.setInitial(transform);
		}

	private:
		void constructSubBVH();
		void subdivideBin(BVHNode* node);
		void partitionBinSingle(BVHNode* node);
		void partitionBinMulti(BVHNode* node);

		void subdivideHQ(BVHNode* node);
		void partitionHQ(BVHNode* node);

		bool updateBVH(float dt, BVHNode* node);

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
		Animation animationManager;
};

typedef std::shared_ptr<BVH> BVHPtr;

#endif
