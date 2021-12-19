#include "bvh.hpp"
#include "defs.hpp"
#include "options_manager.hpp"
#include <chrono>
#include <iostream>
#include <list>

BVH::BVH(std::vector<HittablePtr> h, bool makeTopLevel) : hittables(h) {
	this->nodePool = new BVHNode[h.size() * 2];
	auto t1 = std::chrono::high_resolution_clock::now();
	if (makeTopLevel) {
		constructTopLevelBVH();
	} else {
		constructSubBVH();
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	auto ms_int = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
	std::cout << "BVH Construction for " << h.size() << " hittables: " << ms_int.count() << "us" << std::endl;
}

BVH::~BVH(){
	delete[] this->nodePool;
}

void BVH::constructTopLevelBVH() {
	for (int i = 0; i < hittables.size(); ++i) {
		this->hittableIdxs.push_back(i);
	}

	auto nodeList = std::list<BVHNode*>();
	for (int i = 0; i < hittables.size(); ++i) {
		auto* node = new BVHNode;
		auto hittable = hittables[i];
		auto aabb = hittable->getWorldAABB();
		node->minAABBLeftFirst = { aabb.minX, aabb.minY, aabb.minZ, i };
		node->maxAABBCount = { aabb.maxX, aabb.maxY, aabb.maxZ, 1 };
		nodeList.push_back(node);
	}

	int index = 2 * hittables.size() - 1;

	this->root = &this->nodePool[0];
	this->root->minAABBLeftFirst.w = 0;
	this->root->maxAABBCount.w = this->hittableIdxs.size();
	this->poolPtr = 2;
	computeBounding(root);

	auto nodeA = nodeList.front();
	if (nodeList.size() > 1) {
		auto nodeB = findBestMatch(nodeA, nodeList);
		auto nodeC = findBestMatch(nodeB, nodeList);
		while (nodeList.size() > 1) {
			if (nodeA == nodeC) {

				auto rightIndex = index--;
				auto leftIndex = index--;
				auto A = &this->nodePool[leftIndex];
				auto B = &this->nodePool[rightIndex];

				A->maxAABBCount = nodeA->maxAABBCount;
				A->minAABBLeftFirst = nodeA->minAABBLeftFirst;
				B->maxAABBCount = nodeB->maxAABBCount;
				B->minAABBLeftFirst = nodeB->minAABBLeftFirst;

				nodeList.remove(nodeA);
				nodeList.remove(nodeB);

				nodeA->minAABBLeftFirst.x = min(nodeB->minAABBLeftFirst.x, nodeA->minAABBLeftFirst.x);
				nodeA->minAABBLeftFirst.y = min(nodeB->minAABBLeftFirst.y, nodeA->minAABBLeftFirst.y);
				nodeA->minAABBLeftFirst.z = min(nodeB->minAABBLeftFirst.z, nodeA->minAABBLeftFirst.z);
				nodeA->minAABBLeftFirst.w = leftIndex;
				nodeA->maxAABBCount.x = max(nodeB->maxAABBCount.x, nodeA->maxAABBCount.x);
				nodeA->maxAABBCount.y = max(nodeB->maxAABBCount.y, nodeA->maxAABBCount.y);
				nodeA->maxAABBCount.z = max(nodeB->maxAABBCount.z, nodeA->maxAABBCount.z);
				nodeA->maxAABBCount.w = 0;
				nodeList.push_back(nodeA);

				if (nodeList.size() > 1)
					nodeB = findBestMatch(nodeA, nodeList);
			}
			else {
				nodeA = nodeB;
				nodeB = nodeC;
			}
			nodeC = findBestMatch(nodeB, nodeList);
		}
	}

	this->root = nodeA;
	nodeList.clear();

	setLocalAABB({
		this->root->minAABBLeftFirst.x,
		this->root->minAABBLeftFirst.y,
		this->root->minAABBLeftFirst.z,
		this->root->maxAABBCount.x,
		this->root->maxAABBCount.y,
		this->root->maxAABBCount.z
		});
	this->surfaceArea = calculateSurfaceArea(worldBBox);
}

void BVH::constructSubBVH() {
	for (int i = 0; i < hittables.size(); ++i) {
		this->hittableIdxs.push_back(i);
	}

	this->root = &this->nodePool[0];
	this->root->minAABBLeftFirst.w = 0;
	this->root->maxAABBCount.w = this->hittableIdxs.size();
	this->poolPtr = 2;
	computeBounding(root);
	subdivideBin(root);
	setLocalAABB({
		this->root->minAABBLeftFirst.x,
		this->root->minAABBLeftFirst.y,
		this->root->minAABBLeftFirst.z,
		this->root->maxAABBCount.x,
		this->root->maxAABBCount.y,
		this->root->maxAABBCount.z
	});
	this->surfaceArea = calculateSurfaceArea(worldBBox);
}

bool BVH::computeBounding(BVHNode *node) {
	if(node == nullptr) return false;
	node->minAABBLeftFirst = { INF,INF,INF, node->minAABBLeftFirst.w };
	node->maxAABBCount = { -INF,-INF,-INF , node->maxAABBCount.w };
	int threadNum = OptionsMap::Instance()->getOption(Options::THREADS);
	std::vector<std::future<void>> futures;
	std::vector<AABB> bboxes(threadNum);
	if(node->maxAABBCount.w > 255){
		int nChunks = min((int)node->maxAABBCount.w, threadNum);
		//printf("nChunks: %d\nNode Size: %d\n", nChunks, (int)node->maxAABBCount.w);
		// Each of the N threads works on (t*N)/T triangles
		for(size_t i = 0; i < nChunks; ++i){
			int threadNodeStart = node->minAABBLeftFirst.w + std::ceil((i * node->maxAABBCount.w)/(float)nChunks);
			int threadNodeEnd = node->minAABBLeftFirst.w + std::ceil(((i+1) * node->maxAABBCount.w)/(float)(nChunks));
			//printf("Thread %d\nStart: %d\nEnd: %d\n----------\n", i, threadNodeStart, threadNodeEnd);
			futures.push_back(Threading::pool.queue([&, threadNodeStart, threadNodeEnd, i](uint32_t &rng){
				AABB aabb{INF, INF, INF, -INF, -INF, -INF};
				for(size_t j = threadNodeStart; j < threadNodeEnd; ++j){
					auto hit = hittables[hittableIdxs[j]];
					AABB hitAABB = hit->getWorldAABB();
					aabb.minX = min(aabb.minX, hitAABB.minX);
					aabb.minY = min(aabb.minY, hitAABB.minY);
					aabb.minZ = min(aabb.minZ, hitAABB.minZ);
					aabb.maxX = max(aabb.maxX, hitAABB.maxX);
					aabb.maxY = max(aabb.maxY, hitAABB.maxY);
					aabb.maxZ = max(aabb.maxZ, hitAABB.maxZ);
				}
				bboxes[i] = aabb;
			}));
		}

		for(size_t i = 0; i < futures.size(); ++i){
			futures[i].get();
			AABB aabb = bboxes[i];
			node->minAABBLeftFirst.x = min(aabb.minX, node->minAABBLeftFirst.x);
			node->minAABBLeftFirst.y = min(aabb.minY, node->minAABBLeftFirst.y);
			node->minAABBLeftFirst.z = min(aabb.minZ, node->minAABBLeftFirst.z);
			node->maxAABBCount.x = max(aabb.maxX, node->maxAABBCount.x);
			node->maxAABBCount.y = max(aabb.maxY, node->maxAABBCount.y);
			node->maxAABBCount.z = max(aabb.maxZ, node->maxAABBCount.z);
		}
	} else {
		for(size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i){
			auto hit = hittables[hittableIdxs[i]];
			AABB aabb = hit->getWorldAABB();
			node->minAABBLeftFirst.x = min(aabb.minX, node->minAABBLeftFirst.x);
			node->minAABBLeftFirst.y = min(aabb.minY, node->minAABBLeftFirst.y);
			node->minAABBLeftFirst.z = min(aabb.minZ, node->minAABBLeftFirst.z);
			node->maxAABBCount.x = max(aabb.maxX, node->maxAABBCount.x);
			node->maxAABBCount.y = max(aabb.maxY, node->maxAABBCount.y);
			node->maxAABBCount.z = max(aabb.maxZ, node->maxAABBCount.z);
		}
	}
	return true;
}

void BVH::subdivideBin(BVHNode* node) {
	if (node == nullptr) return;
	if (node->maxAABBCount.w < 3) {
		computeBounding(node);
		return; 
	}

	partitionBin(node);
	// Then subdivide again 
	subdivideBin(&this->nodePool[(int)node->minAABBLeftFirst.w]);
	subdivideBin(&this->nodePool[(int)node->minAABBLeftFirst.w + 1]);
	
	return;
}

void BVH::subdivideHQ(BVHNode* node) {
	if (node == nullptr) return;
	if (node->maxAABBCount.w < 3) {
		computeBounding(node);
		return; // Jacco said this was wrong (or not efficient). Why?
	}

	partitionHQ(node);
	// Then subdivide again 
	subdivideHQ(&this->nodePool[(int)node->minAABBLeftFirst.w]);
	subdivideHQ(&this->nodePool[(int)node->minAABBLeftFirst.w + 1]);

	return;
}

void BVH::partitionBin(BVHNode* node) {

	// For a partition of a node
	// Divide the node into k bins vertically along its longest AABB axis.
	float numOfBins = 16.0f;
	std::vector<Bin> bins(numOfBins);

	// Compute the centroid bounds (the bounds defined by the centroids of all triangles within the node)
	AABB centroidBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
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
	float longestAxisLength = 0.0f;
	int longestAxisIdx = -1;

	float length = centroidBBox.maxX - centroidBBox.minX;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 0;
	}

	length = centroidBBox.maxY - centroidBBox.minY;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 1;
	}

	length = centroidBBox.maxZ - centroidBBox.minZ;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 2;
	}
	if(longestAxisIdx != -1){
		glm::fvec3 minBBox = glm::fvec3(centroidBBox.minX, centroidBBox.minY, centroidBBox.minZ);
		glm::fvec3 maxBBox = glm::fvec3(centroidBBox.maxX, centroidBBox.maxY, centroidBBox.maxZ);

		float k1 = numOfBins * (1.0f - 0.00001f) / (maxBBox[longestAxisIdx] - minBBox[longestAxisIdx]);
		float k0 = minBBox[longestAxisIdx];

		for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
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
		int maxj = node->minAABBLeftFirst.w + node->maxAABBCount.w - 1;
		for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
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
		auto first = node->minAABBLeftFirst.w;
		node->maxAABBCount.w = 0;
		node->minAABBLeftFirst.w = poolPtr;
		auto leftNode = &this->nodePool[poolPtr++];
		auto rightNode = &this->nodePool[poolPtr++];

		// Asign leftFirst and count to our left and right nodes
		leftNode->minAABBLeftFirst.x = optimalLeftBBox.minX;
		leftNode->minAABBLeftFirst.y = optimalLeftBBox.minY;
		leftNode->minAABBLeftFirst.z = optimalLeftBBox.minZ;
		leftNode->minAABBLeftFirst.w = first;

		leftNode->maxAABBCount.x = optimalLeftBBox.maxX;
		leftNode->maxAABBCount.y = optimalLeftBBox.maxY;
		leftNode->maxAABBCount.z = optimalLeftBBox.maxZ;
		leftNode->maxAABBCount.w = optimalLeftCount;


		rightNode->minAABBLeftFirst.x = optimalRightBBox.minX;
		rightNode->minAABBLeftFirst.y = optimalRightBBox.minY;
		rightNode->minAABBLeftFirst.z = optimalRightBBox.minZ;
		rightNode->minAABBLeftFirst.w = first + optimalLeftCount;

		rightNode->maxAABBCount.x = optimalRightBBox.maxX;
		rightNode->maxAABBCount.y = optimalRightBBox.maxY;
		rightNode->maxAABBCount.z = optimalRightBBox.maxZ;
		rightNode->maxAABBCount.w = optimalRightCount;
	}
}


void BVH::partitionHQ(BVHNode* node) {

	// For a high quality partition of a node
	// Find all possible partitions using the centroids across all 3 axes. 
	// Find optimal cost which is our pivot point
	int optimalSplitIdx = -1;
	int optimalLeftCount = 0;
	int optimalRightCount = 0;
	float optimalSplitPos = INF;

	AABB parentNodeAABB = AABB{
		node->minAABBLeftFirst.x,
		node->minAABBLeftFirst.y,
		node->minAABBLeftFirst.z,
		node->maxAABBCount.x,
		node->maxAABBCount.y,
		node->maxAABBCount.z
	};

	float parentCost = calculateSurfaceArea(parentNodeAABB) * node->maxAABBCount.w;
	auto lowestCost = parentCost;

	AABB optimalLeftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	AABB optimalRightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };

	for (int axis = 0; axis < 3; ++axis) {
		for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
			int leftCount = 0;
			int rightCount = 0;

			AABB leftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
			AABB rightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };

			auto prim = hittables[hittableIdxs[i]];
			AABB splitAABB = prim->getWorldAABB();
			float splitCX = (splitAABB.minX + splitAABB.maxX) / 2.0f;
			float splitCY = (splitAABB.minY + splitAABB.maxY) / 2.0f;
			float splitCZ = (splitAABB.minZ + splitAABB.maxZ) / 2.0f;
			auto splitCentroid = glm::fvec3(splitCX, splitCY, splitCZ);
			auto splitPos = splitCentroid[axis];

			for (size_t j = node->minAABBLeftFirst.w; j < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++j) {
				auto prim = hittables[hittableIdxs[j]];
				AABB aabb = prim->getWorldAABB();
				float cx = (aabb.minX + aabb.maxX) / 2.0f;
				float cy = (aabb.minY + aabb.maxY) / 2.0f;
				float cz = (aabb.minZ + aabb.maxZ) / 2.0f;
				auto centroid = glm::fvec3(cx, cy, cz);
				auto pos = centroid[axis];

				if (pos <= splitPos) {
					leftBBox.minX = min(leftBBox.minX, aabb.minX);
					leftBBox.minY = min(leftBBox.minY, aabb.minY);
					leftBBox.minZ = min(leftBBox.minZ, aabb.minZ);
					leftBBox.maxX = max(leftBBox.maxX, aabb.maxX);
					leftBBox.maxY = max(leftBBox.maxY, aabb.maxY);
					leftBBox.maxZ = max(leftBBox.maxZ, aabb.maxZ);
					leftCount++;
				}
				else {
					rightBBox.minX = min(rightBBox.minX, aabb.minX);
					rightBBox.minY = min(rightBBox.minY, aabb.minY);
					rightBBox.minZ = min(rightBBox.minZ, aabb.minZ);
					rightBBox.maxX = max(rightBBox.maxX, aabb.maxX);
					rightBBox.maxY = max(rightBBox.maxY, aabb.maxY);
					rightBBox.maxZ = max(rightBBox.maxZ, aabb.maxZ);
					rightCount++;
				}
			}

			auto splitCost = calculateSurfaceArea(leftBBox) * leftCount + calculateSurfaceArea(rightBBox) * rightCount;
			if (splitCost < lowestCost) {
				lowestCost = splitCost;
				optimalSplitIdx = axis;
				optimalSplitPos = splitPos;
				optimalLeftCount = leftCount;
				optimalRightCount = rightCount;
				optimalLeftBBox = leftBBox;
				optimalRightBBox = rightBBox;
			}
		}
	}

	// Quicksort our hittableIdx 
	int maxj = node->minAABBLeftFirst.w + node->maxAABBCount.w - 1;
	for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
		auto leftPrim = hittables[hittableIdxs[i]];
		AABB leftAABB = leftPrim->getWorldAABB();
		float leftCX = (leftAABB.minX + leftAABB.maxX) / 2.0f;
		float leftCY = (leftAABB.minY + leftAABB.maxY) / 2.0f;
		float leftCZ = (leftAABB.minZ + leftAABB.maxZ) / 2.0f;
		auto leftCentroid = glm::fvec3(leftCX, leftCY, leftCZ);
		auto leftPos = leftCentroid[optimalSplitIdx];

		if (leftPos > optimalSplitPos) {
			for (size_t j = maxj; j > i; --j) {
				auto rightPrim = hittables[hittableIdxs[j]];
				AABB rightAABB = rightPrim->getWorldAABB();
				float rightCX = (rightAABB.minX + rightAABB.maxX) / 2.0f;
				float rightCY = (rightAABB.minY + rightAABB.maxY) / 2.0f;
				float rightCZ = (rightAABB.minZ + rightAABB.maxZ) / 2.0f;
				auto rightCentroid = glm::fvec3(rightCX, rightCY, rightCZ);
				auto rightPos = rightCentroid[optimalSplitIdx];

				if (rightPos <= optimalSplitPos) {
					std::swap(hittableIdxs[i], hittableIdxs[j]);
					maxj = j - 1;
					break;
				}
			}
		}
	}

	// Change this node to be an interior node by setting its count to 0 and setting leftFirst to the poolPtr index
	auto first = node->minAABBLeftFirst.w;
	node->maxAABBCount.w = 0;
	node->minAABBLeftFirst.w = poolPtr;
	auto leftNode = &this->nodePool[poolPtr++];
	auto rightNode = &this->nodePool[poolPtr++];

	// Asign leftFirst and count to our left and right nodes
	leftNode->minAABBLeftFirst.x = optimalLeftBBox.minX;
	leftNode->minAABBLeftFirst.y = optimalLeftBBox.minY;
	leftNode->minAABBLeftFirst.z = optimalLeftBBox.minZ;
	leftNode->minAABBLeftFirst.w = first;

	leftNode->maxAABBCount.x = optimalLeftBBox.maxX;
	leftNode->maxAABBCount.y = optimalLeftBBox.maxY;
	leftNode->maxAABBCount.z = optimalLeftBBox.maxZ;
	leftNode->maxAABBCount.w = optimalLeftCount;


	rightNode->minAABBLeftFirst.x = optimalRightBBox.minX;
	rightNode->minAABBLeftFirst.y = optimalRightBBox.minY;
	rightNode->minAABBLeftFirst.z = optimalRightBBox.minZ;
	rightNode->minAABBLeftFirst.w = first + optimalLeftCount;

	rightNode->maxAABBCount.x = optimalRightBBox.maxX;
	rightNode->maxAABBCount.y = optimalRightBBox.maxY;
	rightNode->maxAABBCount.z = optimalRightBBox.maxZ;
	rightNode->maxAABBCount.w = optimalRightCount;
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

bool BVH::hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
	const Ray transformedRay = ray.transformRay(transformInv);

	HitRecord tmp;
	if (traverse(transformedRay, this->root, tMin, tMax, tmp)) {
		rec = tmp;
		rec.p = transform * glm::fvec4(rec.p, 1.0f);
		rec.setFaceNormal(ray, transposeInv * glm::fvec4(rec.normal, 0.0));
		return true;
	}
	return false;
}

bool BVH::traverse(const Ray& ray, BVHNode* node, float& tMin, float& tMax, HitRecord& rec) const {
	HitRecord tmp;
	bool hasHit = false;
	float closest = tMax;

	if (node->maxAABBCount.w != 0) {

		// We are a leaf
		// Intersect the primitives
		for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
			auto prim = hittables[hittableIdxs[i]];
			if (prim->hit(ray, tMin, closest, tmp)) {
				rec = tmp;
				closest = rec.t;
				hasHit = true;
			}
		}

		tMax = closest;
		return hasHit;
	} else {

		auto firstNode = &this->nodePool[(int)node->minAABBLeftFirst.w];
		auto secondNode = &this->nodePool[(int)node->minAABBLeftFirst.w + 1];

		float firstDistance = 0.0f;
		float secondDistance = 0.0f;

		bool hitAABBFirst = hitAABB(ray, firstNode->minAABBLeftFirst, firstNode->maxAABBCount, firstDistance);
		bool hitAABBSecond = hitAABB(ray, secondNode->minAABBLeftFirst, secondNode->maxAABBCount, secondDistance);

		if (hitAABBFirst && hitAABBSecond) {
			if (firstDistance > secondDistance) {
				auto tempNode = firstNode;
				firstNode = secondNode;
				secondNode = tempNode;

				auto tempDist = firstDistance;
				firstDistance = secondDistance;
				secondDistance = tempDist;
			}

			if (firstDistance < tMax) {
				bool hitFirst = false;
				bool hitSecond = false;
				hitFirst = traverse(ray, firstNode, tMin, tMax, rec);
				if (secondDistance < tMax)
					hitSecond = traverse(ray, secondNode, tMin, tMax, rec);
				return hitFirst || hitSecond;
			}

			return false;
		} else if (hitAABBFirst && firstDistance < tMax) {
			return traverse(ray, firstNode, tMin, tMax, rec);
		} else if (hitAABBSecond && secondDistance < tMax) {
			return traverse(ray, secondNode, tMin, tMax, rec);
		} else {
			return false;
		}
	}
}


BVHNode* BVH::findBestMatch(BVHNode* target, std::list<BVHNode*> nodes) {
	BVHNode* bestMatch;
	float bestSurfaceArea = INF;
	for (auto &node: nodes) {
		if (node != target) {
			AABB aabb = { node->minAABBLeftFirst.x, node->minAABBLeftFirst.y, node->minAABBLeftFirst.z, node->maxAABBCount.x, node->maxAABBCount.y, node->maxAABBCount.z };
			float surfaceArea = calculateSurfaceArea(aabb);
			if (surfaceArea < bestSurfaceArea) {
				bestMatch = node;
				bestSurfaceArea = surfaceArea;
			}
		}
	}

	return bestMatch;
}
