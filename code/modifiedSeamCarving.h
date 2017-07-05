#ifndef _MODIFIEDSEAMCARVING_H_
#define _MODIFIEDSEAMCARVING_H_

#include <fstream>
#include <cv.h>
#include <core/core.hpp>
#include <highgui.h>
#include <imgproc/imgproc.hpp>
using namespace cv;


class modifiedSeamCarving
{
	//static Mat energyMap, missingRegion;
	public:
		string fileName;
		int backgroundColor;
		Mat originalImg, modifiedOriginalImg, originalImg_gray, energyMap, missingRegion,seamResult,tempModifiedOriginalImg,tempEn,tempMiss,tempSeamResult;	// missingRegion���]���¦�
		vector<Mat> longestSeam,element;
		vector<vector<Mat>> seamPath;

		modifiedSeamCarving()
		{
		}

		void initial(Mat img, string name, int bgcolor);
		void initialMissingRegion();
		void getEnergyMap();
		void getMissingRegion(bool sobelForB);
		//void showMissingRegionInOriginal();
		void getLongestSeam();	
		void optimalSeam(bool line); // line=1 ��ܴM��seam���ɭԥu��M+1,0,-1����m�C�Ϥ��A�h�b��i�v���d�򤺴M��C
		void movingSeam();
	private:
		void getSeam(int pos, bool horizotal);
		Mat getCumulatedEnergy(bool line, Mat energy, int dir);
		void changePos(int dir, int end, int row, int col);
};

#endif