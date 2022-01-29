#include "importer.hpp"

#include "hittables/curve.hpp"
#include "glm/vec3.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

// Num. Curves
// width0 width1
// ct1_c0[x] ct1_c0[y] ct1_c0[z]
// ct2_c0[x] ct2_c0[y] ct2_c0[z]
// ct3_c0[x] ct3_c0[y] ct3_c0[z]
// ct4_c0[x] ct4_c0[y] ct4_c0[z]
bool Importer::importBEZ(std::filesystem::path p, std::vector<HittablePtr> &curves, int mat, int numSegments) {
	std::ifstream file(p);
	if (file.is_open()) {
		std::string line;
		std::getline(file, line);
		int nCurves = std::stoi(line);
		std::cout << "N. curves: " << nCurves << std::endl;
		int i = 0;
		while(i < nCurves){
			int k = 0;
			glm::fvec3 ctrlPts[4];
 			// width0 width1
			std::getline(file, line);
			std::istringstream wiss(line);
			float width0, width1;
			wiss >> width0 >> width1;

			float tmp1, tmp2, tmp3;
			std::getline(file, line);
			std::istringstream c0iss(line);
			c0iss.str(line);
			c0iss >> tmp1 >> tmp2 >> tmp3;
			ctrlPts[k++] = (glm::fvec3{tmp1, tmp2, tmp3});

			std::getline(file, line);
			std::istringstream c1iss(line);
			c1iss.str(line);
			c1iss >> tmp1 >> tmp2 >> tmp3;
			ctrlPts[k++] = (glm::fvec3{tmp1, tmp2, tmp3});

			std::getline(file, line);
			std::istringstream c2iss(line);
			c2iss.str(line);
			c2iss >> tmp1 >> tmp2 >> tmp3;
			ctrlPts[k++] = (glm::fvec3{tmp1, tmp2, tmp3});

			std::getline(file, line);
			std::istringstream c3iss(line);
			c3iss.str(line);
			c3iss >> tmp1 >> tmp2 >> tmp3;
			ctrlPts[k++] = (glm::fvec3{tmp1, tmp2, tmp3});

			auto common = std::make_shared<CurveCommon>(ctrlPts, width0, width1);

			for (int i = 0; i < numSegments; i++) {
				float segmentSize = 1.0f / (float)numSegments;
				float uMin = i * segmentSize;
				float uMax = min((i + 1) * segmentSize, 1.0f);
				curves.push_back(
					std::make_shared<Curve>(
						uMin, uMax,
						false,
						mat,
						common
					)
				);
			}

			i++;
		}
	} else {
		file.close();
		return false;
	}
	std::cout << "Done parsing" << std::endl;
	file.close();
	return true;
}
