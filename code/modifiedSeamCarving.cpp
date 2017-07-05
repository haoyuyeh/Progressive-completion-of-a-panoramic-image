#include "stdafx.h"
#include "modifiedSeamCarving.h"
using namespace cv;
using namespace std;

void modifiedSeamCarving::initial(Mat img, string name, int bgcolor)
{
	fileName=name;
	backgroundColor=bgcolor;
	originalImg=img;
	modifiedOriginalImg=img;
}

void modifiedSeamCarving::initialMissingRegion()
{
	missingRegion=255*missingRegion.ones(originalImg.rows,originalImg.cols,CV_8UC1);
}

void modifiedSeamCarving::getEnergyMap()
{
	//char* window_name = "Sobel";
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	Mat originalImg_gray;
	/// Create window
	//namedWindow( window_name, CV_WINDOW_NORMAL );

	/// Generate grad_x and grad_y
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	GaussianBlur( originalImg, originalImg, Size(3,3), 0, 0, BORDER_DEFAULT );
	/// Convert it to gray
	cvtColor( originalImg, originalImg_gray, CV_RGB2GRAY );

	/// Gradient X
	//Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
	Sobel( originalImg_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
	convertScaleAbs( grad_x, abs_grad_x );
	grad_x.release();

	/// Gradient Y
	//Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
	Sobel( originalImg_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
	convertScaleAbs( grad_y, abs_grad_y );
	grad_y.release();
	originalImg_gray.release();

	/// Total Gradient (approximate)
	addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, energyMap );
	abs_grad_x.release();
	abs_grad_y.release();
	string name;
	name="C:/Completion Result/"+fileName+"/"+"energyMap.bmp";
	imwrite(name, energyMap);
	//imshow( window_name, energyMap );
}

void modifiedSeamCarving::getMissingRegion(bool sobelForB) //scroll bar to set threshold and elementSize
{	
	initialMissingRegion();
	if (sobelForB) //用sobel的結果圖來二值化，進而得到待修補的區域
	{
		int BThreshold=10;
		Mat a;
		for (int i=0;i<energyMap.rows;i++)
		{
			for (int j=0;j<energyMap.cols;j++) //由左往右找
			{
				if (energyMap.at<uchar>(i,j)>BThreshold)
				{
					if (j!=0) //判斷第一個位置是否為待填補區域，是的話才要做
					{
						a=missingRegion.rowRange(i,i+1).colRange(0,j);
						a.setTo(Scalar(0));
						a.release();
						break;
					}
					else
					{
						break;
					}
				}
			}
			for (int j=energyMap.cols-1;j>=0;j--) //由右往左找
			{

				if (energyMap.at<uchar>(i,j)>BThreshold)
				{
					if (j!=(missingRegion.cols-1)) //判斷第一個位置是否為待填補區域，是的話才要做
					{
						a=missingRegion.rowRange(i,i+1).colRange(j+1,missingRegion.cols);
						a.setTo(Scalar(0));
						a.release();
						break;
					}		
					else
					{
						break;
					}
				}
			}
		}
		for (int i=0;i<energyMap.cols;i++)
		{
			for (int j=0;j<energyMap.rows;j++) //由上往下找
			{
				if (energyMap.at<uchar>(j,i)>BThreshold)
				{
					if (j!=0) //判斷第一個位置是否為待填補區域，是的話才要做
					{
						a=missingRegion.rowRange(0,j).colRange(i,i+1);
						a.setTo(Scalar(0));
						a.release();
						break;
					}		
					else
					{
						break;
					}
				}
			}
			for (int j=energyMap.rows-1;j>=0;j--) //由下往上找
			{
				if (energyMap.at<uchar>(j,i)>BThreshold)
				{
					if (j!=(missingRegion.rows-1)) //判斷第一個位置是否為待填補區域，是的話才要做
					{
						a=missingRegion.rowRange(j+1,missingRegion.rows).colRange(i,i+1);
						a.setTo(Scalar(0));
						a.release();
						break;
					}			
					else
					{
						break;
					}
				}
			}
		}

		int elementSize=2;
		Mat mElement=getStructuringElement(MORPH_RECT, Size(2*elementSize+1,2*elementSize+1));
		erode(missingRegion, missingRegion, mElement);
		mElement.release();

		string name;
		name="output/"+fileName+"/"+"missingRegion.bmp";
		imwrite(name, missingRegion);
	} 
	else //用原圖的顏色來得到待修補區域
	{
		for (int i=0;i<originalImg.rows;i++)
		{
			for (int j=0;j<originalImg.cols;j++)
			{
				if (originalImg.at<Vec3b>(i, j)==Vec3b(backgroundColor,backgroundColor,backgroundColor))
				{
					missingRegion.at<uchar>(i, j)=0;
				} 
			}
		}
	}
	

	/*namedWindow( "missingRegion", CV_WINDOW_NORMAL );
	imshow("missingRegion", missingRegion);*/
}

//void modifiedSeamCarving::showMissingRegionInOriginal()
//{
//	imshow("missingRegion", missingRegion);
//	Mat temp;
//	originalImg.copyTo(temp);
//	for (int i=0;i<originalImg.rows;i++)
//	{
//		for (int j=0;j<originalImg.cols;j++)
//		{
//			if (missingRegion.at<uchar>(i,j)==backgroundColor)
//			{
//				temp.at<Vec3b>(i, j)=Vec3b(0,0,255);
//			} 
//		}
//	}
//	string name;
//	name="output/missingRegionInOriginal_"+fileName;
//	imwrite(name, temp);
//	namedWindow( "missingRegionInOriginal", CV_WINDOW_NORMAL );
//	imshow("missingRegionInOriginal", temp);
//}

Mat Img8Uto32S(Mat Img8U) // 影像Mat(8U)轉32F
{
	Mat Img32S(Img8U.rows,Img8U.cols,CV_32SC1);

	Img8U.convertTo(Img32S,CV_32SC1);

	return Img32S;
}

void SaveMatInfo(char *F_Name,Mat mat) //存Mat資料(TXT)
{
	char filename[100];
	strcpy(filename,F_Name);
	strcat(filename,".txt");
	FILE *file=fopen(filename,"a");
	fprintf(file,"開始\n");
	for(int i=0;i<mat.rows;i++)
	{
		for(int j=0;j<mat.cols;j++)
		{
			fprintf(file,"%4d ",mat.at<int>(i,j));
		}
		//fprintf(file,"%%\n");
		fprintf(file,"\n");
		//fprintf(file,"=========================\n");
	}
	fprintf(file,"=========================\n");
	fclose(file);
}

void findMin(Mat input)
{
	FILE *file=fopen("output/minEnergy.txt","w");
	double minVal=0;
	Point minLoc;
	minMaxLoc(input,&minVal,NULL,&minLoc,NULL);
	fprintf(file,"function找到的min=");
	fprintf(file,"%d",minVal);
	fprintf(file,"\n");
	fprintf(file,"col位置=");
	fprintf(file,"%d",minLoc.x);
	fprintf(file,"row位置=");
	fprintf(file,"%d",minLoc.y);
	fprintf(file,"\n");
	fclose(file);
	SaveMatInfo("output/path",input);
}

void findMax(Mat input)
{
	FILE *file=fopen("output/seam.txt","w");
	double maxVal=0;
	Point maxLoc;
	minMaxLoc(input,NULL,&maxVal,NULL,&maxLoc);
	fprintf(file,"function找到的max=");
	fprintf(file,"%d",maxVal);
	fprintf(file,"\n");
	fprintf(file,"col位置=");
	fprintf(file,"%d",maxLoc.x);
	fprintf(file,"row位置=");
	fprintf(file,"%d",maxLoc.y);
	fprintf(file,"\n");
	fclose(file);
	SaveMatInfo("output/seam",input);
}

void modifiedSeamCarving::getSeam(int pos, bool horizotal)
{
	bool seamStart=false;
	int length=0,way,start,end;
	if (horizotal)
	{
		for (int i=0;i<missingRegion.cols;i++)  //水平的seam
		{
			if (seamStart)
			{
				way=2;
			}
			else
			{
				way=1;
			}
			switch(way)
			{
			case 1:			
				if (missingRegion.at<uchar>(pos,i)==0)
				{
					Mat elements(1,4,CV_32SC1); // row, col, length, direction
					start=i;
					seamStart=true;
					longestSeam.push_back(elements);
					longestSeam[longestSeam.size()-1].at<int>(0,0)=pos; 
					longestSeam[longestSeam.size()-1].at<int>(0,1)=i;
					longestSeam[longestSeam.size()-1].at<int>(0,3)=0; // horizontal seam
					if (i==missingRegion.cols-1)
					{
						longestSeam[longestSeam.size()-1].at<int>(0,2)=1;
						//SaveMatInfo("output/longestSeam",longestSeam[longestSeam.size()-1]);
					}
				}
				break;
			case  2:
				if (missingRegion.at<uchar>(pos,i)!=0)
				{
					end=i;
					longestSeam[longestSeam.size()-1].at<int>(0,2)=end-start;
					seamStart=false;
					//SaveMatInfo("output/longestSeam",longestSeam[longestSeam.size()-1]);
				}
				else
				{
					if (i==missingRegion.cols-1)
					{
						longestSeam[longestSeam.size()-1].at<int>(0,2)=i-start+1;
						//SaveMatInfo("output/longestSeam",longestSeam[longestSeam.size()-1]);
					}
				}
				break;
			}
		}
	} 
	else
	{
		for (int i=0;i<missingRegion.rows;i++)  //垂直的seam
		{
			if (seamStart)
			{
				way=2;
			}
			else
			{
				way=1;
			}
			switch(way)
			{
			case 1:
				if (missingRegion.at<uchar>(i,pos)==0)
				{
					Mat elements(1,4,CV_32SC1); // row, col, length, direction
					start=i;
					seamStart=true;
					longestSeam.push_back(elements); 
					longestSeam[longestSeam.size()-1].at<int>(0,0)=i; 
					longestSeam[longestSeam.size()-1].at<int>(0,1)=pos;
					longestSeam[longestSeam.size()-1].at<int>(0,3)=1; // vertical seam
					if (i==missingRegion.rows-1)
					{
						longestSeam[longestSeam.size()-1].at<int>(0,2)=1;
						//SaveMatInfo("output/longestSeam",longestSeam[longestSeam.size()-1]);
					}
				}
				break;
			case  2:
				if (missingRegion.at<uchar>(i,pos)!=0)
				{
					end=i;
					longestSeam[longestSeam.size()-1].at<int>(0,2)=end-start;
					seamStart=false;
					//SaveMatInfo("output/longestSeam",longestSeam[longestSeam.size()-1]);
				}
				else
				{
					if (i==missingRegion.rows-1)
					{
						longestSeam[longestSeam.size()-1].at<int>(0,2)=i-start+1;
						//SaveMatInfo("output/longestSeam",longestSeam[longestSeam.size()-1]);
					}
				}
				break;
			}
		}
	}
}

void modifiedSeamCarving::getLongestSeam()
{
	longestSeam.clear();
	// horizontal 
	getSeam(0,true);
	getSeam(missingRegion.rows-1,true);
	// vertical
	getSeam(0,false);
	getSeam(missingRegion.cols-1,false);
}

Mat modifiedSeamCarving::getCumulatedEnergy(bool line, Mat minEnergy, int dir)
{
	for (int i=0;i<minEnergy.rows;i++)
	{
		seamPath.push_back(element);
		for (int j=0;j<minEnergy.cols;j++)
		{
			Mat xy(1,2,CV_32SC1,Scalar(0)); //存放上一個pixel的row,col座標。
			seamPath[i].push_back(xy);
		}
	}
	if (line)
	{
		if (dir==0) //水平
		{
			for (int col=1;col<minEnergy.cols;col++)
			{
				for (int row=0;row<minEnergy.rows;row++)
				{
					if (row==0)
					{
						Mat a;
						a=minEnergy.rowRange(row,row+2).colRange(col-1,col);
						double minVal=0;
						Point minLoc;
						minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
						minEnergy.at<int>(row,col)+=minVal;
						seamPath[row][col].at<int>(0,0)=row+minLoc.y;
						seamPath[row][col].at<int>(0,1)=col-1+minLoc.x;
					} 
					else
					{
						if (row==minEnergy.rows-1)
						{
							Mat a;
							a=minEnergy.rowRange(row-1,row+1).colRange(col-1,col);
							double minVal=0;
							Point minLoc;
							minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
							//SaveMatInfo("output/candidate",a);
							minEnergy.at<int>(row,col)+=minVal;
							seamPath[row][col].at<int>(0,0)=row-1+minLoc.y;
							seamPath[row][col].at<int>(0,1)=col-1+minLoc.x;
						} 
						else
						{
							Mat a;
							a=minEnergy.rowRange(row-1,row+2).colRange(col-1,col);
							double minVal=0;
							Point minLoc;
							minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
							minEnergy.at<int>(row,col)+=minVal;
							seamPath[row][col].at<int>(0,0)=row-1+minLoc.y;
							seamPath[row][col].at<int>(0,1)=col-1+minLoc.x;
						}
					}
				}
			}
		} 
		else
		{
			for (int row=1;row<minEnergy.rows;row++)
			{
				for (int col=0;col<minEnergy.cols;col++)
				{
					if (col==0)
					{
						Mat a;
						a=minEnergy.rowRange(row-1,row).colRange(col,col+2);
						double minVal=0;
						Point minLoc;
						minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
						minEnergy.at<int>(row,col)+=minVal;
						seamPath[row][col].at<int>(0,0)=row-1+minLoc.y;
						seamPath[row][col].at<int>(0,1)=col+minLoc.x;
					} 
					else
					{
						if (col==minEnergy.cols-1)
						{
							Mat a;
							a=minEnergy.rowRange(row-1,row).colRange(col-1,col+1);
							double minVal=0;
							Point minLoc;
							minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
							minEnergy.at<int>(row,col)+=minVal;
							seamPath[row][col].at<int>(0,0)=row-1+minLoc.y;
							seamPath[row][col].at<int>(0,1)=col-1+minLoc.x;
						} 
						else
						{
							Mat a;
							a=minEnergy.rowRange(row-1,row).colRange(col-1,col+2);
							double minVal=0;
							Point minLoc;
							minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
							minEnergy.at<int>(row,col)+=minVal;
							seamPath[row][col].at<int>(0,0)=row-1+minLoc.y;
							seamPath[row][col].at<int>(0,1)=col-1+minLoc.x;
						}
					}
				}
			}
		}
		return minEnergy;
	} 
	else
	{
		if (dir==0)
		{
		} 
		else
		{
		}
		return minEnergy;
	}
}

void modifiedSeamCarving::changePos(int dir , int end,int row, int col)
{
	if (dir==0) //水平的seam
	{
		if (end==0)
		{
			Mat origin,destination;
			origin=tempEn.rowRange(1,row+1).colRange(col,col+1);
			destination=tempEn.rowRange(0,row).colRange(col,col+1);
			origin.copyTo(destination);
			tempEn.at<int>(row,col)=100000;
			origin=tempMiss.rowRange(1,row+1).colRange(col,col+1);
			destination=tempMiss.rowRange(0,row).colRange(col,col+1);
			origin.copyTo(destination);
			tempMiss.at<uchar>(row,col)=255;
			origin=tempSeamResult.rowRange(1,row+1).colRange(col,col+1);
			destination=tempSeamResult.rowRange(0,row).colRange(col,col+1);
			origin.copyTo(destination);
			tempSeamResult.at<uchar>(row,col)=0;
			origin=tempModifiedOriginalImg.rowRange(1,row+1).colRange(col,col+1);
			destination=tempModifiedOriginalImg.rowRange(0,row).colRange(col,col+1);
			origin.copyTo(destination);
			tempModifiedOriginalImg.at<Vec3b>(row,col)=Vec3b(0,0,255);
		} 
		else
		{
			Mat origin,destination;
			origin=tempEn.rowRange(row,end).colRange(col,col+1);
			destination=tempEn.rowRange(row+1,end+1).colRange(col,col+1);
			origin.copyTo(destination);
			tempEn.at<int>(row,col)=100000;
			origin=tempMiss.rowRange(row,end).colRange(col,col+1);
			destination=tempMiss.rowRange(row+1,end+1).colRange(col,col+1);
			origin.copyTo(destination);
			tempMiss.at<uchar>(row,col)=255;
			origin=tempSeamResult.rowRange(row,end).colRange(col,col+1);
			destination=tempSeamResult.rowRange(row+1,end+1).colRange(col,col+1);
			origin.copyTo(destination);
			tempSeamResult.at<uchar>(row,col)=0;
			origin=tempModifiedOriginalImg.rowRange(row,end).colRange(col,col+1);
			destination=tempModifiedOriginalImg.rowRange(row+1,end+1).colRange(col,col+1);
			origin.copyTo(destination);
			tempModifiedOriginalImg.at<Vec3b>(row,col)=Vec3b(0,0,255);
		}
	} 
	else
	{
		if (end==0)
		{
			Mat origin,destination;
			origin=tempEn.rowRange(row,row+1).colRange(1,col+1);
			destination=tempEn.rowRange(row,row+1).colRange(0,col);
			origin.copyTo(destination);
			tempEn.at<int>(row,col)=100000;
			origin=tempMiss.rowRange(row,row+1).colRange(1,col+1);
			destination=tempMiss.rowRange(row,row+1).colRange(0,col);
			origin.copyTo(destination);
			tempMiss.at<uchar>(row,col)=255;
			origin=tempSeamResult.rowRange(row,row+1).colRange(1,col+1);
			destination=tempSeamResult.rowRange(row,row+1).colRange(0,col);
			origin.copyTo(destination);
			tempSeamResult.at<uchar>(row,col)=0;
			origin=tempModifiedOriginalImg.rowRange(row,row+1).colRange(1,col+1);
			destination=tempModifiedOriginalImg.rowRange(row,row+1).colRange(0,col);
			origin.copyTo(destination);
			tempModifiedOriginalImg.at<Vec3b>(row,col)=Vec3b(0,0,255);
		} 
		else
		{
			Mat origin,destination;
			origin=tempEn.rowRange(row,row+1).colRange(col,end);
			destination=tempEn.rowRange(row,row+1).colRange(col+1,end+1);
			origin.copyTo(destination);
			tempEn.at<int>(row,col)=100000;
			origin=tempMiss.rowRange(row,row+1).colRange(col,end);
			destination=tempMiss.rowRange(row,row+1).colRange(col+1,end+1);
			origin.copyTo(destination);
			tempMiss.at<uchar>(row,col)=255;
			origin=tempSeamResult.rowRange(row,row+1).colRange(col,end);
			destination=tempSeamResult.rowRange(row,row+1).colRange(col+1,end+1);
			origin.copyTo(destination);
			tempSeamResult.at<uchar>(row,col)=0;
			origin=tempModifiedOriginalImg.rowRange(row,row+1).colRange(col,end);
			destination=tempModifiedOriginalImg.rowRange(row,row+1).colRange(col+1,end+1);
			origin.copyTo(destination);
			tempModifiedOriginalImg.at<Vec3b>(row,col)=Vec3b(0,0,255);
		}
	}
}

void modifiedSeamCarving::optimalSeam(bool line)
{
	seamPath.clear();
	Mat tempLen(1,longestSeam.size(),CV_32SC1); //儲存所有seam的長度
	for (int i=0;i<longestSeam.size();i++)
	{
		tempLen.at<int>(0,i)=longestSeam[i].at<int>(0,2);
	}
	double maxVal=0;
	Point maxLoc; //x代表col,y代表row
	minMaxLoc(tempLen,NULL,&maxVal,NULL,&maxLoc);
	if (longestSeam[maxLoc.x].at<int>(3)==0) //水平的Seam
	{
		//missingRegion.copyTo(seamResult);
		Mat e,m,s,o,a,minEn;
		e=energyMap.colRange(longestSeam[maxLoc.x].at<int>(1),longestSeam[maxLoc.x].at<int>(1)+longestSeam[maxLoc.x].at<int>(2));
		m=missingRegion.colRange(longestSeam[maxLoc.x].at<int>(1),longestSeam[maxLoc.x].at<int>(1)+longestSeam[maxLoc.x].at<int>(2));
		s=seamResult.colRange(longestSeam[maxLoc.x].at<int>(1),longestSeam[maxLoc.x].at<int>(1)+longestSeam[maxLoc.x].at<int>(2));
		o=modifiedOriginalImg.colRange(longestSeam[maxLoc.x].at<int>(1),longestSeam[maxLoc.x].at<int>(1)+longestSeam[maxLoc.x].at<int>(2));
		e.copyTo(tempEn);
		m.copyTo(tempMiss);
		s.copyTo(tempSeamResult);
		o.copyTo(tempModifiedOriginalImg);
		tempEn=Img8Uto32S(tempEn);
		for (int i=0;i<tempMiss.rows;i++)
		{
			for (int j=0;j<tempMiss.cols;j++)
			{
				if (tempMiss.at<uchar>(i,j)==0)
				{
					tempEn.at<int>(i,j)=100000;
				}
			}
		}
		minEn=getCumulatedEnergy(true,tempEn,0);
		//SaveMatInfo("output/minEn",minEn);
		a=minEn.colRange(minEn.cols-1,minEn.cols);
		double minVal=0;
		Point minLoc;
		minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
		int row=minLoc.y,col=minEn.cols-1;
		changePos(0,longestSeam[maxLoc.x].at<int>(0),row,col);
		for (int i=0;i<minEn.cols-1;i++)
		{
			//SaveMatInfo("output/path",seamPath[row][col]);
			int tempRow=row,tempCol=col;
			row=seamPath[tempRow][tempCol].at<int>(0,0);
			col=seamPath[tempRow][tempCol].at<int>(0,1);
			changePos(0,longestSeam[maxLoc.x].at<int>(0),row,col);
		}
		tempEn.copyTo(e);
		tempMiss.copyTo(m);
		tempSeamResult.copyTo(s);
		tempModifiedOriginalImg.copyTo(o);
		string name;
		name="output/"+fileName+"/"+"modifiedMissingRegion.bmp";
		imwrite(name, missingRegion);
		name="output/"+fileName+"/"+"seamResult.bmp";
		imwrite(name, seamResult);
		name="output/"+fileName+"/"+"seamResultInOrigin.bmp";
		imwrite(name, modifiedOriginalImg);
	} 
	else
	{
		//missingRegion.copyTo(seamResult);
		Mat e,m,s,o,a,minEn;
		e=energyMap.rowRange(longestSeam[maxLoc.x].at<int>(0),longestSeam[maxLoc.x].at<int>(0)+longestSeam[maxLoc.x].at<int>(2));
		m=missingRegion.rowRange(longestSeam[maxLoc.x].at<int>(0),longestSeam[maxLoc.x].at<int>(0)+longestSeam[maxLoc.x].at<int>(2));
		s=seamResult.rowRange(longestSeam[maxLoc.x].at<int>(0),longestSeam[maxLoc.x].at<int>(0)+longestSeam[maxLoc.x].at<int>(2));
		o=modifiedOriginalImg.rowRange(longestSeam[maxLoc.x].at<int>(0),longestSeam[maxLoc.x].at<int>(0)+longestSeam[maxLoc.x].at<int>(2));
		e.copyTo(tempEn);
		m.copyTo(tempMiss);
		s.copyTo(tempSeamResult);
		o.copyTo(tempModifiedOriginalImg);
		tempEn=Img8Uto32S(tempEn);
		for (int i=0;i<tempMiss.rows;i++)
		{
			for (int j=0;j<tempMiss.cols;j++)
			{
				if (tempMiss.at<uchar>(i,j)==0)
				{
					tempEn.at<int>(i,j)=100000;
				}
			}
		}
		minEn=getCumulatedEnergy(true,tempEn,1);
		//SaveMatInfo("output/minEn",minEn);
		a=minEn.rowRange(minEn.rows-1,minEn.rows);
		double minVal=0;
		Point minLoc;
		minMaxLoc(a,&minVal,NULL,&minLoc,NULL);
		int row=minEn.rows-1,col=minLoc.x;
		changePos(1,longestSeam[maxLoc.x].at<int>(1),row,col);
		for (int i=0;i<minEn.rows-1;i++)
		{
			//SaveMatInfo("output/path",seamPath[row][col]);
			int tempRow=row,tempCol=col;
			row=seamPath[tempRow][tempCol].at<int>(0,0);
			col=seamPath[tempRow][tempCol].at<int>(0,1);
			changePos(1,longestSeam[maxLoc.x].at<int>(1),row,col);
		}
		tempEn.copyTo(e);
		tempMiss.copyTo(m);
		tempSeamResult.copyTo(s);
		tempModifiedOriginalImg.copyTo(o);
		string name;
		name="output/"+fileName+"/"+"modifiedMissingRegion.bmp";
		imwrite(name, missingRegion);
		name="output/"+fileName+"/"+"seamResult.bmp";
		imwrite(name, seamResult);
		name="output/"+fileName+"/"+"seamResultInOrigin.bmp";
		imwrite(name, modifiedOriginalImg);
	}
}