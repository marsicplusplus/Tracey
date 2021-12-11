#include "bvh.hpp"

BVH::BVH(std::vector<Hittable> &h) : hittables(h) {
	this->nodePool = new BVHNode[h.size() * 2 - 1];
	this->poolPtr = 0;
}

BVH::~BVH(){
	delete[] nodePool;
}

void BVH::construct(){
	for(int i = 0; hittables.size();++i){
		this->hittableIdxs.push_back(i);
	}
	
	this->root = &this->nodePool[poolPtr++];
	this->root->count = this->hittableIdxs.size();
	computeBounding(root);
	subdivide(root);
}

bool BVH::computeBounding(BVHNode *node) {
	if(node == nullptr) return false;

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
