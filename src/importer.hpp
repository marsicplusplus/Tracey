#ifndef __IMPORTER_HPP__
#define __IMPORTER_HPP__

#include "hittables/hittable.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace Importer {
	bool importBEZ(std::filesystem::path p, std::vector<HittablePtr> &curves, int mat, int numSegments = 1);
};

#endif
