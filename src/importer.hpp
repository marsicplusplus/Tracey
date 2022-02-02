#ifndef __IMPORTER_HPP__
#define __IMPORTER_HPP__

#include "hittables/hittable.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace Importer {
	struct BCCHeader {
		char sign[3];
		unsigned char byteCount;
		char curveType[2];
		char dimensions;
		char upDimension;
		uint64_t curveCount;
		uint64_t totalControlPointCount;
		char fileInfo[40];
	};

	bool importBCC(std::filesystem::path p, std::vector<HittablePtr> &curves, int mat, int numSegments);
	bool importBEZ(std::filesystem::path p, std::vector<HittablePtr> &curves, int mat, int numSegments = 1);
};

#endif
