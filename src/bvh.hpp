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
	glm::fvec4 minAABBLeftFirst = {INF, INF, INF, 0};
	glm::fvec4 maxAABBCount = {-INF, -INF, -INF, 0};
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
		bool update(float dt) override;
		const std::vector<HittablePtr>& getHittable() const {
			return hittables;
		};
		void collapseBVH();
		void constructTopLevelBVH();
		void constructSubBVH();

		void setAnimation(const Animation anim){
			animate = true;
			animationManager = anim;
			animationManager.setInitial(transform);
		}

		virtual inline void translate(glm::fvec3 t) override {
			transform.translate(t);
			updateWorldBBox();
		}
		virtual inline void scale(glm::fvec3 s) override {
			transform.scale(s);
			updateWorldBBox();
		}
		virtual inline void scale(float s) override {
			transform.scale(s);
			updateWorldBBox();
		}
		virtual inline void rotate(float t, glm::fvec3 a) override {
			transform.rotate(t, a);
			updateWorldBBox();
		}
		virtual inline AABB getWorldAABB() const override {
			return worldBBox;
		}
		virtual inline void setLocalAABB(AABB local) override {
			Hittable::setLocalAABB(local);
			updateWorldBBox();
		}
		virtual inline void setTransform(const Transform t) override {
			transform = t;
			updateWorldBBox();
		}
		void updateWorldBBox(); 


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
		bool traverseCollapsed(const Ray& ray, const BVHNode* node, float& tMin, float& tMax, HitRecord& rec) const;
		BVHNode* findBestMatch(BVHNode* target, std::list<BVHNode*> nodes);

		std::vector<HittablePtr> hittables;
		std::vector<int> hittableIdxs;

		BVHNode* nodePool;
		BVHNode* root;
		size_t poolPtr;
		float surfaceArea;
		bool isCollapsed = false;

		Heuristic heuristic;
		bool animate;
		bool mustRefit;
		int refitCounter;
		AABB worldBBox;
		Transform transform;
		Animation animationManager;
};

typedef std::shared_ptr<BVH> BVHPtr;

#endif
