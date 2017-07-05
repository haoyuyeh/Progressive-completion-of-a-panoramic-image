#ifndef _MODIFIEDPRIORITYBP_H_
#define _MODIFIEDPRIORITYBP_H_ 

#include <cv.h>
#include <core/core.hpp>
#include <highgui.h>
#include <imgproc/imgproc.hpp>
#include "nodePBP.h"
#include "patchPBP.h"
#include "edgePBP.h"
#include "examplarBasedIC.h"
#include <vector>
#include <stdio.h>
#include <fstream>
#include <afx.h> // for 睲戈Ж柑ゅン
#include <time.h> // for srand 
#include <direct.h> // for _mkdir
#include <algorithm> // for min_element and max_element
using namespace cv;

class modifiedPriorityBP
{
public:
	string fileName;
	vector<vector<nodePBP>> mrfNodes;
	vector<nodePBP> element, boundaryNodes; //boundaryNode⊿ΤcandidatePatch㎝belief戈
	vector<nodePBP*> forwardOrder;
	vector<patchPBP> candidates;
	vector<Point> extraCandidate, tempCandidate;
	Mat inputImg, SSDImg, outputImg, maskImg, tempMaskImg, candidateMap, confidentMap, icOrigin;
	int imgH, imgW, patchSize, gap, candidateRange, Lmax, Lmin, iterationNum, mrfNodeNum, boundaryNum, candiNum, totalPixel, totalMissingPixel, dw, dh, interval;
	float Bconf, Bprune, cMax, cMin, cThreshold;

	modifiedPriorityBP()
	{
		Lmax=20; // –node玂痙程candidate patch计秖 
		Lmin=3; // –node玂痙程candidate patch计秖 
		iterationNum = 3;
		mrfNodeNum=0;
		boundaryNum = 0;
		cMin = 1;
		cMax = 0;
	}
	void initial(string filename, Mat img, Mat mask, int pSize);
	void run();
	void useCriminisiMethod();
	Mat superimposeBoundary( Mat r, Mat m );
	
private:
	// get MRF nodes
	void produceMrfNode();
	void checkNodeValid();
	bool checkPatchOverlapMissing(Mat mask);
	// construct the relation between nodes
	void setMrfNodeNeighbor();
	// checking the relation between nodes
	void validateNodeNeighbor();
	// get boundary nodes and candidates
	void getBoundaryNodes();
	void setBoundaryNodeNeighbor();
	void getCandidatePatchesFromBoundaryNodes();
	void getCandidatePatchesFromWholeImg();
	void addExtraCandidate();
	void changeCandidates();
	// calculate SSD
	float VpSSD(Mat target, Mat candidate, Mat mask);
	float VpqSSD(Mat target, Mat candidate);
	// calculate the Bconf and Bprune
	int decideCluster( float v );
	void getSSD0();
	////////// main part of Priority-BP //////////
	void priorityBP( bool allUse );
	// label pruning
	void getNodeInitialBeliefAndPriority();
	//void getNodeInitialBeliefAndPriority_modified();
	void labelPruning( nodePBP* node );
	// send msg
	void sendMsg( nodePBP* node, bool sendWhichOne );
	// calculate confident map
	void setNodeConfidence();
	void updateConfidentMap( int row, int col, float cv  );
	// image completion
	void fillMissingRegion( char* fName, bool isModified );
	// using criminisi's method to complete image
	
};

#endif