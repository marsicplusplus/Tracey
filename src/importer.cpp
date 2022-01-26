#include "importer.hpp"

#include "hittables/curve.hpp"
#include "glm/vec3.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

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
		auto common = std::make_shared<CurveCommon>(&controlPoints[0], 0.001, 0.001);
		curves.push_back(
			std::make_shared<Curve>(
				0.0, 1.0,
				isClosed,
				mat,
				common
			)
		);
	}
	CLOSE_RETURN(pFile, true);
}

bool Importer::readPBRCurve(std::filesystem::path p, std::vector<HittablePtr> &curves, int mat, int numSegments){
	std::ifstream file(p);
	if (file.is_open()) {
		std::string line;
		// Shape "curve" "string type" [ "cylinder" ] "point P" [ 0.368558 9.90303 1.79676 0.302866 9.9561 1.92861 0.237174 10.0092 2.06046 0.233016 9.95108 2.07153 ] "float width0" [ 0.002167 ] "float width1" [ 0.001167 ] 
		while (std::getline(file, line)) {
			auto pointPStr = "\"point P\" ";
			auto width0Str = "\"float width0\" ";
			auto width1Str = "\"float width1\" ";
			int idx = line.find(pointPStr);
			std::string tmp = line.substr(idx + strlen(pointPStr));
			std::string points = tmp.substr(1, tmp.find(']') - 1);
			std::istringstream iss(points.c_str());
			std::string word;
			float ptx, pty, ptz;
			glm::fvec3 curvePts[4];
			int k = 0;
			while (iss >> word) {
				ptx = atof(word.c_str());
				iss >> word;
				pty = atof(word.c_str());
				iss >> word;
				ptz = atof(word.c_str());
				curvePts[k++] = glm::fvec3(ptx, pty, ptz);
			}
			idx = line.find(width0Str);
			tmp = line.substr(idx + strlen(width0Str));
			std::string w0Str = tmp.substr(1, tmp.find(']') - 1);
			float w0 = atof(w0Str.c_str());

			idx = line.find(width1Str);
			tmp = line.substr(idx + strlen(width1Str));
			std::string w1Str = tmp.substr(1, tmp.find(']') - 1);
			float w1 = atof(w1Str.c_str());

			auto common = std::make_shared<CurveCommon>(curvePts, w0, w1);

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

		}
		file.close();
	}
	return true;
}
