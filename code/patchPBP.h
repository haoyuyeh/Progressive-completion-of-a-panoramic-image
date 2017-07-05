#ifndef _PATCHPBP_H_
#define _PATCHPBP_H_ 

#include <cv.h>
#include <core/core.hpp>
using namespace cv;

class patchPBP
{
public:
	Point patchPos;
	int iCluster, sCluster;
	float Vp, leftMsg, rightMsg, upperMsg, lowerMsg;
	patchPBP(Point pos)
	{
		iCluster = 0;
		sCluster = 0;
		patchPos=pos;
		leftMsg=1000000;
		rightMsg=1000000;
		upperMsg=1000000;
		lowerMsg=1000000;
		Vp=0;
	}
};

#endif