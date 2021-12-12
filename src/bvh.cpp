#include "bvh.hpp"
#include "defs.hpp"

BVH::BVH(std::vector<HittablePtr> &h) : hittables(h) {
	this->nodePool = new BVHNode[h.size() * 2];
	construct();
}

BVH::~BVH(){
	delete[] this->nodePool;
}

void BVH::construct(){
	for(int i = 0; i < hittables.size();++i){
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
		AABB aabb = prim->getWorldAABB();
		node->aabb.minX = min(aabb.minX, node->aabb.minX);
		node->aabb.minY = min(aabb.minY, node->aabb.minY);
		node->aabb.minZ = min(aabb.minZ, node->aabb.minZ);
		node->aabb.maxX = max(aabb.maxX, node->aabb.maxX);
		node->aabb.maxY = max(aabb.maxY, node->aabb.maxY);
		node->aabb.maxZ = max(aabb.maxZ, node->aabb.maxZ);
	}
	return true;
}

void BVH::subdivide(BVHNode* node) {
	if (node == nullptr) return;
	if (node->count < 3) {
		computeBounding(node);
		return; // Jacco said this was wrong (or not efficient). Why?
	}

	partition(node);
	// Then subdivide again 
	subdivide(&this->nodePool[node->leftFirst]);
	subdivide(&this->nodePool[node->leftFirst+1]);
	
	return;
}

void BVH::partition(BVHNode* node) {

	// For a partition of a node
	// Divide the node into k bins vertically along its longest AABB axis.
	float numOfBins = 16.0f;
	std::vector<Bin> bins(numOfBins);

	float longestAxisLength = 0.0f;
	int longestAxisIdx = -1;

	float length = node->aabb.maxX - node->aabb.minX;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 0;
	}

	length = node->aabb.maxY - node->aabb.minY;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 1;
	}

	length = node->aabb.maxZ - node->aabb.minZ;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 2;
	}

	// Compute the centroid bounds (the bounds defined by the centroids of all triangles within the node)
	AABB centroidBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
		auto prim = hittables[hittableIdxs[i]];
		AABB aabb = prim->getWorldAABB();
		float cx = (aabb.minX + aabb.maxX) / 2.0f;
		float cy = (aabb.minY + aabb.maxY) / 2.0f;
		float cz = (aabb.minZ + aabb.maxZ) / 2.0f;

		centroidBBox.minX = min(cx, centroidBBox.minX);
		centroidBBox.minY = min(cy, centroidBBox.minY);
		centroidBBox.minZ = min(cz, centroidBBox.minZ);
		centroidBBox.maxX = max(cx, centroidBBox.maxX);
		centroidBBox.maxY = max(cy, centroidBBox.maxY);
		centroidBBox.maxZ = max(cz, centroidBBox.maxZ);
	}

	// For each triangle located within our node, we assign a bin ID (using centroid of triangle and centroid bounds)
	if (centroidBBox.minX == centroidBBox.maxX) {
		centroidBBox.maxX += FLT_EPSILON;
	}

	if (centroidBBox.minY == centroidBBox.maxY) {
		centroidBBox.maxY += FLT_EPSILON;
	}

	if (centroidBBox.minZ == centroidBBox.maxZ) {
		centroidBBox.maxZ += FLT_EPSILON;
	}

	glm::fvec3 minBBox = glm::fvec3(centroidBBox.minX, centroidBBox.minY, centroidBBox.minZ);
	glm::fvec3 maxBBox = glm::fvec3(centroidBBox.maxX, centroidBBox.maxY, centroidBBox.maxZ);

	float k1 = numOfBins * (1.0f - FLT_EPSILON) / (maxBBox[longestAxisIdx] - minBBox[longestAxisIdx]);
	float k0 = minBBox[longestAxisIdx];

	for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
		auto prim = hittables[hittableIdxs[i]];
		auto primAABB = prim->getWorldAABB();
		int binID = calculateBinID(primAABB, k1, k0, longestAxisIdx);

		// For each bin we keep track of the number of triangles as well as the bins bounds
		bins[binID].count += 1;

		auto binAABB = bins[binID].aabb;
		binAABB.minX = min(binAABB.minX, primAABB.minX);
		binAABB.minY = min(binAABB.minY, primAABB.minY);
		binAABB.minZ = min(binAABB.minZ, primAABB.minZ);
		binAABB.maxX = max(binAABB.maxX, primAABB.maxX);
		binAABB.maxY = max(binAABB.maxY, primAABB.maxY);
		binAABB.maxZ = max(binAABB.maxZ, primAABB.maxZ);
		bins[binID].aabb = binAABB;
	}


	// Once all triangles have been assigned a bin ID, we compute the cost of the individual partitions
	// Find optimal cost which is our pivot point
	int optimalSplitIdx = -1;
	auto lowestCost = INF;
	int optimalLeftCount = 0;
	int optimalRightCount = 0;
	AABB optimalLeftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	AABB optimalRightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };

	for (int split = 1; split < bins.size(); ++split) {
		int leftCount = 0;
		int rightCount = 0;

		AABB leftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
		AABB rightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };

		for (int i = 0; i < split; ++i) {

			auto bin = bins[i];
			leftBBox.minX = min(leftBBox.minX, bin.aabb.minX);
			leftBBox.minY = min(leftBBox.minY, bin.aabb.minY);
			leftBBox.minZ = min(leftBBox.minZ, bin.aabb.minZ);
			leftBBox.maxX = max(leftBBox.maxX, bin.aabb.maxX);
			leftBBox.maxY = max(leftBBox.maxY, bin.aabb.maxY);
			leftBBox.maxZ = max(leftBBox.maxZ, bin.aabb.maxZ);
			leftCount += bin.count;
		}

		for (int j = split; j < bins.size(); ++j) {

			auto bin = bins[j];
			rightBBox.minX = min(rightBBox.minX, bin.aabb.minX);
			rightBBox.minY = min(rightBBox.minY, bin.aabb.minY);
			rightBBox.minZ = min(rightBBox.minZ, bin.aabb.minZ);
			rightBBox.maxX = max(rightBBox.maxX, bin.aabb.maxX);
			rightBBox.maxY = max(rightBBox.maxY, bin.aabb.maxY);
			rightBBox.maxZ = max(rightBBox.maxZ, bin.aabb.maxZ);
			rightCount += bin.count;
		}

		auto splitCost = calculateSurfaceArea(leftBBox) * leftCount + calculateSurfaceArea(rightBBox) * rightCount;
		if (splitCost < lowestCost) {
			lowestCost = splitCost;
			optimalSplitIdx = split;
			optimalLeftCount = leftCount;
			optimalRightCount = rightCount;
			optimalLeftBBox = leftBBox;
			optimalRightBBox = rightBBox;
		}
	}

	// Quicksort our hittableIdx 
	int maxj = node->leftFirst + node->count - 1;
	for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
		auto leftPrim = hittables[hittableIdxs[i]];
		int leftBinID = calculateBinID(leftPrim->getWorldAABB(), k1, k0, longestAxisIdx);

		if (leftBinID >= optimalSplitIdx) {
			for (size_t j = maxj; j > i; --j) {
				auto rightPrim = hittables[hittableIdxs[j]];
				int rightBinID = calculateBinID(rightPrim->getWorldAABB(), k1, k0, longestAxisIdx);

				if (rightBinID < optimalSplitIdx) {
					std::swap(hittableIdxs[i], hittableIdxs[j]);
					maxj = j - 1;
					break;
				}
			}
		}
	}

	// Change this node to be an interior node by setting its count to 0 and setting leftFirst to the poolPtr index
	auto first = node->leftFirst;
	node->count = 0;
	node->leftFirst = poolPtr;
	auto leftNode = &this->nodePool[poolPtr++];
	auto rightNode = &this->nodePool[poolPtr++];

	// Asign leftFirst and count to our left and right nodes
	leftNode->count = optimalLeftCount;
	leftNode->leftFirst = first;
	leftNode->aabb = optimalLeftBBox;

	rightNode->leftFirst = first + optimalLeftCount;
	rightNode->count = optimalRightCount;
	rightNode->aabb = optimalRightBBox;
}

float BVH::calculateSurfaceArea(AABB bbox) {

	auto length = bbox.maxX - bbox.minX;
	auto height = bbox.maxY - bbox.minY;
	auto width = bbox.maxZ - bbox.minZ;

	return length * width * height;
}

float BVH::calculateBinID(AABB primAABB, float k1, float k0, int longestAxisIdx) {
	glm::fvec3 centroid = glm::fvec3((primAABB.minX + primAABB.maxX) / 2.0f, (primAABB.minY + primAABB.maxY) / 2.0f, (primAABB.minZ + primAABB.maxZ) / 2.0f);
	return k1 * (centroid[longestAxisIdx] - k0);
}

bool BVH::traverse(const Ray& ray, float tMin, float tMax, HitRecord& rec) {
	return traverseInternal(ray, this->root, tMin, tMax, rec);
}

bool BVH::traverseInternal(const Ray& ray, BVHNode* node, float& tMin, float& tMax, HitRecord& rec) {
	HitRecord tmp;
	bool hasHit = false;
	float closest = tMax;

	if (!hitAABB(ray, node->aabb)) return false;

	if (node->count != 0) {
		// We are a leaf
		// Intersect the primitives
		for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
			auto prim = hittables[hittableIdxs[i]];
			if (prim->hit(ray, tMin, closest, tmp)) {
				rec = tmp;
				closest = tmp.t;
				hasHit = true;
			}
		}

		tMax = closest;
		return hasHit;
	} else {
		bool hitLeft = traverseInternal(ray, &this->nodePool[node->leftFirst], tMin, tMax, rec);
		bool hitRight = traverseInternal(ray, &this->nodePool[node->leftFirst + 1], tMin, tMax, rec);
		return hitLeft || hitRight;
	}
}