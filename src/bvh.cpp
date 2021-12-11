#include "bvh.hpp"
#include "defs.hpp"

BVH::BVH(std::vector<HittablePtr> &h) : hittables(h) {
	this->nodePool = new BVHNode[h.size() * 2];
}

BVH::~BVH(){
	delete[] nodePool;
}

void BVH::construct(){
	for(int i = 0; hittables.size();++i){
		this->hittableIdxs.push_back(i);
	}
	
	this->root = &this->nodePool[0];
	this->root->leftFirst = 0;
	this->root->count = this->hittableIdxs.size();
	this->poolPtr = 2;
	computeBounding(root);
	subdivide(root);
}

bool BVH::computeBounding(BVHNode *node) {
	if(node == nullptr) return false;
	node->aabb = AABB{INF,INF,INF,-INF,-INF,-INF};
	for(size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i){
		auto prim = hittables[hittableIdxs[i]];
		AABB aabb = prim->getAABB();
		node->aabb.minX = min(aabb.minX, node->aabb.minX);
		node->aabb.minY = min(aabb.minY, node->aabb.minY);
		node->aabb.minZ = min(aabb.minZ, node->aabb.minZ);
		node->aabb.maxX = max(aabb.maxX, node->aabb.maxX);
		node->aabb.maxY = max(aabb.maxY, node->aabb.maxY);
		node->aabb.maxZ = max(aabb.maxZ, node->aabb.maxZ);
	}
	return true;
}

bool BVH::subdivide(BVHNode *node) {
	if(node == nullptr) return false;
	if(node->count < 3) return false; // Jacco said this was wrong (or not efficient). Why?
	// If count == 0, we are not a leaf, hence we need to subdivide;
	// node->leftFirst = poolPtr; // And ofc new node needs to be created at pool[poolPtr];
	// poolPtr += 1;
	// node->right = Same as before;

	// if count != 0 we are a leaf!
	// node->leftFirst= first element of the leaf;
	// node->count = number of hittables in this leaf;
	return true;
}
