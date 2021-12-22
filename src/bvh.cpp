#include "bvh.hpp"
#include "defs.hpp"
#include "options_manager.hpp"
#include <chrono>
#include <iostream>
#include <list>

BVH::BVH(std::vector<HittablePtr> h, bool makeTopLevel, bool anim) : hittables(h), animate(anim) {
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
		node->minAABBLeftFirst = { aabb.min.x, aabb.min.y, aabb.min.z, i };
		node->maxAABBCount = { aabb.max.x, aabb.max.y, aabb.max.z, 1 };
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
		glm::fvec4(
			this->root->minAABBLeftFirst.x,
			this->root->minAABBLeftFirst.y,
			this->root->minAABBLeftFirst.z,
			INF
		),
		glm::fvec4(
			this->root->maxAABBCount.x,
			this->root->maxAABBCount.y,
			this->root->maxAABBCount.z,
			-INF
		)
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
		glm::fvec4(
			this->root->minAABBLeftFirst.x,
			this->root->minAABBLeftFirst.y,
			this->root->minAABBLeftFirst.z,
			INF
		),
		glm::fvec4(
			this->root->maxAABBCount.x,
			this->root->maxAABBCount.y,
			this->root->maxAABBCount.z,
			-INF
		)
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
	if(node->maxAABBCount.w > 5000){
		int nChunks = min((int)node->maxAABBCount.w, threadNum);
		//printf("nChunks: %d\nNode Size: %d\n", nChunks, (int)node->maxAABBCount.w);
		// Each of the N threads works on (t*N)/T triangles
		for(size_t i = 0; i < nChunks; ++i){
			int threadNodeStart = node->minAABBLeftFirst.w + std::ceil((i * node->maxAABBCount.w)/(float)nChunks);
			int threadNodeEnd = node->minAABBLeftFirst.w + std::ceil(((i+1) * node->maxAABBCount.w)/(float)(nChunks));
			//printf("Thread %d\nStart: %d\nEnd: %d\n----------\n", i, threadNodeStart, threadNodeEnd);
			futures.push_back(Threading::pool.queue([&, threadNodeStart, threadNodeEnd, i](uint32_t &rng){
				AABB aabb{glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF)};
				for(size_t j = threadNodeStart; j < threadNodeEnd; ++j){
					auto hit = hittables[hittableIdxs[j]];
					AABB hitAABB = hit->getWorldAABB();
					aabb.min.x = min(aabb.min.x, hitAABB.min.x);
					aabb.min.y = min(aabb.min.y, hitAABB.min.y);
					aabb.min.z = min(aabb.min.z, hitAABB.min.z);
					aabb.max.x = max(aabb.max.x, hitAABB.max.x);
					aabb.max.y = max(aabb.max.y, hitAABB.max.y);
					aabb.max.z = max(aabb.max.z, hitAABB.max.z);
				}
				bboxes[i] = aabb;
			}));
		}

		for(size_t i = 0; i < futures.size(); ++i){
			futures[i].get();
			AABB aabb = bboxes[i];
			node->minAABBLeftFirst.x = min(aabb.min.x, node->minAABBLeftFirst.x);
			node->minAABBLeftFirst.y = min(aabb.min.y, node->minAABBLeftFirst.y);
			node->minAABBLeftFirst.z = min(aabb.min.z, node->minAABBLeftFirst.z);
			node->maxAABBCount.x = max(aabb.max.x, node->maxAABBCount.x);
			node->maxAABBCount.y = max(aabb.max.y, node->maxAABBCount.y);
			node->maxAABBCount.z = max(aabb.max.z, node->maxAABBCount.z);
		}
	} else {
		for(size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i){
			auto hit = hittables[hittableIdxs[i]];
			AABB aabb = hit->getWorldAABB();
			node->minAABBLeftFirst.x = min(aabb.min.x, node->minAABBLeftFirst.x);
			node->minAABBLeftFirst.y = min(aabb.min.y, node->minAABBLeftFirst.y);
			node->minAABBLeftFirst.z = min(aabb.min.z, node->minAABBLeftFirst.z);
			node->maxAABBCount.x = max(aabb.max.x, node->maxAABBCount.x);
			node->maxAABBCount.y = max(aabb.max.y, node->maxAABBCount.y);
			node->maxAABBCount.z = max(aabb.max.z, node->maxAABBCount.z);
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

	if (node->maxAABBCount.w > 5000)
		partitionBinMulti(node);
	else
		partitionBinSingle(node);

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

void BVH::partitionBinMulti(BVHNode* node) {

	// For a partition of a node
	// Divide the node into k bins vertically along its longest AABB axis.
	float numOfBins = 16.0f;
	auto numSplits = numOfBins - 1;

	int threadNum = OptionsMap::Instance()->getOption(Options::THREADS);
	int nChunks = min((int)node->maxAABBCount.w, threadNum);


	auto t1 = std::chrono::high_resolution_clock::now();



	std::vector<std::future<void>> futures;
	std::vector<BinningJob> binnings(nChunks);
	std::vector<AABB> chunkBoundingBoxes(nChunks);
	for (size_t j = 0; j < nChunks; ++j) {
		int threadNodeStart = node->minAABBLeftFirst.w + std::ceil((j * node->maxAABBCount.w) / (float)nChunks);
		int threadNodeEnd = node->minAABBLeftFirst.w + std::ceil(((j + 1) * node->maxAABBCount.w) / (float)(nChunks));
		futures.push_back(Threading::pool.queue([&, threadNodeStart, threadNodeEnd, j](uint32_t& rng) {
			// Compute the centroid bounds (the bounds defined by the centroids of all triangles within the node)
			AABB centroidBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
			__m128 centroidMinBBox = _mm_load_ps(&centroidBBox.min[0]);
			__m128 centroidMaxBBox = _mm_load_ps(&centroidBBox.max[0]);
			__m128 primBBox = { 0, 0, 0, 0 };

			for (size_t i = threadNodeStart; i < threadNodeEnd; ++i) {
				auto prim = hittables[hittableIdxs[i]];
				primBBox = _mm_load_ps(&prim->getWorldAABBCentroid()[0]);
				centroidMinBBox = _mm_min_ps(centroidMinBBox, primBBox);
				centroidMaxBBox = _mm_max_ps(centroidMaxBBox, primBBox);
			}

			_mm_store_ps(&centroidBBox.min[0], centroidMinBBox);
			_mm_store_ps(&centroidBBox.max[0], centroidMaxBBox);

			chunkBoundingBoxes[j] = centroidBBox;
		}));
	}

	for (auto& f : futures) {
		f.get();
	}
	futures.clear();

	auto t2 = std::chrono::high_resolution_clock::now();
	auto ms_int = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

	if (node->maxAABBCount.w > 500000) {
		std::cout << "Centroid BB Construction for " << node->maxAABBCount.w << " hittables: " << ms_int.count() << "us" << std::endl;
	}


	AABB globalCentroidAABB = AABB{glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF)};
	__m128 centroidMinBBox = _mm_load_ps(&globalCentroidAABB.min[0]);
	__m128 centroidMaxBBox = _mm_load_ps(&globalCentroidAABB.max[0]);
	__m128 chunkBoxMin = { 0, 0, 0, 0 };
	__m128 chunkBoxMax = { 0, 0, 0, 0 };

	for (const auto& aabb : chunkBoundingBoxes) {
		chunkBoxMin = _mm_load_ps(&aabb.min[0]);
		chunkBoxMax = _mm_load_ps(&aabb.max[0]);
		centroidMinBBox = _mm_min_ps(centroidMinBBox, chunkBoxMin);
		centroidMaxBBox = _mm_max_ps(centroidMaxBBox, chunkBoxMax);
	}

	_mm_store_ps(&globalCentroidAABB.min[0], centroidMinBBox);
	_mm_store_ps(&globalCentroidAABB.max[0], centroidMaxBBox);

	// For each triangle located within our node, we assign a bin ID (using centroid of triangle and centroid bounds)
	float longestAxisLength = 0.0f;
	int longestAxisIdx = -1;

	float length = globalCentroidAABB.max.x - globalCentroidAABB.min.x;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 0;
	}

	length = globalCentroidAABB.max.y - globalCentroidAABB.min.y;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 1;
	}

	length = globalCentroidAABB.max.z - globalCentroidAABB.min.z;
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 2;
	}

	if (longestAxisIdx == -1) {
		return;
	}

	glm::fvec3 minBBox = glm::fvec3(globalCentroidAABB.min.x, globalCentroidAABB.min.y, globalCentroidAABB.min.z);
	glm::fvec3 maxBBox = glm::fvec3(globalCentroidAABB.max.x, globalCentroidAABB.max.y, globalCentroidAABB.max.z);

	float k1 = numOfBins * (1.0f - 0.00001f) / (maxBBox[longestAxisIdx] - minBBox[longestAxisIdx]);
	float k0 = minBBox[longestAxisIdx];

	for (size_t j = 0; j < nChunks; ++j) {
		int threadNodeStart = node->minAABBLeftFirst.w + std::ceil((j * node->maxAABBCount.w) / (float)nChunks);
		int threadNodeEnd = node->minAABBLeftFirst.w + std::ceil(((j + 1) * node->maxAABBCount.w) / (float)(nChunks));
		futures.push_back(Threading::pool.queue([&, threadNodeStart, threadNodeEnd, j](uint32_t& rng) {

			std::vector<Bin> bins(numOfBins);

			for (size_t i = threadNodeStart; i < threadNodeEnd; ++i) {
				auto prim = hittables[hittableIdxs[i]];
				auto primAABB = prim->getWorldAABB();
				int binID = calculateBinID(prim->getWorldAABBCentroid(), k1, k0, longestAxisIdx);

				// For each bin we keep track of the number of triangles as well as the bins bounds
				bins[binID].count += 1;

				auto binAABB = bins[binID].aabb;
				binAABB.min.x = min(binAABB.min.x, primAABB.min.x);
				binAABB.min.y = min(binAABB.min.y, primAABB.min.y);
				binAABB.min.z = min(binAABB.min.z, primAABB.min.z);
				binAABB.max.x = max(binAABB.max.x, primAABB.max.x);
				binAABB.max.y = max(binAABB.max.y, primAABB.max.y);
				binAABB.max.z = max(binAABB.max.z, primAABB.max.z);
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
			bins[i].aabb.max.x = max(binnings[j].bins[i].aabb.max.x, bins[i].aabb.max.x);
			bins[i].aabb.max.y = max(binnings[j].bins[i].aabb.max.y, bins[i].aabb.max.y);
			bins[i].aabb.max.z = max(binnings[j].bins[i].aabb.max.z, bins[i].aabb.max.z);
			bins[i].aabb.min.x = min(binnings[j].bins[i].aabb.min.x, bins[i].aabb.min.x);
			bins[i].aabb.min.y = min(binnings[j].bins[i].aabb.min.y, bins[i].aabb.min.y);
			bins[i].aabb.min.z = min(binnings[j].bins[i].aabb.min.z, bins[i].aabb.min.z);
		}
	}

	std::vector<std::pair<int, float>> leftNumArea(numSplits);

	auto leftElemCount = 0;
	AABB leftBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
	AABB rightBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
	for (int split = 0; split < numSplits; ++split) {
		auto bin = bins[split];
		leftElemCount += bin.count;
		
		leftBBox.min.x = min(leftBBox.min.x, bin.aabb.min.x);
		leftBBox.min.y = min(leftBBox.min.y, bin.aabb.min.y);
		leftBBox.min.z = min(leftBBox.min.z, bin.aabb.min.z);
		leftBBox.max.x = max(leftBBox.max.x, bin.aabb.max.x);
		leftBBox.max.y = max(leftBBox.max.y, bin.aabb.max.y);
		leftBBox.max.z = max(leftBBox.max.z, bin.aabb.max.z);

		leftNumArea[split] = (std::make_pair(leftElemCount, calculateSurfaceArea(leftBBox)));
	}

	int optimalSplitIdx = -1;
	auto lowestCost = INF;
	int optimalLeftCount = 0;
	int optimalRightCount = 0;
	AABB optimalLeftBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
	AABB optimalRightBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };

	int rightElemCount = 0;
	for (int split = numSplits; split > 0; --split) {
		auto bin = bins[split];
		rightElemCount += bin.count;

		rightBBox.min.x = min(rightBBox.min.x, bin.aabb.min.x);
		rightBBox.min.y = min(rightBBox.min.y, bin.aabb.min.y);
		rightBBox.min.z = min(rightBBox.min.z, bin.aabb.min.z);
		rightBBox.max.x = max(rightBBox.max.x, bin.aabb.max.x);
		rightBBox.max.y = max(rightBBox.max.y, bin.aabb.max.y);
		rightBBox.max.z = max(rightBBox.max.z, bin.aabb.max.z);

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
	int maxj = node->minAABBLeftFirst.w + node->maxAABBCount.w - 1;
	for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
		auto leftIndex = hittableIdxs[i];
		auto leftPrim = hittables[leftIndex];
		int leftBinID = calculateBinID(leftPrim->getWorldAABBCentroid(), k1, k0, longestAxisIdx);

		if (leftBinID >= optimalSplitIdx) {
			for (size_t j = maxj; j > i; --j) {
				auto rightIndex = hittableIdxs[j];
				auto rightPrim = hittables[rightIndex];
				int rightBinID = calculateBinID(rightPrim->getWorldAABBCentroid(), k1, k0, longestAxisIdx);

				if (rightBinID < optimalSplitIdx) {
					hittableIdxs[i] = rightIndex;
					hittableIdxs[j] = leftIndex;
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
	auto first = node->minAABBLeftFirst.w;
	auto numElems = node->maxAABBCount.w;
	node->maxAABBCount.w = 0;
	node->minAABBLeftFirst.w = poolPtr;
	auto leftNode = &this->nodePool[poolPtr++];
	auto rightNode = &this->nodePool[poolPtr++];

	// Asign leftFirst and count to our left and right nodes
	leftNode->minAABBLeftFirst.x = optimalLeftBBox.min.x;
	leftNode->minAABBLeftFirst.y = optimalLeftBBox.min.y;
	leftNode->minAABBLeftFirst.z = optimalLeftBBox.min.z;
	leftNode->minAABBLeftFirst.w = first;

	leftNode->maxAABBCount.x = optimalLeftBBox.max.x;
	leftNode->maxAABBCount.y = optimalLeftBBox.max.y;
	leftNode->maxAABBCount.z = optimalLeftBBox.max.z;
	leftNode->maxAABBCount.w = optimalLeftCount;


	rightNode->minAABBLeftFirst.x = optimalRightBBox.min.x;
	rightNode->minAABBLeftFirst.y = optimalRightBBox.min.y;
	rightNode->minAABBLeftFirst.z = optimalRightBBox.min.z;
	rightNode->minAABBLeftFirst.w = first + optimalLeftCount;

	rightNode->maxAABBCount.x = optimalRightBBox.max.x;
	rightNode->maxAABBCount.y = optimalRightBBox.max.y;
	rightNode->maxAABBCount.z = optimalRightBBox.max.z;
	rightNode->maxAABBCount.w = optimalRightCount;
}

void BVH::partitionBinSingle(BVHNode* node) {

	auto t1 = std::chrono::high_resolution_clock::now();

	// For a partition of a node
	// Divide the node into k bins vertically along its longest AABB axis.
	float numOfBins = 16.0f;
	auto numSplits = numOfBins - 1;

	// Compute the centroid bounds (the bounds defined by the centroids of all triangles within the node)
	union { __m128 mmMin; float min4[4]; };
	union { __m128 mmMax; float max4[4]; };
	union { __m128 diff; float diff4[4]; };
	min4[0] = INF; min4[1] = INF; min4[2] = INF; min4[3] = INF;
	max4[0] = -INF; max4[1] = -INF; max4[2] = -INF; max4[3] = -INF;

	__m128 primCentroid = _mm_setzero_ps();

	auto start = node->minAABBLeftFirst.w;
	auto end = node->minAABBLeftFirst.w + node->maxAABBCount.w;
	for (size_t i = start; i < end; ++i) {
		auto prim = hittables[hittableIdxs[i]];
		primCentroid = _mm_load_ps(&prim->getWorldAABBCentroid()[0]);
		mmMin = _mm_min_ps(mmMin, primCentroid);
		mmMax = _mm_max_ps(mmMax, primCentroid);
	}

	// For each triangle located within our node, we assign a bin ID (using centroid of triangle and centroid bounds)
	float longestAxisLength = 0.0f;
	int longestAxisIdx = -1;

	diff = _mm_sub_ps(mmMax, mmMin);

	float length = diff4[0];
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 0;
	}

	length = diff4[1];
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 1;
	}

	length = diff4[2];
	if (length > longestAxisLength) {
		longestAxisLength = length;
		longestAxisIdx = 2;
	}

	if (longestAxisIdx == -1) {
		return;
	}

	float k1 = numOfBins * (1.0f - 0.00001f) / (max4[longestAxisIdx] - min4[longestAxisIdx]);
	float k0 = min4[longestAxisIdx];

	std::vector<Bin> bins(numOfBins);

	for (size_t i = start; i < end; ++i) {
		auto prim = hittables[hittableIdxs[i]];
		auto primAABB = prim->getWorldAABB();
		int binID = calculateBinID(prim->getWorldAABBCentroid(), k1, k0, longestAxisIdx);

		// For each bin we keep track of the number of triangles as well as the bins bounds
		bins[binID].count += 1;

		auto binAABB = bins[binID].aabb;
		binAABB.min.x = min(binAABB.min.x, primAABB.min.x);
		binAABB.min.y = min(binAABB.min.y, primAABB.min.y);
		binAABB.min.z = min(binAABB.min.z, primAABB.min.z);
		binAABB.max.x = max(binAABB.max.x, primAABB.max.x);
		binAABB.max.y = max(binAABB.max.y, primAABB.max.y);
		binAABB.max.z = max(binAABB.max.z, primAABB.max.z);
		bins[binID].aabb = binAABB;
	}

	std::vector<std::pair<int, float>> leftNumArea(numSplits);
	auto leftElemCount = 0;
	AABB leftBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
	AABB rightBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
	for (int split = 0; split < numSplits; ++split) {
		auto bin = bins[split];
		leftElemCount += bin.count;

		leftBBox.min.x = min(leftBBox.min.x, bin.aabb.min.x);
		leftBBox.min.y = min(leftBBox.min.y, bin.aabb.min.y);
		leftBBox.min.z = min(leftBBox.min.z, bin.aabb.min.z);
		leftBBox.max.x = max(leftBBox.max.x, bin.aabb.max.x);
		leftBBox.max.y = max(leftBBox.max.y, bin.aabb.max.y);
		leftBBox.max.z = max(leftBBox.max.z, bin.aabb.max.z);

		leftNumArea[split] = (std::make_pair(leftElemCount, calculateSurfaceArea(leftBBox)));
	}

	int optimalSplitIdx = -1;
	auto lowestCost = INF;
	int optimalLeftCount = 0;
	int optimalRightCount = 0;
	AABB optimalLeftBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
	AABB optimalRightBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };

	int rightElemCount = 0;
	for (int split = numSplits; split > 0; --split) {
		auto bin = bins[split];
		rightElemCount += bin.count;

		rightBBox.min.x = min(rightBBox.min.x, bin.aabb.min.x);
		rightBBox.min.y = min(rightBBox.min.y, bin.aabb.min.y);
		rightBBox.min.z = min(rightBBox.min.z, bin.aabb.min.z);
		rightBBox.max.x = max(rightBBox.max.x, bin.aabb.max.x);
		rightBBox.max.y = max(rightBBox.max.y, bin.aabb.max.y);
		rightBBox.max.z = max(rightBBox.max.z, bin.aabb.max.z);

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
	int maxj = end - 1;
	for (size_t i = start; i < end; ++i) {
		auto leftIndex = hittableIdxs[i];
		auto leftPrim = hittables[leftIndex];
		int leftBinID = calculateBinID(leftPrim->getWorldAABBCentroid(), k1, k0, longestAxisIdx);

		if (leftBinID >= optimalSplitIdx) {
			for (size_t j = maxj; j > i; --j) {
				auto rightIndex = hittableIdxs[j];
				auto rightPrim = hittables[rightIndex];
				int rightBinID = calculateBinID(rightPrim->getWorldAABBCentroid(), k1, k0, longestAxisIdx);

				if (rightBinID < optimalSplitIdx) {
					hittableIdxs[i] = rightIndex;
					hittableIdxs[j] = leftIndex;
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
	leftNode->minAABBLeftFirst.x = optimalLeftBBox.min.x;
	leftNode->minAABBLeftFirst.y = optimalLeftBBox.min.y;
	leftNode->minAABBLeftFirst.z = optimalLeftBBox.min.z;
	leftNode->minAABBLeftFirst.w = first;

	leftNode->maxAABBCount.x = optimalLeftBBox.max.x;
	leftNode->maxAABBCount.y = optimalLeftBBox.max.y;
	leftNode->maxAABBCount.z = optimalLeftBBox.max.z;
	leftNode->maxAABBCount.w = optimalLeftCount;


	rightNode->minAABBLeftFirst.x = optimalRightBBox.min.x;
	rightNode->minAABBLeftFirst.y = optimalRightBBox.min.y;
	rightNode->minAABBLeftFirst.z = optimalRightBBox.min.z;
	rightNode->minAABBLeftFirst.w = first + optimalLeftCount;

	rightNode->maxAABBCount.x = optimalRightBBox.max.x;
	rightNode->maxAABBCount.y = optimalRightBBox.max.y;
	rightNode->maxAABBCount.z = optimalRightBBox.max.z;
	rightNode->maxAABBCount.w = optimalRightCount;
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
		glm::fvec4(
			node->minAABBLeftFirst.x,
			node->minAABBLeftFirst.y,
			node->minAABBLeftFirst.z,
			INF
		),
		glm::fvec4(
			node->maxAABBCount.x,
			node->maxAABBCount.y,
			node->maxAABBCount.z,
			-INF
		)
	};

	float parentCost = calculateSurfaceArea(parentNodeAABB) * node->maxAABBCount.w;
	auto lowestCost = parentCost;

	AABB optimalLeftBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
	AABB optimalRightBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };

	for (int axis = 0; axis < 3; ++axis) {
		for (size_t i = node->minAABBLeftFirst.w; i < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++i) {
			int leftCount = 0;
			int rightCount = 0;

			AABB leftBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };
			AABB rightBBox = AABB{ glm::fvec4(INF, INF, INF, INF), glm::fvec4(-INF, -INF, -INF, -INF) };

			auto prim = hittables[hittableIdxs[i]];
			AABB splitAABB = prim->getWorldAABB();
			float splitCX = (splitAABB.min.x + splitAABB.max.x) / 2.0f;
			float splitCY = (splitAABB.min.y + splitAABB.max.y) / 2.0f;
			float splitCZ = (splitAABB.min.z + splitAABB.max.z) / 2.0f;
			auto splitCentroid = glm::fvec3(splitCX, splitCY, splitCZ);
			auto splitPos = splitCentroid[axis];

			for (size_t j = node->minAABBLeftFirst.w; j < node->minAABBLeftFirst.w + node->maxAABBCount.w; ++j) {
				auto prim = hittables[hittableIdxs[j]];
				AABB aabb = prim->getWorldAABB();
				float cx = (aabb.min.x + aabb.max.x) / 2.0f;
				float cy = (aabb.min.y + aabb.max.y) / 2.0f;
				float cz = (aabb.min.z + aabb.max.z) / 2.0f;
				auto centroid = glm::fvec3(cx, cy, cz);
				auto pos = centroid[axis];

				if (pos <= splitPos) {
					leftBBox.min.x = min(leftBBox.min.x, aabb.min.x);
					leftBBox.min.y = min(leftBBox.min.y, aabb.min.y);
					leftBBox.min.z = min(leftBBox.min.z, aabb.min.z);
					leftBBox.max.x = max(leftBBox.max.x, aabb.max.x);
					leftBBox.max.y = max(leftBBox.max.y, aabb.max.y);
					leftBBox.max.z = max(leftBBox.max.z, aabb.max.z);
					leftCount++;
				}
				else {
					rightBBox.min.x = min(rightBBox.min.x, aabb.min.x);
					rightBBox.min.y = min(rightBBox.min.y, aabb.min.y);
					rightBBox.min.z = min(rightBBox.min.z, aabb.min.z);
					rightBBox.max.x = max(rightBBox.max.x, aabb.max.x);
					rightBBox.max.y = max(rightBBox.max.y, aabb.max.y);
					rightBBox.max.z = max(rightBBox.max.z, aabb.max.z);
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
		float leftCX = (leftAABB.min.x + leftAABB.max.x) / 2.0f;
		float leftCY = (leftAABB.min.y + leftAABB.max.y) / 2.0f;
		float leftCZ = (leftAABB.min.z + leftAABB.max.z) / 2.0f;
		auto leftCentroid = glm::fvec3(leftCX, leftCY, leftCZ);
		auto leftPos = leftCentroid[optimalSplitIdx];

		if (leftPos > optimalSplitPos) {
			for (size_t j = maxj; j > i; --j) {
				auto rightPrim = hittables[hittableIdxs[j]];
				AABB rightAABB = rightPrim->getWorldAABB();
				float rightCX = (rightAABB.min.x + rightAABB.max.x) / 2.0f;
				float rightCY = (rightAABB.min.y + rightAABB.max.y) / 2.0f;
				float rightCZ = (rightAABB.min.z + rightAABB.max.z) / 2.0f;
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
	leftNode->minAABBLeftFirst.x = optimalLeftBBox.min.x;
	leftNode->minAABBLeftFirst.y = optimalLeftBBox.min.y;
	leftNode->minAABBLeftFirst.z = optimalLeftBBox.min.z;
	leftNode->minAABBLeftFirst.w = first;

	leftNode->maxAABBCount.x = optimalLeftBBox.max.x;
	leftNode->maxAABBCount.y = optimalLeftBBox.max.y;
	leftNode->maxAABBCount.z = optimalLeftBBox.max.z;
	leftNode->maxAABBCount.w = optimalLeftCount;


	rightNode->minAABBLeftFirst.x = optimalRightBBox.min.x;
	rightNode->minAABBLeftFirst.y = optimalRightBBox.min.y;
	rightNode->minAABBLeftFirst.z = optimalRightBBox.min.z;
	rightNode->minAABBLeftFirst.w = first + optimalLeftCount;

	rightNode->maxAABBCount.x = optimalRightBBox.max.x;
	rightNode->maxAABBCount.y = optimalRightBBox.max.y;
	rightNode->maxAABBCount.z = optimalRightBBox.max.z;
	rightNode->maxAABBCount.w = optimalRightCount;
}

float BVH::calculateSurfaceArea(AABB bbox) {

	auto length = bbox.max.x - bbox.min.x;
	auto height = bbox.max.y - bbox.min.y;
	auto width = bbox.max.z - bbox.min.z;

	return 2 * (width * length + height * length + height * width);
}

float BVH::calculateBinID(glm::fvec4 centroid, float k1, float k0, int longestAxisIdx) {
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
			AABB aabb = { {node->minAABBLeftFirst.x, node->minAABBLeftFirst.y, node->minAABBLeftFirst.z, INF}, {node->maxAABBCount.x, node->maxAABBCount.y, node->maxAABBCount.z, -INF} };
			float surfaceArea = calculateSurfaceArea(aabb);
			if (surfaceArea < bestSurfaceArea) {
				bestMatch = node;
				bestSurfaceArea = surfaceArea;
			}
		}
	}

	return bestMatch;
}

bool BVH::update(float dt) {
	return false;
}
