#include "bvh.hpp"
#include "defs.hpp"
#include "options_manager.hpp"
#include <chrono>
#include "GLFW/glfw3.h"
#include <iostream>
#include <list>
#include <stack>

BVH::BVH(std::vector<HittablePtr> h, Heuristic heur, bool _refit, bool makeTopLevel) : hittables(h), heuristic(heur), animate(false), mustRefit(_refit) {
	this->refitCounter = 0;
	auto t1 = std::chrono::high_resolution_clock::now();
	this->nodePool = nullptr;
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
	delete[] this->nodePool;
	this->size = hittables.size() * 2;
	this->nodePool = new BVHNode[size];
	this->hittableIdxs.clear();
	for (int i = 0; i < hittables.size(); ++i) {
		this->hittableIdxs.push_back(i);
	}

	auto nodeList = std::list<BVHNode*>();
	for (int i = 0; i < hittables.size(); ++i) {
		auto* node = new BVHNode;
		auto hittable = hittables[i];
		auto aabb = hittable->getWorldAABB();
		node->minAABB = { aabb.minX, aabb.minY, aabb.minZ};
		node->leftFirst = i;
		node->maxAABB = { aabb.maxX, aabb.maxY, aabb.maxZ};
		node->count = 1;
		nodeList.push_back(node);
	}

	int index = 2 * hittables.size() - 1;

	this->root = &this->nodePool[0];
	this->root->leftFirst = 0;
	this->root->count = this->hittableIdxs.size();
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

				A->maxAABB = nodeA->maxAABB;
				A->count = nodeA->count;
				A->minAABB = nodeA->minAABB;
				A->leftFirst = nodeA->leftFirst;
				B->maxAABB = nodeB->maxAABB;
				B->count = nodeB->count;
				B->minAABB = nodeB->minAABB;
				B->leftFirst = nodeB->leftFirst;

				nodeList.remove(nodeA);
				nodeList.remove(nodeB);

				nodeA->minAABB = glm::min(nodeB->minAABB, nodeA->minAABB);
				nodeA->leftFirst = leftIndex;
				nodeA->maxAABB = glm::max(nodeB->maxAABB, nodeA->maxAABB);
				nodeA->count = 0;
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
		this->root->minAABB.x,
		this->root->minAABB.y,
		this->root->minAABB.z,
		this->root->maxAABB.x,
		this->root->maxAABB.y,
		this->root->maxAABB.z
		});
	this->surfaceArea = calculateSurfaceArea(worldBBox);
}

void BVH::constructSubBVH() {
	if(this->nodePool != nullptr)
		delete[] this->nodePool;
	this->size = hittables.size() * 2;
	this->nodePool = new BVHNode[size];
	this->hittableIdxs.clear();
	for (int i = 0; i < hittables.size(); ++i) {
		this->hittableIdxs.push_back(i);
	}

	this->root = &this->nodePool[0];
	this->root->leftFirst = 0;
	this->root->count = this->hittableIdxs.size();
	this->poolPtr = 2;
	computeBounding(root);
	subdivideBin(root);
	setLocalAABB({
		this->root->minAABB.x,
		this->root->minAABB.y,
		this->root->minAABB.z,
		this->root->maxAABB.x,
		this->root->maxAABB.y,
		this->root->maxAABB.z
	});
	this->surfaceArea = calculateSurfaceArea(worldBBox);
}

bool BVH::computeBounding(BVHNode *node) {
	if(node == nullptr) return false;
	node->minAABB = { INF,INF,INF };
	node->leftFirst = node->leftFirst;
	node->maxAABB = { -INF,-INF,-INF };
	node->count = { node->count };
	int threadNum = OptionsMap::Instance()->getOption(Options::THREADS);
	std::vector<std::future<void>> futures;
	std::vector<AABB> bboxes(threadNum);
	if(node->count > 20000){
		int nChunks = min((int)node->count, threadNum);
		//printf("nChunks: %d\nNode Size: %d\n", nChunks, (int)node->maxAABBCount.w);
		// Each of the N threads works on (t*N)/T triangles
		for(size_t i = 0; i < nChunks; ++i){
			int threadNodeStart = node->leftFirst + std::ceil((i * node->count)/(float)nChunks);
			int threadNodeEnd = node->leftFirst + std::ceil(((i+1) * node->count)/(float)(nChunks));
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
			node->minAABB.x = min(aabb.minX, node->minAABB.x);
			node->minAABB.y = min(aabb.minY, node->minAABB.y);
			node->minAABB.z = min(aabb.minZ, node->minAABB.z);
			node->maxAABB.x = max(aabb.maxX, node->maxAABB.x);
			node->maxAABB.y = max(aabb.maxY, node->maxAABB.y);
			node->maxAABB.z = max(aabb.maxZ, node->maxAABB.z);
		}
	} else {
		for(size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i){
			auto hit = hittables[hittableIdxs[i]];
			AABB aabb = hit->getWorldAABB();
			node->minAABB.x = min(aabb.minX, node->minAABB.x);
			node->minAABB.y = min(aabb.minY, node->minAABB.y);
			node->minAABB.z = min(aabb.minZ, node->minAABB.z);
			node->maxAABB.x = max(aabb.maxX, node->maxAABB.x);
			node->maxAABB.y = max(aabb.maxY, node->maxAABB.y);
			node->maxAABB.z = max(aabb.maxZ, node->maxAABB.z);
		}
	}
	return true;
}

void BVH::subdivideBin(BVHNode* node) {
	if (node == nullptr) return;
	if (node->count < 3) {
		computeBounding(node);
		return; 
	}
	if(this->heuristic == Heuristic::SAH){
		if (node->count > 20000)
			partitionBinMulti(node);
		else
			partitionBinSingle(node);
	} else if (this->heuristic == Heuristic::MIDPOINT){ 
		midpointSplit(node);
	}

	// Then subdivide again 
	subdivideBin(&this->nodePool[(int)node->leftFirst]);
	subdivideBin(&this->nodePool[(int)node->leftFirst + 1]);
	
	return;
}

void BVH::midpointSplit(BVHNode* node){
	// Compute the centroid bounds (the bounds defined by the centroids of all triangles within the node)
	AABB globalCentroidAABB = AABB{ INF,INF,INF,-INF,-INF,-INF };
	for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
		auto prim = hittables[hittableIdxs[i]];
		AABB aabb = prim->getWorldAABB();
		float cx = (aabb.minX + aabb.maxX) / 2.0f;
		float cy = (aabb.minY + aabb.maxY) / 2.0f;
		float cz = (aabb.minZ + aabb.maxZ) / 2.0f;

		globalCentroidAABB.minX = min(cx, globalCentroidAABB.minX);
		globalCentroidAABB.minY = min(cy, globalCentroidAABB.minY);
		globalCentroidAABB.minZ = min(cz, globalCentroidAABB.minZ);
		globalCentroidAABB.maxX = max(cx, globalCentroidAABB.maxX);
		globalCentroidAABB.maxY = max(cy, globalCentroidAABB.maxY);
		globalCentroidAABB.maxZ = max(cz, globalCentroidAABB.maxZ);
	}

	// For each triangle located within our node, we assign a bin ID (using centroid of triangle and centroid bounds)
	float longestAxisLength = 0.0f;
	int longestAxisIdx = -1;

	float length = globalCentroidAABB.maxX - globalCentroidAABB.minX;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 0;
	}

	length = globalCentroidAABB.maxY - globalCentroidAABB.minY;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 1;
	}

	length = globalCentroidAABB.maxZ - globalCentroidAABB.minZ;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 2;
	}

	if (longestAxisIdx == -1) {
		return;
	}
	glm::fvec3 minBBox = glm::fvec3(globalCentroidAABB.minX, globalCentroidAABB.minY, globalCentroidAABB.minZ);
	glm::fvec3 maxBBox = glm::fvec3(globalCentroidAABB.maxX, globalCentroidAABB.maxY, globalCentroidAABB.maxZ);

	auto split = (maxBBox[longestAxisIdx] + minBBox[longestAxisIdx]) / 2.0f;
	
	auto right = std::partition(&hittableIdxs[node->leftFirst], &hittableIdxs[node->leftFirst + node->count], [&](int idx){
				auto aabb = hittables[idx]->getWorldAABB();
				glm::fvec3 centroid = glm::fvec3((aabb.minX + aabb.maxX) / 2.0f, (aabb.minY + aabb.maxY) / 2.0f, (aabb.minZ + aabb.maxZ) / 2.0f);
				return centroid[longestAxisIdx] < split;
			});

	auto first = node->leftFirst;
	auto numElems = node->count;
	node->count = 0;
	node->leftFirst = poolPtr;
	auto leftNode = &this->nodePool[poolPtr++];
	auto rightNode = &this->nodePool[poolPtr++];

	// Asign leftFirst and count to our left and right nodes
	leftNode->leftFirst = first;
	leftNode->count = right - &hittableIdxs[first];
	computeBounding(leftNode);

	rightNode->leftFirst = first + leftNode->count;
	rightNode->count = &hittableIdxs[first + numElems] - right;
	computeBounding(rightNode);
}

void BVH::subdivideHQ(BVHNode* node) {
	if (node == nullptr) return;
	if (node->count < 3) {
		computeBounding(node);
		return; // Jacco said this was wrong (or not efficient). Why?
	}

	partitionHQ(node);
	// Then subdivide again 
	subdivideHQ(&this->nodePool[(int)node->leftFirst]);
	subdivideHQ(&this->nodePool[(int)node->leftFirst + 1]);

	return;
}

void BVH::partitionBinMulti(BVHNode* node) {

	// For a partition of a node
	// Divide the node into k bins vertically along its longest AABB axis.
	float numOfBins = 16.0f;
	auto numSplits = numOfBins - 1;

	int threadNum = OptionsMap::Instance()->getOption(Options::THREADS);
	int nChunks = min((int)node->count, threadNum);

	std::vector<std::future<void>> futures;
	std::vector<BinningJob> binnings(nChunks);
	std::vector<AABB> chunkBoundingBoxes(nChunks);
	for (size_t j = 0; j < nChunks; ++j) {
		int threadNodeStart = node->leftFirst + std::ceil((j * node->count) / (float)nChunks);
		int threadNodeEnd = node->leftFirst + std::ceil(((j + 1) * node->count) / (float)(nChunks));
		futures.push_back(Threading::pool.queue([&, threadNodeStart, threadNodeEnd, j](uint32_t& rng) {
			// Compute the centroid bounds (the bounds defined by the centroids of all triangles within the node)
			AABB centroidBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
			for (size_t i = threadNodeStart; i < threadNodeEnd; ++i) {
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
			chunkBoundingBoxes[j] = centroidBBox;
		}));
	}

	for (auto& f : futures) {
		f.get();
	}
	futures.clear();

	AABB globalCentroidAABB = AABB{ INF,INF,INF,-INF,-INF,-INF };
	for (const auto& aabb : chunkBoundingBoxes) {
		globalCentroidAABB.minX = min(aabb.minX, globalCentroidAABB.minX);
		globalCentroidAABB.minY = min(aabb.minY, globalCentroidAABB.minY);
		globalCentroidAABB.minZ = min(aabb.minZ, globalCentroidAABB.minZ);
		globalCentroidAABB.maxX = max(aabb.maxX, globalCentroidAABB.maxX);
		globalCentroidAABB.maxY = max(aabb.maxY, globalCentroidAABB.maxY);
		globalCentroidAABB.maxZ = max(aabb.maxZ, globalCentroidAABB.maxZ);
	}

	// For each triangle located within our node, we assign a bin ID (using centroid of triangle and centroid bounds)
	float longestAxisLength = 0.0f;
	int longestAxisIdx = -1;

	float length = globalCentroidAABB.maxX - globalCentroidAABB.minX;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 0;
	}

	length = globalCentroidAABB.maxY - globalCentroidAABB.minY;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 1;
	}

	length = globalCentroidAABB.maxZ - globalCentroidAABB.minZ;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 2;
	}

	if (longestAxisIdx == -1) {
		return;
	}

	glm::fvec3 minBBox = glm::fvec3(globalCentroidAABB.minX, globalCentroidAABB.minY, globalCentroidAABB.minZ);
	glm::fvec3 maxBBox = glm::fvec3(globalCentroidAABB.maxX, globalCentroidAABB.maxY, globalCentroidAABB.maxZ);

	float k1 = numOfBins * (1.0f - 0.00001f) / (maxBBox[longestAxisIdx] - minBBox[longestAxisIdx]);
	float k0 = minBBox[longestAxisIdx];

	for (size_t j = 0; j < nChunks; ++j) {
		int threadNodeStart = node->leftFirst + std::ceil((j * node->count) / (float)nChunks);
		int threadNodeEnd = node->leftFirst + std::ceil(((j + 1) * node->count) / (float)(nChunks));
		futures.push_back(Threading::pool.queue([&, threadNodeStart, threadNodeEnd, j](uint32_t& rng) {

			std::vector<Bin> bins(numOfBins);

			for (size_t i = threadNodeStart; i < threadNodeEnd; ++i) {
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
			std::vector<int> nLeft(numSplits);
			std::vector<int> nRight(numSplits);

			auto nLeftCount = 0;
			auto nRightCount = threadNodeEnd - threadNodeStart;

			for (int split = 0; split < numSplits; ++split) {
				nLeftCount += bins[split].count;
				nLeft[split] = nLeftCount;

				nRightCount -= bins[split].count;
				nRight[split] = nRightCount;
			}

			binnings[j].bins = bins;
			binnings[j].nLeft = nLeft;
			binnings[j].nRight = nRight;

		}));
	}

	for(auto &f : futures){
		f.get();
	}
	futures.clear();

	// Find best partition from the one in binnings;
	// Is this the best way to do it? Obviously not, I'm doing a mess here.
	std::vector<Bin> bins(numOfBins);

	for(int i = 0; i < bins.size(); ++i){
		// Join the bins from the N threads;
		for(int j = 0; j < binnings.size(); ++j){
			bins[i].count += binnings[j].bins[i].count;
			bins[i].aabb.maxX = max(binnings[j].bins[i].aabb.maxX, bins[i].aabb.maxX);
			bins[i].aabb.maxY = max(binnings[j].bins[i].aabb.maxY, bins[i].aabb.maxY);
			bins[i].aabb.maxZ = max(binnings[j].bins[i].aabb.maxZ, bins[i].aabb.maxZ);
			bins[i].aabb.minX = min(binnings[j].bins[i].aabb.minX, bins[i].aabb.minX);
			bins[i].aabb.minY = min(binnings[j].bins[i].aabb.minY, bins[i].aabb.minY);
			bins[i].aabb.minZ = min(binnings[j].bins[i].aabb.minZ, bins[i].aabb.minZ);
		}
	}

	std::vector<std::pair<int, float>> leftNumArea(numSplits);

	auto leftElemCount = 0;
	AABB leftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	AABB rightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	for (int split = 0; split < numSplits; ++split) {
		auto bin = bins[split];
		leftElemCount += bin.count;
		
		leftBBox.minX = min(leftBBox.minX, bin.aabb.minX);
		leftBBox.minY = min(leftBBox.minY, bin.aabb.minY);
		leftBBox.minZ = min(leftBBox.minZ, bin.aabb.minZ);
		leftBBox.maxX = max(leftBBox.maxX, bin.aabb.maxX);
		leftBBox.maxY = max(leftBBox.maxY, bin.aabb.maxY);
		leftBBox.maxZ = max(leftBBox.maxZ, bin.aabb.maxZ);

		leftNumArea[split] = (std::make_pair(leftElemCount, calculateSurfaceArea(leftBBox)));
	}

	int optimalSplitIdx = -1;
	auto lowestCost = INF;
	int optimalLeftCount = 0;
	int optimalRightCount = 0;
	AABB optimalLeftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	AABB optimalRightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };

	int rightElemCount = 0;
	for (int split = numSplits; split > 0; --split) {
		auto bin = bins[split];
		rightElemCount += bin.count;

		rightBBox.minX = min(rightBBox.minX, bin.aabb.minX);
		rightBBox.minY = min(rightBBox.minY, bin.aabb.minY);
		rightBBox.minZ = min(rightBBox.minZ, bin.aabb.minZ);
		rightBBox.maxX = max(rightBBox.maxX, bin.aabb.maxX);
		rightBBox.maxY = max(rightBBox.maxY, bin.aabb.maxY);
		rightBBox.maxZ = max(rightBBox.maxZ, bin.aabb.maxZ);

		auto leftCount = leftNumArea[split - 1].first;
		auto leftArea = leftNumArea[split - 1].second;
		auto splitCost = leftArea * leftCount + calculateSurfaceArea(rightBBox) * rightElemCount;

		if (splitCost < lowestCost) {
			lowestCost = splitCost;
			optimalSplitIdx = split;
			optimalLeftCount = leftCount;
			optimalRightCount = rightElemCount;
			optimalLeftBBox = leftBBox;
			optimalRightBBox = rightBBox;
		}
	}


	std::vector<int> threadCumulativeLeftCount(binnings.size() + 1);
	threadCumulativeLeftCount[0] = 0;
	std::vector<int> threadCumulativeRightCount(binnings.size() + 1);
	threadCumulativeRightCount[binnings.size()] = 0;

	for (int i = 0; i < binnings.size(); ++i) {
		threadCumulativeLeftCount[i + 1] = binnings[i].nLeft[optimalSplitIdx - 1];
		threadCumulativeRightCount[i] = binnings[i].nRight[optimalSplitIdx - 1];
	}

	// The non-threaded version
	//Quicksort our hittableIdx 
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

	//for (size_t j = 0; j < nChunks; ++j) {
	//	int threadNodeStart = node->minAABBLeftFirst.w + std::ceil((j * node->maxAABBCount.w) / (float)nChunks);
	//	int threadNodeEnd = node->minAABBLeftFirst.w + std::ceil(((j + 1) * node->maxAABBCount.w) / (float)(nChunks));
	//	futures.push_back(Threading::pool.queue([&, threadNodeStart, threadNodeEnd, j](uint32_t& rng) {

	//		auto threadTriangles = std::vector<int>(&hittableIdxs[threadNodeStart], &hittableIdxs[threadNodeEnd - 1]);
	//		int leftIndex = node->minAABBLeftFirst.w + threadCumulativeLeftCount[j];
	//		int rightIndex = (node->minAABBLeftFirst.w + node->maxAABBCount.w) - threadCumulativeRightCount[j];

	//		for (size_t i = 0; i < threadTriangles.size(); ++i) {
	//			auto prim = hittables[threadTriangles[i]];
	//			int binID = calculateBinID(prim->getWorldAABB(), k1, k0, longestAxisIdx);

	//			if (binID < optimalSplitIdx) {
	//				hittableIdxs[leftIndex++] = threadTriangles[i];
	//			} else { 
	//				hittableIdxs[rightIndex++] = threadTriangles[i];
	//			}
	//		}
	//	}));
	//}

	//for (auto& f : futures) {
	//	f.get();
	//}
	//futures.clear();

	// Change this node to be an interior node by setting its count to 0 and setting leftFirst to the poolPtr index
	auto first = node->leftFirst;
	auto numElems = node->count;
	node->count = 0;
	node->leftFirst = poolPtr;
	auto leftNode = &this->nodePool[poolPtr++];
	auto rightNode = &this->nodePool[poolPtr++];

	// Asign leftFirst and count to our left and right nodes
	leftNode->minAABB.x = optimalLeftBBox.minX;
	leftNode->minAABB.y = optimalLeftBBox.minY;
	leftNode->minAABB.z = optimalLeftBBox.minZ;
	leftNode->leftFirst = first;

	leftNode->maxAABB.x = optimalLeftBBox.maxX;
	leftNode->maxAABB.y = optimalLeftBBox.maxY;
	leftNode->maxAABB.z = optimalLeftBBox.maxZ;
	leftNode->count = optimalLeftCount;


	rightNode->minAABB.x = optimalRightBBox.minX;
	rightNode->minAABB.y = optimalRightBBox.minY;
	rightNode->minAABB.z = optimalRightBBox.minZ;
	rightNode->leftFirst = first + optimalLeftCount;

	rightNode->maxAABB.x = optimalRightBBox.maxX;
	rightNode->maxAABB.y = optimalRightBBox.maxY;
	rightNode->maxAABB.z = optimalRightBBox.maxZ;
	rightNode->count = optimalRightCount;
}

void BVH::partitionBinSingle(BVHNode* node) {

	auto t1 = std::chrono::high_resolution_clock::now();

	// For a partition of a node
	// Divide the node into k bins vertically along its longest AABB axis.
	float numOfBins = 16.0f;
	auto numSplits = numOfBins - 1;

	// Compute the centroid bounds (the bounds defined by the centroids of all triangles within the node)
	AABB globalCentroidAABB = AABB{ INF,INF,INF,-INF,-INF,-INF };
	for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
		auto prim = hittables[hittableIdxs[i]];
		AABB aabb = prim->getWorldAABB();
		float cx = (aabb.minX + aabb.maxX) / 2.0f;
		float cy = (aabb.minY + aabb.maxY) / 2.0f;
		float cz = (aabb.minZ + aabb.maxZ) / 2.0f;

		globalCentroidAABB.minX = min(cx, globalCentroidAABB.minX);
		globalCentroidAABB.minY = min(cy, globalCentroidAABB.minY);
		globalCentroidAABB.minZ = min(cz, globalCentroidAABB.minZ);
		globalCentroidAABB.maxX = max(cx, globalCentroidAABB.maxX);
		globalCentroidAABB.maxY = max(cy, globalCentroidAABB.maxY);
		globalCentroidAABB.maxZ = max(cz, globalCentroidAABB.maxZ);
	}

	// For each triangle located within our node, we assign a bin ID (using centroid of triangle and centroid bounds)
	float longestAxisLength = 0.0f;
	int longestAxisIdx = -1;

	float length = globalCentroidAABB.maxX - globalCentroidAABB.minX;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 0;
	}

	length = globalCentroidAABB.maxY - globalCentroidAABB.minY;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 1;
	}

	length = globalCentroidAABB.maxZ - globalCentroidAABB.minZ;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 2;
	}

	if (longestAxisIdx == -1) {
		return;
	}

	glm::fvec3 minBBox = glm::fvec3(globalCentroidAABB.minX, globalCentroidAABB.minY, globalCentroidAABB.minZ);
	glm::fvec3 maxBBox = glm::fvec3(globalCentroidAABB.maxX, globalCentroidAABB.maxY, globalCentroidAABB.maxZ);

	float k1 = numOfBins * (1.0f - 0.00001f) / (maxBBox[longestAxisIdx] - minBBox[longestAxisIdx]);
	float k0 = minBBox[longestAxisIdx];

	std::vector<Bin> bins(numOfBins);

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

	std::vector<std::pair<int, float>> leftNumArea(numSplits);
	auto leftElemCount = 0;
	AABB leftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	AABB rightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	for (int split = 0; split < numSplits; ++split) {
		auto bin = bins[split];
		leftElemCount += bin.count;

		leftBBox.minX = min(leftBBox.minX, bin.aabb.minX);
		leftBBox.minY = min(leftBBox.minY, bin.aabb.minY);
		leftBBox.minZ = min(leftBBox.minZ, bin.aabb.minZ);
		leftBBox.maxX = max(leftBBox.maxX, bin.aabb.maxX);
		leftBBox.maxY = max(leftBBox.maxY, bin.aabb.maxY);
		leftBBox.maxZ = max(leftBBox.maxZ, bin.aabb.maxZ);

		leftNumArea[split] = (std::make_pair(leftElemCount, calculateSurfaceArea(leftBBox)));
	}

	int optimalSplitIdx = -1;
	auto lowestCost = INF;
	int optimalLeftCount = 0;
	int optimalRightCount = 0;
	AABB optimalLeftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	AABB optimalRightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };

	int rightElemCount = 0;
	for (int split = numSplits; split > 0; --split) {
		auto bin = bins[split];
		rightElemCount += bin.count;

		rightBBox.minX = min(rightBBox.minX, bin.aabb.minX);
		rightBBox.minY = min(rightBBox.minY, bin.aabb.minY);
		rightBBox.minZ = min(rightBBox.minZ, bin.aabb.minZ);
		rightBBox.maxX = max(rightBBox.maxX, bin.aabb.maxX);
		rightBBox.maxY = max(rightBBox.maxY, bin.aabb.maxY);
		rightBBox.maxZ = max(rightBBox.maxZ, bin.aabb.maxZ);

		auto leftCount = leftNumArea[split - 1].first;
		auto leftArea = leftNumArea[split - 1].second;
		auto splitCost = leftArea * leftCount + calculateSurfaceArea(rightBBox) * rightElemCount;

		if (splitCost < lowestCost) {
			lowestCost = splitCost;
			optimalSplitIdx = split;
			optimalLeftCount = leftCount;
			optimalRightCount = rightElemCount;
			optimalLeftBBox = leftBBox;
			optimalRightBBox = rightBBox;
		}
	}

	//Quicksort our hittableIdx 
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
	leftNode->minAABB.x = optimalLeftBBox.minX;
	leftNode->minAABB.y = optimalLeftBBox.minY;
	leftNode->minAABB.z = optimalLeftBBox.minZ;
	leftNode->leftFirst = first;

	leftNode->maxAABB.x = optimalLeftBBox.maxX;
	leftNode->maxAABB.y = optimalLeftBBox.maxY;
	leftNode->maxAABB.z = optimalLeftBBox.maxZ;
	leftNode->count = optimalLeftCount;


	rightNode->minAABB.x = optimalRightBBox.minX;
	rightNode->minAABB.y = optimalRightBBox.minY;
	rightNode->minAABB.z = optimalRightBBox.minZ;
	rightNode->leftFirst = first + optimalLeftCount;

	rightNode->maxAABB.x = optimalRightBBox.maxX;
	rightNode->maxAABB.y = optimalRightBBox.maxY;
	rightNode->maxAABB.z = optimalRightBBox.maxZ;
	rightNode->count = optimalRightCount;
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
		node->minAABB.x,
		node->minAABB.y,
		node->minAABB.z,
		node->maxAABB.x,
		node->maxAABB.y,
		node->maxAABB.z
	};

	float parentCost = calculateSurfaceArea(parentNodeAABB) * node->count;
	auto lowestCost = parentCost;

	AABB optimalLeftBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };
	AABB optimalRightBBox = AABB{ INF,INF,INF,-INF,-INF,-INF };

	for (int axis = 0; axis < 3; ++axis) {
		for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
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

			for (size_t j = node->leftFirst; j < node->leftFirst + node->count; ++j) {
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
	int maxj = node->leftFirst + node->count - 1;
	for (size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i) {
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
	auto first = node->leftFirst;
	node->count = 0;
	node->leftFirst = poolPtr;
	auto leftNode = &this->nodePool[poolPtr++];
	auto rightNode = &this->nodePool[poolPtr++];

	// Asign leftFirst and count to our left and right nodes
	leftNode->minAABB.x = optimalLeftBBox.minX;
	leftNode->minAABB.y = optimalLeftBBox.minY;
	leftNode->minAABB.z = optimalLeftBBox.minZ;
	leftNode->leftFirst = first;

	leftNode->maxAABB.x = optimalLeftBBox.maxX;
	leftNode->maxAABB.y = optimalLeftBBox.maxY;
	leftNode->maxAABB.z = optimalLeftBBox.maxZ;
	leftNode->count = optimalLeftCount;


	rightNode->minAABB.x = optimalRightBBox.minX;
	rightNode->minAABB.y = optimalRightBBox.minY;
	rightNode->minAABB.z = optimalRightBBox.minZ;
	rightNode->leftFirst = first + optimalLeftCount;

	rightNode->maxAABB.x = optimalRightBBox.maxX;
	rightNode->maxAABB.y = optimalRightBBox.maxY;
	rightNode->maxAABB.z = optimalRightBBox.maxZ;
	rightNode->count = optimalRightCount;
}

float BVH::calculateSurfaceArea(AABB bbox) {

	auto length = bbox.maxX - bbox.minX;
	auto height = bbox.maxY - bbox.minY;
	auto width = bbox.maxZ - bbox.minZ;

	return 2 * (width * length + height * length + height * width);
}

float BVH::calculateBinID(AABB primAABB, float k1, float k0, int longestAxisIdx) {
	glm::fvec3 centroid = glm::fvec3((primAABB.minX + primAABB.maxX) / 2.0f, (primAABB.minY + primAABB.maxY) / 2.0f, (primAABB.minZ + primAABB.maxZ) / 2.0f);
	return k1 * (centroid[longestAxisIdx] - k0);
}

bool BVH::hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
	auto transformInv = transform.getInverse();
	auto transposeInv = transform.getTransposeInverse();
	auto transformMat = transform.getMatrix();
	const Ray transformedRay = ray.transformRay(transformInv);

	HitRecord tmp;
	if (traverse(transformedRay, this->root, tMin, tMax, tmp)) {
		rec = tmp;
		rec.p = transformMat * glm::fvec4(rec.p, 1.0f);
		rec.setFaceNormal(ray, transposeInv * glm::fvec4(rec.normal, 0.0));
		return true;
	}
	return false;
}

bool BVH::traverse(const Ray& ray, BVHNode* node, float& tMin, float& tMax, HitRecord& rec) const {
	HitRecord tmp;
	bool hasHit = false;
	float closest = tMax;

	BVHNode* nodestack[64];
	size_t stackPtr = 0;
	nodestack[stackPtr++] = node; // Push the root into the stack
	while(stackPtr != 0){
		BVHNode* currNode = nodestack[--stackPtr];
		if(currNode->count != 0) {// I'm a leaf
			for (size_t i = currNode->leftFirst; i < currNode->leftFirst + currNode->count; ++i) {
				auto prim = hittables[hittableIdxs[i]];
				if (prim->hit(ray, tMin, closest, tmp)) {
					rec = tmp;
					closest = rec.t;
					hasHit = true;
				}
			}
			tMax = closest;
		} else {
			auto firstNode = &this->nodePool[(int)currNode->leftFirst];
			auto secondNode = &this->nodePool[(int)currNode->leftFirst + 1];
			float firstDistance = 0.0f;
			float secondDistance = 0.0f;
			bool hitAABBFirst = hitAABB(ray, firstNode->minAABB, firstNode->maxAABB, firstDistance);
			bool hitAABBSecond = hitAABB(ray, secondNode->minAABB, secondNode->maxAABB, secondDistance);
			if (hitAABBFirst && hitAABBSecond) {
				if(firstDistance < secondDistance && firstDistance < tMax){
					nodestack[stackPtr++] = secondNode;
					nodestack[stackPtr++] = firstNode;
				} else if (secondDistance < tMax) {
					nodestack[stackPtr++] = firstNode;
					nodestack[stackPtr++] = secondNode;
				}
			} else if(hitAABBFirst && firstDistance < tMax){
				nodestack[stackPtr++] = firstNode;
			} else if(hitAABBSecond && secondDistance < tMax) {
				nodestack[stackPtr++] = secondNode;
			}
		}
	}
	return hasHit;
}

BVHNode* BVH::findBestMatch(BVHNode* target, std::list<BVHNode*> nodes) {
	BVHNode* bestMatch;
	float bestSurfaceArea = INF;
	for (auto &node: nodes) {
		if (node != target) {
			AABB aabb = { node->minAABB.x, node->minAABB.y, node->minAABB.z, node->maxAABB.x, node->maxAABB.y, node->maxAABB.z };
			float surfaceArea = calculateSurfaceArea(aabb);
			if (surfaceArea < bestSurfaceArea) {
				bestMatch = node;
				bestSurfaceArea = surfaceArea;
			}
		}
	}

	return bestMatch;
}

void BVH::refitNode(BVHNode* node){
	if(node == nullptr) return;
	if(node->count != 0){ /* Leaf! Refit */
		computeBounding(node);
	} else {
		auto leftNode = &this->nodePool[(int)node->leftFirst];
		refitNode(leftNode);
		computeBounding(leftNode);
		auto rightNode = &this->nodePool[(int)node->leftFirst + 1];
		refitNode(rightNode);
		computeBounding(rightNode);
	}
}


void BVH::refit() {
	if(this->refitCounter > 3) {
		this->refitCounter = 0;
		constructSubBVH();
	} else {
		++refitCounter;
		refitNode(this->root);
	}
}

bool BVH::update(float dt) {
	bool ret = false;
	if(animate){
		animationManager.update(dt);
		if(animationManager.isStarted() && !animationManager.isEnded()){
			this->setTransform(animationManager.getNextTransform());
			ret = true;
		}
	}
	if(mustRefit) {
		this->refit();
		ret = true;
	}
	return ret;
}

bool BVH::updateNode(BVHNode* node, float dt){
	if(node == nullptr) return false;
	bool ret = false;
	if(node->count != 0){ /* Leaf! Updates */
		for(size_t i = node->leftFirst; i < node->leftFirst + node->count; ++i){
			ret |= hittables[hittableIdxs[i]]->update(dt);
		}
	} else {
		auto leftNode = &this->nodePool[(int)node->leftFirst];
		ret |= updateNode(leftNode, dt);
		auto rightNode = &this->nodePool[(int)node->leftFirst + 1];
		ret |= updateNode(rightNode, dt);
	}
	return ret;
}
