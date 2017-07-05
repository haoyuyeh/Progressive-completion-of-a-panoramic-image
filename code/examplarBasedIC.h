#ifndef _EXAMPLARBASEDIC_H_
#define _EXAMPLARBASEDIC_H_ 

#include <cv.h>
#include <core/core.hpp>
#include <highgui.h>
#include <imgproc/imgproc.hpp>
#include "fillFront.h"
#include "patchPBP.h"
#include <time.h> // for srand 

using namespace cv;

class examplarBasedIC
{
public:
	string fileName;
	Mat inputImg, SSDInput, maskImg, outputImg, confidenceMap;  // inputImg, origin¬°BGR
	int patchSize, gap, imgH, imgW, pointNum, patchNum, pixelNum, missingPixelNum;
	float threshold;
	Scalar tMean;
	vector<fillFront> contourOfTargetRegion;
	vector<patchPBP> candidates;
	examplarBasedIC()
	{
	}
	void initial( string filename, vector<patchPBP> candi, Mat input, Mat mask, int patchS );  
	void run();

private:
	void getFillFront();
	float setNodeCp( Mat t );
	void getPriority();
	void fillPatch();
	bool checkPatchOverlapMissing( Mat mask );
	float SSD(Mat target, Mat candidate, Mat mask );
};

#endif