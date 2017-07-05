#ifndef _FILLFRONT_H_
#define _FILLFRONT_H_ 

#include <cv.h>
#include <core/core.hpp>
#include <highgui.h>
#include <imgproc/imgproc.hpp>

using namespace cv;

class fillFront
{
public:
	Point pos;
	float Cp, Dp, priority;
	fillFront( Point p )
	{
		pos = p;
		Cp = 0;
		Dp = 0;
		priority = 0;
	}
	fillFront()
	{
		Cp = 0;
		Dp = 0;
		priority = 0;
	}
};

#endif