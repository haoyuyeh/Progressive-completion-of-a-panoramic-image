#include "stdafx.h"
#include "examplarBasedIC.h"
using namespace cv;
using namespace std;

void examplarBasedIC::initial( string filename, vector<patchPBP> candi, Mat input, Mat mask, int patchS )
{
	fileName =filename;
	imgH = input.rows;
	imgW = input.cols;
	mask.copyTo( maskImg );
	mask.copyTo( confidenceMap );
	confidenceMap = Mat( imgH, imgW, CV_32FC1, Scalar( 0.0 ) );
	confidenceMap.setTo( 1.0, maskImg );
	// 根據mask清空input
	vector<Mat> ch;
	split( input, ch );
	for ( int i = 0; i < imgH; i++ )
	{
		for ( int j = 0; j < imgW; j++ )
		{
			if ( maskImg.at<uchar>( i, j ) == 0 )
			{
				ch[ 0 ].at<uchar>( i, j ) = 0;
				ch[ 1 ].at<uchar>( i, j ) = 0;
				ch[ 2 ].at<uchar>( i, j ) = 0;
			}
		}
	}
	merge( ch, inputImg );
	namedWindow( "original image", CV_WINDOW_AUTOSIZE);
	imshow("original image",inputImg);
	inputImg.copyTo( SSDInput );
	cvtColor( SSDInput, SSDInput, CV_BGR2HLS );
	candidates = candi;
	patchSize = patchS;	
	gap = patchSize / 2;
	pixelNum = imgH * imgW;
	missingPixelNum = pixelNum - countNonZero( maskImg );
}

void examplarBasedIC::getFillFront()
{ 
	vector<fillFront>().swap( contourOfTargetRegion ); 
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat temp;
	maskImg.copyTo( temp );
	/// Find contours
	findContours( temp, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0) );
	int contourNum = contours.size(), c=0;
	for ( int j = 0; j < contourNum; j++ )
	{
		pointNum = contours[ j ].size();
		for ( int i = 0; i < pointNum; i++ )
		{
			fillFront element( contours[ j ][ i ] );
			contourOfTargetRegion.push_back( element );
  			/*if (contourNum == 2)
			{
				if (j==0)
				{
					circle( inputImg, contourOfTargetRegion[ c ].pos, 1,Scalar(0,0,255),-1 );
				} 
				else if(j==1)
				{
					circle( inputImg, contourOfTargetRegion[ c ].pos, 1,Scalar(0,255,255),-1 );
					namedWindow( "output", CV_WINDOW_NORMAL);
					imshow("output", inputImg );
					cvWaitKey(0);
				}
				else
				{
					circle( inputImg, contourOfTargetRegion[ c ].pos, 1,Scalar(255,0,255),-1 );
				}
				
			}
			c++;*/
		}
	}
}

void SaveMatInfoIC(char *F_Name,Mat mat) //存Mat資料(TXT)
{
	//char filename[100];
	//strcpy(filename,F_Name);
	//strcat(filename,".txt");
	FILE *file=fopen( F_Name,"w");
	for(int i=0;i<mat.rows;i++)
	{
		for(int j=0;j<mat.cols;j++)
		{
			fprintf(file,"%f ",mat.at<float>(i,j));
		}
		fprintf(file,"\n");
	}
	fclose(file);
}

float examplarBasedIC::setNodeCp( Mat t )
{
	int row = t.rows, col = t.cols;
	int totalPixel = row * col;
	float confidenceValue = 0.0;
	for ( int i = 0; i < row; i++ )
	{
		for ( int j = 0; j < col; j++ )
		{
			confidenceValue += t.at<float>( i,j );
		}
	}
	confidenceValue /= (float)totalPixel;
	return confidenceValue;
}

void examplarBasedIC::getPriority()
{
	// get gradient
	Mat gray, grad_x, grad_y, gradValue( inputImg.rows, inputImg.cols, CV_32F , Scalar( 0 ) );
	cvtColor( inputImg, gray, CV_BGR2GRAY  );
	Scharr( gray, grad_x, CV_32F, 1, 0 );
	Scharr( gray, grad_y, CV_32F, 0, 1 );
	// 需要去掉因為未知區域造成的不正確gradient值
	for ( int i = 0; i < imgH; i++ )
	{
		for ( int j = 0; j < imgW; j++ )
		{
			if ( i == 0 )
			{
				if ( j == 0 )
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i, i + 3 ).colRange( j, j + 3 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
				else if ( j == ( imgW - 1 ) )
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i, i + 3 ).colRange( j-2, j + 1 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
				else
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i, i + 3 ).colRange( j - 1, j + 2 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
			}
			else if ( i == ( imgH - 1 ) )
			{
				if ( j == 0 )
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i - 2, i + 1 ).colRange( j, j + 3 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
				else if ( j == ( imgW - 1 ) )
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i - 2, i + 1 ).colRange( j-2, j + 1 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
				else
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i - 2, i + 1 ).colRange( j - 1, j + 2 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
			}
			else
			{
				if ( j == 0 )
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i - 1, i +2 ).colRange( j, j + 3 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
				else if ( j == ( imgW - 1 ) )
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i - 1, i + 2 ).colRange( j-2, j + 1 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
				else
				{
					if ( checkPatchOverlapMissing( maskImg.rowRange( i - 1, i + 2 ).colRange( j - 1, j + 2 ).clone() ) )
					{
						grad_x.at<float>( i, j ) = 0.0;
						grad_y.at<float>( i, j ) = 0.0;
					}
				}
			}			
		}
	}
	/*char fName[200];
	sprintf(fName, "output/grad_x.txt");
	SaveMatInfoIC(fName, grad_x);
	sprintf(fName, "output/grad_y.txt");
	SaveMatInfoIC(fName, grad_y);*/
	for ( int i = 0; i < imgH; i++ )
	{
		for ( int j = 0; j < imgW; j++ )
		{
			gradValue.at<float>( i, j ) = sqrtf( ( grad_x.at<float>( i, j ) * grad_x.at<float>( i, j ) ) + ( grad_y.at<float>( i, j ) * grad_y.at<float>( i, j ) ) );
		}
	}
	/*char fName[200];
	sprintf(fName, "output/grad_value.txt");
	SaveMatInfoIC(fName, gradValue);*/
	int i = 0;
	while( i < pointNum )
	{
		/////calculate Cp/////
		int upRow, lowRow, upCol, lowCol, x = contourOfTargetRegion[ i ].pos.x, y = contourOfTargetRegion[ i ].pos.y;
		// patch range
		if ( x - gap <= 0 ) // 左限
		{
			lowCol = 0;
			//continue;
		}
		else
		{
			lowCol = x - gap;
		}
		if ( x + gap >= imgW ) // 右限
		{
			upCol = imgW;
			//continue;
		}
		else
		{
			upCol = x + gap;
		}
		if ( y - gap <= 0 ) // 上限
		{
			lowRow = 0;
			//continue;
		}
		else
		{
			lowRow = y - gap;
		}
		if ( y + gap >= imgH )  // 下限
		{
			upRow = imgH;
			//continue;
		}
		else
		{
			upRow = y + gap;
		}
		Mat target, tMask;
		maskImg.rowRange( lowRow, upRow ).colRange( lowCol, upCol ).copyTo( tMask );
		confidenceMap.rowRange( lowRow, upRow ).colRange( lowCol, upCol ).copyTo( target );
		contourOfTargetRegion[ i ].Cp = setNodeCp( target );
		// 檢查target是否存在未知區域，沒有的話就去掉不做
		if ( ( ( tMask.rows * tMask.cols ) - countNonZero( tMask ) ) == 0 )
		{
			contourOfTargetRegion.erase( contourOfTargetRegion.begin()+i );
			vector<fillFront>( contourOfTargetRegion ).swap( contourOfTargetRegion );
			pointNum = contourOfTargetRegion.size();
			continue;
		}
		/////calculate Dp/////
		// set Np
		Mat Np( 1, 2, CV_32F ), Ip( 1, 2, CV_32F );
		int dx = 0, dy = 0;
		if ( i == ( pointNum - 1 ) )
		{
			dx = contourOfTargetRegion[ i ].pos.x - contourOfTargetRegion[ i - 2 ].pos.x;
			dy = contourOfTargetRegion[ i ].pos.y - contourOfTargetRegion[ i - 2 ].pos.y;			
		} 
		else if( i ==0 )
		{
			dx = contourOfTargetRegion[ i + 2 ].pos.x - contourOfTargetRegion[ i  ].pos.x;
			dy = contourOfTargetRegion[ i + 2 ].pos.y - contourOfTargetRegion[ i  ].pos.y;
		}
		else
		{
			dx = contourOfTargetRegion[ i + 1 ].pos.x - contourOfTargetRegion[ i - 1 ].pos.x;
			dy = contourOfTargetRegion[ i + 1 ].pos.y - contourOfTargetRegion[ i - 1 ].pos.y;
		}
		float normal = sqrtf( ( -dy * -dy ) + ( dx * dx ) );
		if ( normal )
		{
			Np.at<float>( 0, 0 ) = (float)-dy / normal;
			Np.at<float>( 0, 1 ) = (float)dx / normal;
		}
		else
		{
			Np.at<float>( 0, 0 ) = 0.0;
			Np.at<float>( 0, 1 ) = 0.0;
		}
		//set Ip		
		target.~Mat();
		gradValue.rowRange( lowRow, upRow ).colRange( lowCol, upCol ).clone().copyTo( target );
		Point maxLoc;
		minMaxLoc( target, NULL, NULL, NULL, &maxLoc );
		float a,b;
		b = grad_y.at<float>( lowRow + maxLoc.y, lowCol + maxLoc.x );
		a = grad_x.at<float>( lowRow + maxLoc.y, lowCol + maxLoc.x );
		normal = sqrtf( ( a * a ) + ( b * b ) );
		if ( normal )
		{
			Ip.at<float>( 0, 0 ) = -b / normal;
			Ip.at<float>( 0, 1 ) = a / normal;
		} 
		else
		{
			Ip.at<float>( 0, 0 ) = 0;
			Ip.at<float>( 0, 1 ) = 0;
		}

		double c = abs( Np.dot( Ip ) );
		contourOfTargetRegion[ i ].Dp = (float)c / 255.0;
		// set priority
		contourOfTargetRegion[ i ].priority = contourOfTargetRegion[ i ].Cp * contourOfTargetRegion[ i ].Dp;
		i++;
	}	
}

bool examplarBasedIC::checkPatchOverlapMissing(Mat mask)
{
	// 假如回傳true，表示patch有包含missing region
	bool overLap=false;
	if ( countNonZero(mask) != (mask.rows * mask.cols))
	{
		overLap=true;
		return overLap;
	}
	else
	{
		return overLap;
	}
}

float examplarBasedIC::SSD(Mat target, Mat candidate, Mat mask)
{
	int row = target.rows, col = target.cols;
	Mat tRGB, cRGB, tRGBch0, tRGBch1, tRGBch2, cRGBch0, cRGBch1, cRGBch2, tHLSch0, tHLSch1, tHLSch2, cHLSch0, cHLSch1, cHLSch2, tAvgR( row, col, CV_32FC1, Scalar(0) ), tAvgG( row, col, CV_32FC1, Scalar(0) ), tAvgI( row, col, CV_32FC1, Scalar(0) ), cAvgR( row, col, CV_32FC1, Scalar(0) ), cAvgG( row, col, CV_32FC1, Scalar(0) ), cAvgI( row, col, CV_32FC1, Scalar(0) );
	vector<Mat> tRGBch, cRGBch, tHLSch, cHLSch;
	cvtColor( target, tRGB, CV_HLS2BGR );
	cvtColor( candidate, cRGB, CV_HLS2BGR );
	split(tRGB, tRGBch);
	tRGBch[ 0 ].convertTo( tRGBch0, CV_32FC1 );
	tRGBch[ 1 ].convertTo( tRGBch1, CV_32FC1 );
	tRGBch[ 2 ].convertTo( tRGBch2, CV_32FC1 );
	split( cRGB, cRGBch );
	cRGBch[ 0 ].convertTo( cRGBch0, CV_32FC1 );
	cRGBch[ 1 ].convertTo( cRGBch1, CV_32FC1 );
	cRGBch[ 2 ].convertTo( cRGBch2, CV_32FC1 );
	split( target, tHLSch );
	tHLSch[ 0 ].convertTo( tHLSch0, CV_32FC1 );
	tHLSch[ 1 ].convertTo( tHLSch1, CV_32FC1 );
	tHLSch[ 2 ].convertTo( tHLSch2, CV_32FC1 );
	split( candidate, cHLSch );
	cHLSch[ 0 ].convertTo( cHLSch0, CV_32FC1 );
	cHLSch[ 1 ].convertTo( cHLSch1, CV_32FC1 );
	cHLSch[ 2 ].convertTo( cHLSch2, CV_32FC1 );
	tAvgR = tRGBch2 / ( tRGBch0 + tRGBch1 + tRGBch2 );
	tAvgG = tRGBch1 / ( tRGBch0 + tRGBch1 + tRGBch2 );
	tAvgI = tHLSch1 / 255.0;
	cAvgR = cRGBch2 / ( cRGBch0 + cRGBch1 + cRGBch2 );
	cAvgG = cRGBch1 / ( cRGBch0 + cRGBch1 + cRGBch2 );
	cAvgI = cHLSch1 / 255.0;
	return ( norm( tAvgR, cAvgR, NORM_L2, mask ) + norm( tAvgG, cAvgG, NORM_L2, mask ) + norm( tAvgI, cAvgI, NORM_L2, mask ) ) / (float)countNonZero(mask);
}

void examplarBasedIC::fillPatch()
{
	// 切patch
	/*if ( candidates.size() == 0 )
	{
	int row = temp.rows - patchSize + 1, col = temp.cols - patchSize + 1;
	for ( int i = 0; i < row; i++ )
	{
	for ( int j = 0; j < col; j++ )
	{
	if ( !checkPatchOverlapMissing( maskImg.rowRange( i, i + patchSize ).colRange( j, j + patchSize ).clone() ) )
	{
	candidates.push_back( Point( j, i ) );
	}
	}
	}
	patchNum = candidates.size();
	}*/	
	// find max priority point
	int upRow, lowRow, upCol, lowCol, x, y;
	fillFront maxPriority;
	for ( int i = 0; i < pointNum; i++ )
	{
		if ( contourOfTargetRegion[ i ].priority >= maxPriority.priority )
		{
			maxPriority = contourOfTargetRegion[ i ];
		}
	}
	// patch range
	x = maxPriority.pos.x;
	y = maxPriority.pos.y;
	if ( x - gap <= 0 ) // 左限
	{
		lowCol = 0;
	}
	else
	{
		lowCol = x - gap;
	}
	if ( x + gap >= imgW ) // 右限
	{
		upCol = imgW;
	}
	else
	{
		upCol = x + gap;
	}
	if ( y - gap <= 0 ) // 上限
	{
		lowRow = 0;
	}
	else
	{
		lowRow = y - gap;
	}
	if ( y + gap >= imgH )  // 下限
	{
		upRow = imgH;
	}
	else
	{
		upRow = y + gap;
	}
	// find best match patch
	float tempMax = 10000000.0;
	Point bestPatch;
	patchNum = candidates.size();
	int tempRow = upRow - lowRow, tempCol = upCol - lowCol;
	for ( int i = 0; i < patchNum; i++ )
	{		
		float ssd = SSD( SSDInput.rowRange( lowRow, upRow ).colRange( lowCol, upCol ).clone(), SSDInput.rowRange( candidates[ i ].patchPos.y, candidates[ i ].patchPos.y + tempRow ).colRange( candidates[ i ].patchPos.x, candidates[ i ].patchPos.x + tempCol ).clone(), maskImg.rowRange( lowRow, upRow ).colRange( lowCol, upCol ).clone() );
		if ( ssd <= tempMax )
		{
			tempMax = ssd;
			bestPatch = candidates[ i ].patchPos;
		}
	}
	// fill missing region
	Mat candidate;
	vector<Mat> tch, cch;
	inputImg.rowRange( bestPatch.y, bestPatch.y + patchSize ).colRange( bestPatch.x, bestPatch.x + patchSize ).clone().copyTo( candidate );
	split( inputImg, tch );
	split( candidate, cch );
	for ( int i = 0; i < tempRow; i++ )
	{
		for ( int j = 0; j < tempCol; j++ )
		{
			if ( maskImg.at<uchar>( lowRow + i, lowCol + j ) == 0 )
			{
				tch[ 0 ].at<uchar>( lowRow + i, lowCol + j ) = cch[ 0 ].at<uchar>( i, j );
				tch[ 1 ].at<uchar>( lowRow + i, lowCol + j ) = cch[ 1 ].at<uchar>( i, j );
				tch[ 2 ].at<uchar>( lowRow + i, lowCol + j ) = cch[ 2 ].at<uchar>( i, j );
				maskImg.at<uchar>( lowRow + i, lowCol + j ) = 255;
				confidenceMap.at<float>( lowRow + i, lowCol + j ) = maxPriority.Cp;
			}
		}
	}
	merge( tch, outputImg );
	inputImg.~Mat();
	outputImg.copyTo( inputImg );	
	SSDInput.~Mat();
	inputImg.copyTo( SSDInput );
	cvtColor( SSDInput, SSDInput, CV_BGR2HLS );
	/*namedWindow( "output", CV_WINDOW_NORMAL);
	imshow("output", inputImg );
	namedWindow( "maskImg", CV_WINDOW_NORMAL);
	imshow("maskImg",maskImg);
	cvWaitKey(0);*/
}

void examplarBasedIC::run()
{
	char fName[ 200 ];
	while( missingPixelNum )
	{
		/*if (missingPixelNum == 29)
		{
		namedWindow( "output", CV_WINDOW_NORMAL);
		imshow("output", outputImg );
		namedWindow( "maskImg", CV_WINDOW_NORMAL);
		imshow("maskImg",maskImg);
		cvWaitKey(0);
		}*/
		cout << "missing pixel rate = " << (float)missingPixelNum / (float)pixelNum << "\r";
		getFillFront();
		getPriority();
		/*for ( int i = 0; i < pointNum; i++ )
		{
		circle( inputImg, contourOfTargetRegion[ i ].pos, 1,Scalar(0,0,255),-1 );
		namedWindow( "output", CV_WINDOW_NORMAL);
		imshow("output", inputImg );
		cvWaitKey(0);
		}*/
		fillPatch();
		missingPixelNum = pixelNum - countNonZero( maskImg );
	}	
	sprintf(fName, "C:/Completion Result/%s/CriminisiFinalOutput.bmp",fileName.c_str() );
	imwrite( fName, outputImg );
}