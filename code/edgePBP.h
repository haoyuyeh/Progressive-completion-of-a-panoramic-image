#ifndef _EDGEPBP_H_
#define _EDGEPBP_H_ 

#include <cv.h>
#include <core/core.hpp>
using namespace cv;

class edgePBP
{
public:
	Mat  Vpq;
	Point row, col;

	edgePBP( Point r, Point c )
	{
		row = r;
		col = c;
	}

	bool checkRowOrNot( Point p ); // if p == row, then return true
};

#endif