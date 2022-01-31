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

#define CLOSE_RETURN(X, Y) fclose((X)); return (Y);

bool Importer::readBCC(std::filesystem::path p, std::vector<HittablePtr> &curves, int mat){
	BCCHeader header;
	FILE *pFile = fopen(p.string().c_str(), "rb");
	fread(&header, sizeof(header), 1, pFile);

	if ( header.sign[0] != 'B' ) {CLOSE_RETURN(pFile, false);} 		// Invalid file signature
	if ( header.sign[1] != 'C' ) {CLOSE_RETURN(pFile, false);} 		// Invalid file signature
	if ( header.sign[2] != 'C' ) {CLOSE_RETURN(pFile, false);} 		// Invalid file signature
	if ( header.byteCount != 0x44 ) {CLOSE_RETURN(pFile, false);} 	// Only supporting 4-byte integers and floats

	if ( header.curveType[0] != 'C' ) {CLOSE_RETURN(pFile, false);} // Not a Catmull-Rom curve
	if ( header.curveType[1] != '0' ) {CLOSE_RETURN(pFile, false);} // Not uniform parameterization
	if ( header.dimensions != 3 ) {CLOSE_RETURN(pFile, false);} 	// Only curves in 3D

	curves.reserve(header.curveCount);
	for ( uint64_t i=0; i<header.curveCount; i++ ) {
		int curveControlPointCount;
		bool isClosed = false;
		fread(&curveControlPointCount, sizeof(int), 1, pFile);
		isClosed = curveControlPointCount < 0;
		if ( curveControlPointCount < 0 ) curveControlPointCount = -curveControlPointCount;
		std::vector<glm::fvec3> controlPoints(curveControlPointCount);

		for(uint64_t j = 0; j < curveControlPointCount; ++j){
			float cpx, cpy, cpz;
			fread(&cpx, sizeof(float), 1, pFile);
			fread(&cpy, sizeof(float), 1, pFile);
			fread(&cpz, sizeof(float), 1, pFile);
			controlPoints.push_back({cpx, cpy, cpz});
		}

		//TODO - Turn these splines into cubic bezier curves
		//auto common = std::make_shared<CurveCommon>(&controlPoints[0], 0.001, 0.001);
		//curves.push_back(
			//std::make_shared<Curve>(
				//0.0, 1.0,
				//isClosed,
				//mat,
				//common
			//)
		//);
	}
	CLOSE_RETURN(pFile, true);
}
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
