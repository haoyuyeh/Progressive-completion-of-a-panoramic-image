#ifndef _NODEPBP_H_
#define _NODEPBP_H_ 

#include <cv.h>
#include <core/core.hpp>
#include "patchPBP.h"
#include "edgePBP.h"
using namespace cv;

class nodePBP
{
public:
	Point nodePos; // row=point.y, col=point.x
	nodePBP *upperNeighbor, *lowerNeighbor, *leftNeighbor, *rightNeighbor;
	Mat bestPatch;
	edgePBP *upEdge, *lowEdge, *leftEdge, *rightEdge;
	bool boundary, committed, alreadyVisited, beliefExist;
	float priority, confidentValue;
	int activeLabelNum;
	vector<patchPBP> nodeCandidate;
	vector<float> belief;

	nodePBP(int x, int y)
	{
		confidentValue = 0.0;
		boundary = false;
		committed=false;
		alreadyVisited = false;
		beliefExist=false;
		priority=0;
		nodePos.x=x;
		nodePos.y=y;
		upperNeighbor=nullptr;
		lowerNeighbor=nullptr;
		leftNeighbor=nullptr;
		rightNeighbor=nullptr;
		upEdge=nullptr;
		lowEdge=nullptr;
		leftEdge=nullptr;
		rightEdge=nullptr;
	}
};

#endif