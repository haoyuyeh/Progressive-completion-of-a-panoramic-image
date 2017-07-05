#include "stdafx.h"
#include "modifiedPriorityBP.h"
using namespace cv;
using namespace std;

Mat modifiedPriorityBP::superimposeBoundary( Mat r, Mat m )
{	
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	/// Find contours
	findContours( m, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, Point(0, 0) );
	int pointNum = 0;
	pointNum = contours[ 0 ].size();
	for ( int i = 0; i < pointNum; i++ )
	{
		circle( r, contours[ 0 ][ i ], 1,Scalar(0,255,255),-1 );
	}
	return r;
}

void modifiedPriorityBP::useCriminisiMethod()
{
	// generate candidates
	produceMrfNode();
	setMrfNodeNeighbor();
	getBoundaryNodes();
	vector<vector<nodePBP>>().swap( mrfNodes );
	setBoundaryNodeNeighbor();		
	vector<patchPBP>().swap( candidates ); 
	getCandidatePatchesFromBoundaryNodes();
	int num = tempCandidate.size();
	string Name;
	Name="C:/Completion Result/"+fileName+"/"+"data.txt";
	FILE *file=fopen(Name.c_str(),"a");
	fprintf(file,"patchSize=%d\n", patchSize);
	fprintf(file,"candidateNum=%d\n", num);
	fclose(file);
	for ( int i = 0; i < num; i++ )
	{
		patchPBP patch( tempCandidate[ i ] );
		candidates.push_back(patch);	
		candidateMap.at<uchar>( tempCandidate[ i ].y, tempCandidate[ i ].x ) = 255;
	}
	// 圖示candidate的位置
	Mat te;
	inputImg.copyTo( te );
	for ( int i = 0; i < imgH; i++ )
	{
		for ( int j = 0; j < imgW; j++ )
		{
			if ( candidateMap.at<uchar>( i, j ) == 255 )
			{
				circle(te, Point( j, i ),1,Scalar(255, 255, 255 ),-1);
				/*namedWindow( "candidateMap", CV_WINDOW_AUTOSIZE);
				imshow("candidateMap",te);
				cvWaitKey(0);*/
			}
		}
	}
	char fName[ 200 ];
	sprintf(fName, "C:/Completion Result/%s/candidateMap.bmp",fileName.c_str() );
	imwrite( fName, te );

	examplarBasedIC ic;
	ic.initial( fileName, candidates, inputImg, maskImg, patchSize );
	ic.run();
}

void modifiedPriorityBP::run()
{
	char fName[ 200 ];
	produceMrfNode();
	setMrfNodeNeighbor();
	getBoundaryNodes();
	vector<vector<nodePBP>>().swap( mrfNodes );
	setBoundaryNodeNeighbor();		
	vector<patchPBP>().swap( candidates ); 
	getCandidatePatchesFromBoundaryNodes();
	int num = tempCandidate.size();
	for ( int i = 0; i < num; i++ )
	{
		patchPBP patch( tempCandidate[ i ] );
		candidates.push_back(patch);	
		candidateMap.at<uchar>( tempCandidate[ i ].y, tempCandidate[ i ].x ) = 255;
	}
	// 圖示candidate的位置
	Mat te;
	inputImg.copyTo( te );
	/*string name;
	name = "C:/Completion Result/"+fileName+"/"+"boundaryNodes.bmp";
	te = imread(name);*/
	for ( int i = 0; i < imgH; i++ )
	{
		for ( int j = 0; j < imgW; j++ )
		{
			if ( candidateMap.at<uchar>( i, j ) == 255 )
			{
				circle(te, Point( j, i ),1,Scalar(255, 255, 255 ),-1);
			}
		}
	}
	sprintf(fName, "C:/Completion Result/%s/candidateMap.bmp",fileName.c_str() );
	imwrite( fName, te );

	candiNum = candidates.size();
	getSSD0();

	int count = 1, currentMissingPixel;
	float restMissingPart, preMissing = 0;
	bool all = false;
	// 漸進式補圖
	while ( count )
	{
		// default mode: 太差不補
		currentMissingPixel = totalPixel - countNonZero( tempMaskImg );
		restMissingPart = (float)currentMissingPixel / (float)totalMissingPixel;	
		produceMrfNode();
		setMrfNodeNeighbor();
		getBoundaryNodes();
		vector<vector<nodePBP>>().swap( mrfNodes );
		setBoundaryNodeNeighbor();		
		getNodeInitialBeliefAndPriority();		
		priorityBP( false );
		sprintf(fName, "C:/Completion Result/%s/BPoutput%d.bmp",fileName.c_str(), count );
		fillMissingRegion( fName, true );	
		// 一旦發生所有點都沒有夠相近的patch，而造成都沒補的話，則patchSize減半後，繼續iteration。
		// 當patchSize = 8的時候， 發生所有點都沒有夠相近的patch，而造成都沒補的話，則做一輪全部補
		preMissing = restMissingPart;
		currentMissingPixel = totalPixel - countNonZero( tempMaskImg );
		restMissingPart = (float)currentMissingPixel / (float)totalMissingPixel;
		if ( preMissing == restMissingPart )
		{
			if ( patchSize == 8 )
			{
				produceMrfNode();
				setMrfNodeNeighbor();
				getBoundaryNodes();
				vector<vector<nodePBP>>().swap( mrfNodes );
				setBoundaryNodeNeighbor();		
				getNodeInitialBeliefAndPriority();
				priorityBP( true );
				sprintf(fName, "C:/Completion Result/%s/BPoutput%d.bmp",fileName.c_str(), count );
				fillMissingRegion( fName, true );	
			} 
			else
			{
				Mat temp;
				tempMaskImg.copyTo( temp );
				maskImg.copyTo( tempMaskImg );			 
				patchSize /=2;
				candidateRange = 1.5 * patchSize;
				// 代表patch overlap 7成
				gap = patchSize * 0.3;				
				produceMrfNode();
				setMrfNodeNeighbor();
				getBoundaryNodes();
				vector<vector<nodePBP>>().swap( mrfNodes );
				setBoundaryNodeNeighbor();		
				vector<patchPBP>().swap( candidates ); 
				getCandidatePatchesFromBoundaryNodes();
				int num = tempCandidate.size();
				for ( int i = 0; i < num; i++ )
				{
					patchPBP patch( tempCandidate[ i ] );
					candidates.push_back(patch);	
					candidateMap.at<uchar>( tempCandidate[ i ].y, tempCandidate[ i ].x ) = 255;
				}
				candiNum = candidates.size();
				getSSD0();
				temp.copyTo( tempMaskImg );
			}						
		}
		if ( countNonZero( tempMaskImg ) == totalPixel )
		{
			break;
		}
		count++;
	}
}

void modifiedPriorityBP::initial(string filename, Mat img, Mat mask, int pSize)
{
	fileName=filename;
	mask.setTo( 255, mask );
	mask.copyTo(maskImg);
	mask.copyTo( tempMaskImg );
	imgH=img.rows;
	imgW=img.cols;
	candidateMap = Mat( imgH, imgW, CV_8UC1, Scalar( 0 ) );
	confidentMap = Mat( imgH, imgW, CV_32FC1, Scalar( 0.0 ) );
	confidentMap.setTo( 1.0, mask );
	totalPixel = imgH * imgW;
	totalMissingPixel = totalPixel - countNonZero( maskImg );
	vector<Mat> inputCh;
	split( img, inputCh );
	// 根據maskImg把missingRegion部分清零
	for ( int i = 0; i < imgH; i++ )
	{
		for ( int j = 0; j < imgW; j++ )
		{
			if ( maskImg.at<uchar>( i, j ) == 0 )
			{				
				inputCh[ 0 ].at<uchar>( i, j ) = 0;
				inputCh[ 1 ].at<uchar>( i, j ) = 0;
				inputCh[ 2 ].at<uchar>( i, j ) = 0;				
			}
		}
	}
	merge( inputCh, inputImg );
	inputImg.copyTo( SSDImg );
	cvtColor( SSDImg, SSDImg, CV_BGR2HLS );
	patchSize=pSize;
	candidateRange = 1.5 * patchSize;
	// 代表patch overlap 7成
	gap = patchSize * 0.3;
	// 計算缺失區域大小和所占比例
	string fName;
	fName="C:/Completion Result/"+fileName+"/"+"data.txt";
	FILE *file=fopen(fName.c_str(),"w");
	fprintf(file,"missingRegionRatio(%d/%d) = %f\n", totalMissingPixel, totalPixel, (float)totalMissingPixel / (float)totalPixel );
	fclose(file);
}

void modifiedPriorityBP::produceMrfNode()
{
	cout << "produceMrfNode" << endl;
	int num=0; //代表已有的nodes排數
	bool shortRow=false; //避免剩下的距離不足一個patch size但是大於gap，而造成重複新增同一個點
	for (int i=0 ; i<imgH ; i+=gap) // row=point中的y
	{
		if (!shortRow)
		{
			mrfNodes.push_back(element);
			for (int j=0 ; j<imgW ; j+=gap) // col=point中的x
			{
				if ( (imgW-j-1) < patchSize && (imgH-i-1) < patchSize) // 剩餘部分不足一個patch
				{
					// insert a node
					nodePBP node( imgW-patchSize , imgH-patchSize);
					mrfNodes[num].push_back(node);
					break; //避免剩下的距離不足一個patch size但是大於gap，而造成重複新增同一個點
				} 
				else if( (imgW-j-1) < patchSize) 
				{
					nodePBP node( imgW-patchSize , i );
					mrfNodes[num].push_back(node);
					break; //避免剩下的距離不足一個patch size但是大於gap，而造成重複新增同一個點
				}
				else if( (imgH-i-1) < patchSize)
				{
					shortRow=true;
					nodePBP node( j , imgH-patchSize );
					mrfNodes[num].push_back(node);
					//break; //避免剩下的距離不足一個patch size但是大於gap，而造成重複新增同一個點
				}
				else
				{
					// insert a node
					nodePBP node( j , i );
					mrfNodes[num].push_back(node);
				}
			}
			num++;
		}		
	}
	checkNodeValid();
	// 紀錄mrfNode總數
	if ( mrfNodeNum == 0 )
	{
		Mat temp;
		inputImg.copyTo(temp);
		int nodeNum=0, row=mrfNodes.size(), col=0;
		for (int i=0 ; i<row ; i++)
		{
			col=mrfNodes[i].size();
			for (int j=0 ; j<col ; j++)
			{
				circle(temp,mrfNodes[i][j].nodePos,1,Scalar(0,0,255),-1);
				nodeNum++;
			}
		}
		mrfNodeNum = nodeNum;
		string a="C:/Completion Result/"+fileName+"/"+"mrfNodes.bmp";
		imwrite(a,temp);

		string fName;
		fName="C:/Completion Result/"+fileName+"/"+"data.txt";
		FILE *file=fopen(fName.c_str(),"a");
		fprintf(file,"mrfNodes=%d\n", nodeNum);
		fclose(file);
	}
}

bool modifiedPriorityBP::checkPatchOverlapMissing(Mat mask)
{
	// 假如回傳true，表示patch有包含missing region
	bool overLap=false;
	if ( countNonZero(mask) != (patchSize*patchSize))
	{
		overLap=true;
		return overLap;
	}
	else
	{
		return overLap;
	}
}

void modifiedPriorityBP::checkNodeValid()
{
	int row=mrfNodes.size(), col, num=-1; //需要去掉的node個數
	for (int i=0 ; i<row ; i++)
	{
		num=-1;
		col=mrfNodes[i].size();
		for (int j=0 ; j<col ; j++)
		{
			if (!checkPatchOverlapMissing( tempMaskImg.rowRange( mrfNodes[i][j].nodePos.y, mrfNodes[i][j].nodePos.y+patchSize).colRange( mrfNodes[i][j].nodePos.x, mrfNodes[i][j].nodePos.x+patchSize).clone() ) ) //patch不包含缺失區域，所以從MRF Nodes中去掉
			{
				num++;
				swap(mrfNodes[i][num] , mrfNodes[i][j]);				
			}
		}
		if (num!=-1)
		{
			mrfNodes[i].erase(mrfNodes[i].begin() , mrfNodes[i].begin()+num+1); //erase(first, last)，刪除時不包含last那個元素
		}		
	}
	vector<vector<nodePBP>>(mrfNodes).swap(mrfNodes);
}

void modifiedPriorityBP::setMrfNodeNeighbor()
{
	cout << "setMrfNodeNeighbor" << endl;
	//利用兩點的座標來判斷是否有鄰居，且鄰居的位置是哪裡
	int row=mrfNodes.size(), col;
	for (int i=0 ; i<row ; i++)
	{
		col=mrfNodes[i].size();
		for (int j=0 ; j<col ; j++)
		{
			//up
			if ( mrfNodes[i][j].nodePos.y != 0 )
			{
				int pCol=mrfNodes[i-1].size();
				for (int a=0 ; a<pCol ; a++)
				{
					if ( mrfNodes[i-1][a].nodePos.x == mrfNodes[i][j].nodePos.x )
					{
						mrfNodes[i][j].upperNeighbor=&mrfNodes[i-1][a];
					} 
				}
			} 
			//down
			if ( mrfNodes[i][j].nodePos.y != (imgH-patchSize) )
			{
				int nCol=mrfNodes[i+1].size();
				for (int a=0 ; a<nCol ; a++)
				{
					if ( mrfNodes[i+1][a].nodePos.x == mrfNodes[i][j].nodePos.x )
					{
						mrfNodes[i][j].lowerNeighbor=&mrfNodes[i+1][a];
					} 
				}
			} 
			//left
			if ( mrfNodes[i][j].nodePos.x != 0 )
			{
				int tCol=mrfNodes[i].size();
				for (int a=0 ; a<tCol ; a++)
				{
					if ( abs( mrfNodes[i][j].nodePos.x - mrfNodes[i][a].nodePos.x ) <= gap )
					{
						if ( mrfNodes[i][a].nodePos.x < mrfNodes[i][j].nodePos.x )
						{
							mrfNodes[i][j].leftNeighbor=&mrfNodes[i][a];
						}						
					} 
				}
			} 
			//right
			if ( mrfNodes[i][j].nodePos.x != (imgW-patchSize) )
			{
				int tCol=mrfNodes[i].size();
				for (int a=0 ; a<tCol ; a++)
				{
					if ( abs( mrfNodes[i][j].nodePos.x - mrfNodes[i][a].nodePos.x ) <= gap )
					{
						if ( mrfNodes[i][a].nodePos.x > mrfNodes[i][j].nodePos.x )
						{
							mrfNodes[i][j].rightNeighbor=&mrfNodes[i][a];
						}						
					} 
				}
			} 
		}
	}
}

void modifiedPriorityBP::getBoundaryNodes()
{
	cout << "getBoundaryNodes" << endl;
	// 重置boundary nodes
	vector<nodePBP>().swap( boundaryNodes );
	// 缺少鄰居的點有成為邊界點的可能性。特例為在影像四周的最外圍點，因此，需要額外判斷
	int row=mrfNodes.size(), col;
	for (int i=0 ; i<row ; i++)
	{
		col=mrfNodes[i].size();
		for (int j=0 ; j<col ; j++)
		{
			if ( mrfNodes[i][j].upperNeighbor == nullptr )
			{
				if ( mrfNodes[i][j].nodePos.y == 0 ) // 額外判斷
				{
					if ( mrfNodes[i][j].lowerNeighbor == nullptr )
					{
						boundaryNodes.push_back(mrfNodes[i][j]);
					}					
				}
				else
				{
					boundaryNodes.push_back(mrfNodes[i][j]);
				}			
			} 
			else if( mrfNodes[i][j].lowerNeighbor == nullptr )
			{
				if ( mrfNodes[i][j].nodePos.y == (imgH-patchSize) ) // 額外判斷
				{
					if ( mrfNodes[i][j].upperNeighbor == nullptr )
					{
						boundaryNodes.push_back(mrfNodes[i][j]);
					}					
				}
				else
				{
					boundaryNodes.push_back(mrfNodes[i][j]);
				}				
			}
			else if( mrfNodes[i][j].leftNeighbor == nullptr )
			{
				if ( mrfNodes[i][j].nodePos.x == 0 ) // 額外判斷
				{
					if ( mrfNodes[i][j].rightNeighbor == nullptr )
					{
						boundaryNodes.push_back(mrfNodes[i][j]);
					}					
				}
				else
				{
					boundaryNodes.push_back(mrfNodes[i][j]);
				}	
			}
			else if ( mrfNodes[i][j].rightNeighbor == nullptr )
			{
				if ( mrfNodes[i][j].nodePos.x == (imgW-patchSize) ) // 額外判斷
				{
					if ( mrfNodes[i][j].leftNeighbor == nullptr )
					{
						boundaryNodes.push_back(mrfNodes[i][j]);
					}					
				}
				else
				{
					boundaryNodes.push_back(mrfNodes[i][j]);
				}	
			}
		}
	}
	// 把boundary nodes的鄰居也加入成為boundary nodes

	vector<nodePBP> originalBoundaryNodes = boundaryNodes;
	boundaryNum = boundaryNodes.size();
	int x, y, obn = originalBoundaryNodes.size();
	bool add;
	for ( int i = 0; i < obn; i++ )
	{
		// up neighbor
		x = originalBoundaryNodes[ i ].nodePos.x;
		y = originalBoundaryNodes[ i ].nodePos.y - gap;
		if ( y >= 0 )
		{
			add = true;
			Point now( x, y );
			for ( int j = 0; j < boundaryNum; j++ )
			{
				if ( now == boundaryNodes[ j ].nodePos )
				{
					add = false;
					break;
				}
			}
			if ( add )
			{
				nodePBP p( x, y );
				boundaryNodes.push_back( p );
				boundaryNum++;
			}
		} 
		// down neighbor
		x = originalBoundaryNodes[ i ].nodePos.x;
		y = originalBoundaryNodes[ i ].nodePos.y + gap;
		if ( y <= ( imgH - patchSize - 1 ) )
		{
			add = true;
			Point now( x, y );
			for ( int j = 0; j < boundaryNum; j++ )
			{
				if ( now == boundaryNodes[ j ].nodePos )
				{
					add = false;
					break;
				}
			}
			if ( add )
			{
				nodePBP p( x, y );
				boundaryNodes.push_back( p );
				boundaryNum++;
			}
		} 
		// left neighbor
		x = originalBoundaryNodes[ i ].nodePos.x - gap;
		y = originalBoundaryNodes[ i ].nodePos.y;
		if ( x >= 0 )
		{
			add = true;
			Point now( x, y );
			for ( int j = 0; j < boundaryNum; j++ )
			{
				if ( now == boundaryNodes[ j ].nodePos )
				{
					add = false;
					break;
				}
			}
			if ( add )
			{
				nodePBP p( x, y );
				boundaryNodes.push_back( p );
				boundaryNum++;
			}
		} 
		// right neighbor
		x = originalBoundaryNodes[ i ].nodePos.x + gap;
		y = originalBoundaryNodes[ i ].nodePos.y;
		if ( x <= ( imgW - patchSize - 1 ) )
		{
			add = true;
			Point now( x, y );
			for ( int j = 0; j < boundaryNum; j++ )
			{
				if ( now == boundaryNodes[ j ].nodePos )
				{
					add = false;
					break;
				}
			}
			if ( add )
			{
				nodePBP p( x, y );
				boundaryNodes.push_back( p );
				boundaryNum++;
			}
		} 
	}
	Mat temp;
	//inputImg.copyTo( temp );
	string a="C:/Completion Result/"+fileName+"/"+"mrfNodes.bmp";
	temp = imread( a );
	string fName;
	fName = "C:/Completion Result/"+fileName+"/"+"boundaryNodes.bmp";
	boundaryNum = boundaryNodes.size();
	for (int i=0 ; i< boundaryNum; i++)
	{
		circle(temp,boundaryNodes[i].nodePos,1,Scalar(0,255,255),-1);
	}
	imwrite( fName,temp );
	fName="C:/Completion Result/"+fileName+"/"+"data.txt";
	FILE *file=fopen( fName.c_str(),"a" );
	fprintf(file,"boundaryNodes=%d\n", boundaryNum );
	fclose(file);
}

void modifiedPriorityBP::setBoundaryNodeNeighbor()
{
	cout << "setBoundaryNodeNeighbor" << endl;
	// 清空鄰居資訊
	for ( int i = 0; i < boundaryNum; i++ )
	{
		boundaryNodes[ i ].upperNeighbor = nullptr;
		boundaryNodes[ i ].lowerNeighbor = nullptr;
		boundaryNodes[ i ].leftNeighbor = nullptr;
		boundaryNodes[ i ].rightNeighbor = nullptr;
	}
	// 重置
	for ( int i = 0; i < boundaryNum; i++ )
	{
		boundaryNodes[ i ].alreadyVisited = false;
		boundaryNodes[ i ].beliefExist = false;
		boundaryNodes[ i ].committed = false;
	}
	//利用兩點的座標來判斷是否有鄰居，且鄰居的位置是哪裡
	for (int i=0 ; i<boundaryNum ; i++)
	{
		//up
		for ( int j=0; j < boundaryNum; j++ )
		{
			if ( boundaryNodes[ i ].nodePos.x == boundaryNodes[ j ].nodePos.x )
			{
				if ( ( abs( boundaryNodes[ i ].nodePos.y - boundaryNodes[ j ].nodePos.y ) <= gap ) && ( boundaryNodes[ j ].nodePos.y < boundaryNodes[ i ].nodePos.y ) )
				{
					boundaryNodes[ i ].upperNeighbor=&boundaryNodes[ j ];
				}						
			} 
		}
		//down
		for ( int j=0; j < boundaryNum; j++ )
		{
			if ( boundaryNodes[ i ].nodePos.x == boundaryNodes[ j ].nodePos.x )
			{
				if ( ( abs( boundaryNodes[ i ].nodePos.y - boundaryNodes[ j ].nodePos.y ) <= gap ) && ( boundaryNodes[ j ].nodePos.y > boundaryNodes[ i ].nodePos.y ) )
				{
					boundaryNodes[ i ].lowerNeighbor=&boundaryNodes[ j ];
				}						
			} 
		}
		//left
		for ( int j=0; j < boundaryNum; j++ )
		{
			if ( boundaryNodes[ i ].nodePos.y == boundaryNodes[ j ].nodePos.y )
			{
				if ( ( abs( boundaryNodes[ i ].nodePos.x - boundaryNodes[ j ].nodePos.x ) <= gap ) && ( boundaryNodes[ j ].nodePos.x < boundaryNodes[ i ].nodePos.x ) )
				{
					boundaryNodes[ i ].leftNeighbor=&boundaryNodes[ j ];
				}						
			} 
		}
		//right
		for ( int j=0; j < boundaryNum; j++ )
		{
			if ( boundaryNodes[ i ].nodePos.y == boundaryNodes[ j ].nodePos.y )
			{
				if ( ( abs( boundaryNodes[ i ].nodePos.x - boundaryNodes[ j ].nodePos.x ) <= gap ) && ( boundaryNodes[ j ].nodePos.x > boundaryNodes[ i ].nodePos.x ) )
				{
					boundaryNodes[ i ].rightNeighbor=&boundaryNodes[ j ];
				}						
			} 
		}		
	}	
}

void modifiedPriorityBP::validateNodeNeighbor()
{
	Mat temp;
	for (int i=0 ; i < boundaryNum; i++)
	{
		inputImg.copyTo(temp);
		circle( temp,boundaryNodes[ i ].nodePos,1,Scalar(0,0,255),-1); //紅
		if (  boundaryNodes[ i ].upperNeighbor != nullptr )
		{
			circle(temp,boundaryNodes[ i ].upperNeighbor->nodePos ,1,Scalar(255,0,0),-1); //藍
		}			
		if ( boundaryNodes[ i ].lowerNeighbor != nullptr )
		{
			circle(temp,boundaryNodes[ i ].lowerNeighbor->nodePos ,1,Scalar(0,255,0),-1); //綠
		}
		if ( boundaryNodes[ i ].leftNeighbor != nullptr )
		{
			circle(temp,boundaryNodes[ i ].leftNeighbor->nodePos,1,Scalar(0,255,255),-1); //黃
		}
		if ( boundaryNodes[ i ].rightNeighbor != nullptr )
		{
			circle(temp,boundaryNodes[ i ].rightNeighbor->nodePos,1,Scalar(255,0,255),-1); //紫
		}
		namedWindow( "mrfNodesNeighbor", CV_WINDOW_NORMAL);
		imshow("mrfNodesNeighbor",temp);
		cvWaitKey(0);		
	}
}

void modifiedPriorityBP::getCandidatePatchesFromWholeImg()
{
	vector<patchPBP>().swap( candidates );
	// 由全圖去找candidate
	for ( int i = 0 ; i < ( imgH - patchSize +1 ); i++ ) //y
	{
		for ( int j = 0; j < ( imgW - patchSize +1 ); j++ ) //x
		{		
			if ( !checkPatchOverlapMissing(maskImg.rowRange(i, i+patchSize).colRange(j, j+patchSize)))
			{
				patchPBP patch(Point(j, i));
				candidates.push_back(patch);						
			}			
		}
	}
	candiNum=candidates.size();
}

void modifiedPriorityBP::getCandidatePatchesFromBoundaryNodes()
{
	interval = cvRound( (float)patchSize / 10.0 );
	if ( interval == 0 )
	{
		interval = 1;
	}	
	for ( int i=0; i < boundaryNum; i++ )
	{
		cout << "getCandidatePatchesFromBoundaryNodes(" << i + 1 << "/" << boundaryNum << ")\r";
		// 設定某一節點的candidate範圍，2*candidateRange X 2*candidateRange
		int upperLX, upperLY, lowerRX, lowerRY; // candidate range的左上和右下點
		if ( ( boundaryNodes[i].nodePos.y - candidateRange ) < 0 )
		{
			upperLY=0;
		} 
		else
		{
			upperLY=boundaryNodes[i].nodePos.y-candidateRange;
		}
		if ( ( boundaryNodes[i].nodePos.y + candidateRange ) >= imgH )
		{
			lowerRY=imgH;
		} 
		else
		{
			lowerRY=boundaryNodes[i].nodePos.y+candidateRange;
		}
		if ( ( boundaryNodes[i].nodePos.x - candidateRange ) < 0 )
		{
			upperLX=0;
		} 
		else
		{
			upperLX=boundaryNodes[i].nodePos.x-candidateRange;
		}
		if ( ( boundaryNodes[i].nodePos.x + candidateRange ) >= imgW )
		{
			lowerRX=imgW;
		} 
		else
		{
			lowerRX=boundaryNodes[i].nodePos.x+candidateRange;
		}
		// get all patches for the node
		int candidatesNum=0;
		for ( int i=upperLY ; i<lowerRY ; i += interval ) //y
		{
			for ( int j=upperLX ; j<lowerRX ; j += interval ) //x
			{
				candidatesNum = tempCandidate.size();
				if ( ( ( i+patchSize ) < imgH ) && ( ( j+patchSize ) < imgW ) ) //檢查patch是否超出輸入影像的邊界
				{
					if ( !checkPatchOverlapMissing( maskImg.rowRange(i, i+patchSize).colRange(j, j+patchSize).clone() ))
					{
						// 檢查是否有相同的patch存在
						if (candidatesNum)
						{
							bool duplicate=false;
							for ( int k=0 ; k<candidatesNum ; k++ )
							{
								if ( tempCandidate[k] == Point(j, i) )
								{
									duplicate=true;
									break;
								}
							}
							if ( !duplicate )
							{
								tempCandidate.push_back( Point(j, i) );
							}
						} 
						else
						{
							tempCandidate.push_back( Point(j, i) );
						}						
					}
				}
			}
		}
	}
	cout << endl;
}

void modifiedPriorityBP::addExtraCandidate()
{
	int num = extraCandidate.size();
	for ( int i=0; i < num; i++ )
	{
		// 設定candidate範圍，2*candidateRange X 2*candidateRange
		int upperLX, upperLY, lowerRX, lowerRY; // candidate range的左上和右下點
		if ( ( extraCandidate[i].y - candidateRange ) < 0 )
		{
			upperLY=0;
		} 
		else
		{
			upperLY=extraCandidate[i].y-candidateRange;
		}
		if ( ( extraCandidate[i].y + candidateRange ) >= imgH )
		{
			lowerRY=imgH;
		} 
		else
		{
			lowerRY=extraCandidate[i].y+candidateRange;
		}
		if ( ( extraCandidate[i].x - candidateRange ) < 0 )
		{
			upperLX=0;
		} 
		else
		{
			upperLX=extraCandidate[i].x-candidateRange;
		}
		if ( ( extraCandidate[i].x + candidateRange ) >= imgW )
		{
			lowerRX=imgW;
		} 
		else
		{
			lowerRX=extraCandidate[i].x+candidateRange;
		}

		// get all patches for the node
		int candidatesNum=0;
		for ( int i=upperLY ; i<lowerRY ; i++ ) //y
		{
			for ( int j=upperLX ; j<lowerRX ; j++ ) //x
			{
				candidatesNum=tempCandidate.size();
				if ( ( ( i+patchSize ) < imgH ) && ( ( j+patchSize ) < imgW ) ) //檢查patch是否超出輸入影像的邊界
				{
					if ( !checkPatchOverlapMissing(maskImg.rowRange(i, i+patchSize).colRange(j, j+patchSize)))
					{
						// 檢查是否有相同的patch存在
						if (candidatesNum)
						{
							bool duplicate=false;
							for ( int k=0 ; k<candidatesNum ; k++ )
							{
								if ( tempCandidate[k] == Point(j, i) )
								{
									duplicate=true;
									break;
								}
							}
							if ( !duplicate )
							{
								tempCandidate.push_back( Point(j, i) );
							}
						} 
						else
						{
							tempCandidate.push_back( Point(j, i) );
						}						
					}
				}
			}
		}
	}
}

void modifiedPriorityBP::changeCandidates()
{
	int realCanNum = tempCandidate.size(), canNum = candidates.size();
	for ( int i = 0; i < realCanNum; i++ )
	{
		for ( int j = i; j < canNum; j++ )
		{
			if ( tempCandidate[ i ] == candidates[ j ].patchPos )
			{
				swap( candidates[ i ], candidates [ j ] );
				break;
			}
		}
	}
	candidates.erase( candidates.begin()+realCanNum, candidates.end() );
	vector<patchPBP>( candidates ).swap( candidates );
	Mat temp;
	inputImg.copyTo( temp );
	canNum = candidates.size();
	for ( int j = 0; j < canNum; j++ )
	{
		circle(temp,candidates[ j ].patchPos,1,Scalar( 255, 255, 255 ),-1);
	}
	string a="output/"+fileName+"/"+"candidatePatchPos.bmp";
	imwrite(a,temp);
	string fName;
	fName="output/"+fileName+"/"+"data.txt";
	FILE *file=fopen(fName.c_str(),"a");
	candiNum=candidates.size();
	fprintf(file,"candidatePatchSize=%d\n", candiNum);
	fclose(file);
}

float modifiedPriorityBP::VpSSD(Mat target, Mat candidate, Mat mask)
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

float modifiedPriorityBP::VpqSSD(Mat target, Mat candidate) //分別傳入各自overlap的區域
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
	return ( norm( tAvgR, cAvgR, NORM_L2 ) + norm( tAvgG, cAvgG, NORM_L2 ) + norm( tAvgI, cAvgI, NORM_L2 ) ) / (float)( row * col );
}

int modifiedPriorityBP::decideCluster( float v )
{
	if ( v > 0.9 )
	{
		return 10;
	} 
	else if( v > 0.8 )
	{
		return 9;
	}
	else if( v > 0.7 )
	{
		return 8;
	}
	else if( v > 0.6 )
	{
		return 7;
	}
	else if( v > 0.5 )
	{
		return 6;
	}
	else if( v > 0.4 )
	{
		return 5;
	}
	else if( v > 0.3 )
	{
		return 4;
	}
	else if( v > 0.2 )
	{
		return 3;
	}
	else if( v > 0.1 )
	{
		return 2;
	}
	else
	{
		return 1;
	}
}

void modifiedPriorityBP::getSSD0()
{
	cout << "getSSD0" << endl;
	int ssdNum = 10000, similarLevel = 2 * interval, count;
	float ssdSum = 0.0;
	srand( time(NULL) );
	// 在所有的candidates中，隨機取出一個，以它為中心，上下左右similarLevel範圍內隨機取2個patches，算SSD值。重複10000次算平均，即得到SSD0
	for ( int i = 0; i < ssdNum; i++ )
	{
		count = 0;
		Point center = candidates[ ( rand() % candiNum ) ].patchPos, p1, p2;
		int upRow, lowRow, upCol, lowCol, x = center.x, y = center.y, xRange, yRange;
		// patch range
		if ( x - similarLevel <= 0 ) // 左限
		{
			lowCol = 0;
		}
		else
		{
			lowCol = x - similarLevel;
		}
		if ( x + similarLevel >= imgW ) // 右限
		{
			upCol = imgW - 1;
		}
		else
		{
			upCol = x + similarLevel;
		}
		if ( y - similarLevel <= 0 ) // 上限
		{
			lowRow = 0;
		}
		else
		{
			lowRow = y - similarLevel;
		}
		if ( y + similarLevel >= imgH )  // 下限
		{
			upRow = imgH - 1;
		}
		else
		{
			upRow = y + similarLevel;
		}
		xRange = upCol - lowCol;
		yRange = upRow - lowRow;
		// 在candidates中取出2個不同的patch
		do 
		{
			do 
			{
				p1.x = lowCol + ( rand() % ( xRange + 1 ) );
				p1.y = lowRow + ( rand() % ( yRange + 1 ) );
			} while ( candidateMap.at<uchar>( p1.y, p1.x ) == 0 );
			do 
			{
				p2.x = lowCol + ( rand() % ( xRange + 1 ) );
				p2.y = lowRow + ( rand() % ( yRange + 1 ) );
			} while ( candidateMap.at<uchar>( p2.y, p2.x ) == 0 );		
			count++;
			if ( count == ( 4 * similarLevel *similarLevel ) )
			{
				break;
			}
		} while (p1 == p2);
		/*char fName[ 200 ];
		sprintf(fName, "output/%s/candidateMap.bmp",fileName.c_str() );
		Mat t = imread( fName );
		rectangle( t, Point( lowCol, lowRow ), Point( upCol, upRow ), Scalar( 0, 255, 255 ) );
		circle(t, center,1,Scalar(0, 0, 255 ),-1);
		circle(t, p1,1,Scalar(255, 0, 0 ),-1);
		circle(t, p2,1,Scalar(0, 255, 0 ),-1);
		namedWindow( "t", CV_WINDOW_NORMAL);
		imshow("t",t);
		cvWaitKey(0);*/
		ssdSum += VpqSSD( SSDImg.rowRange( p1.y, p1.y + patchSize ).colRange( p1.x, p1.x + patchSize ).clone(), SSDImg.rowRange( p2.y, p2.y + patchSize ).colRange( p2.x, p2.x + patchSize ).clone() );
	}
	float SSD0 = ssdSum / (float)ssdNum;
	Bconf = -SSD0;
	Bprune = -2.0 * SSD0;
	string fName;
	fName="C:/Completion Result/"+fileName+"/"+"data.txt";
	FILE *file=fopen(fName.c_str(),"a");
	fprintf(file,"patchSize=%d\n", patchSize);
	fprintf(file,"candidateNum=%d\n", candiNum);
	fprintf(file,"Bconf=%f\n", Bconf);
	fprintf(file,"Bprune=%f\n", Bprune);
	fclose(file);
	// 粗略的分群
	time_t nStart = time( NULL );
	cout << "分群" << endl;
	Mat target, intensity, saturation;
	vector<Mat> ch;
	for ( int i = 0; i < candiNum; i++ )
	{
		target.~Mat();
		intensity.~Mat();
		vector<Mat>().swap( ch );
		Scalar iAvg, sAvg;
		SSDImg.rowRange( candidates[ i ].patchPos.y, candidates[ i ].patchPos.y + patchSize ).colRange( candidates[ i ].patchPos.x, candidates[ i ].patchPos.x + patchSize ).clone().copyTo( target );
		split( target, ch );
		ch[ 1 ].convertTo( intensity, CV_32FC1 );
		ch[ 2 ].convertTo( saturation, CV_32FC1 );
		intensity /= 255.0;
		saturation /= 255.0;
		iAvg = mean( intensity );
		sAvg = mean( saturation );
		candidates[ i ].iCluster = decideCluster( iAvg.val[ 0 ] );
		candidates[ i ].sCluster = decideCluster( sAvg.val[ 0 ] );
	}
	time_t nEnd = time( NULL );
	cout<< "耗時 = " << ( nEnd - nStart ) << "s" << endl;
}

void modifiedPriorityBP::priorityBP( bool allUse )
{
	cout << "priorityBP"<< endl;
	// iterate until convergence
	time_t nStart = time( NULL );
	for ( int i = 0; i < iterationNum; i++ )
	{
		forwardOrder.clear();
		// forwardPass
		float highestPriority, tempHigh;
		for ( int j = 0; j < boundaryNum; j++ )  // number of nodes
		{
			//system( "cls" );
			cout << "第" << i << "次iteration(" << j + 1 << "/"<< boundaryNum<<")"<< "\r";
			// search for the uncommitted node with highest priority ( boundary nodes first )
			// 因為node的priority有可能為0，所以highestPriority要初始為負值
			highestPriority = -1.0;
			tempHigh = 0;
			for ( int k = 0; k < boundaryNum; k++ )
			{
				// only uncommitted nodes
				if ( !boundaryNodes[ k ].committed )  
				{
					tempHigh = boundaryNodes[ k ].priority;
					if ( tempHigh > highestPriority )
					{
						highestPriority = tempHigh;	
						// 判斷當次的node是否已加入過
						if ( forwardOrder.size() == ( j+1 ) ) 
						{
							forwardOrder[ j ] = &boundaryNodes[ k ];
						} 
						else
						{
							forwardOrder.push_back( &boundaryNodes[ k ] );
						}
					}
				}									
			}
			labelPruning( forwardOrder[ j ] );
			(*forwardOrder[ j ]).committed = true;
			// send msg and update belief and priority to existed and uncommitted neighbors
			sendMsg( forwardOrder[ j ], false );
		}
		// backwardPass
		for ( int j = boundaryNum - 1; j >= 0; j-- )
		{
			(*forwardOrder[ j ]).committed = false;
			sendMsg( forwardOrder[ j ], true );
		}		
	}
	time_t nEnd = time( NULL );
	cout << endl << "耗時 " << ( nEnd - nStart ) <<endl;
	// save the best candidate for every node at each iteration
	for ( int j = 0; j < boundaryNum; j++ )
	{
		int patchIndex=-1;
		//因為是位址所以要相減得到相對位置。
		patchIndex=max_element( boundaryNodes[ j ].belief.begin(), boundaryNodes[ j ].belief.end() ) - boundaryNodes[ j ].belief.begin(); 
		inputImg.rowRange( boundaryNodes[ j ].nodeCandidate[ patchIndex ].patchPos.y, boundaryNodes[ j ].nodeCandidate[ patchIndex ].patchPos.y + patchSize ).colRange( boundaryNodes[ j ].nodeCandidate[ patchIndex ].patchPos.x, boundaryNodes[ j ].nodeCandidate[ patchIndex ].patchPos.x + patchSize ).clone().copyTo( boundaryNodes[ j ].bestPatch );
	}
	// 計算confidence value的門檻值
	cThreshold = ( cMin + cMax ) * 0.2;
	// 如果best patch和target的相似度夠高並且confidence value超過門檻值，則那個點才做
	if ( !allUse )
	{
		float ssd = 0;
		int c = 0;
		while( c != boundaryNum )
		{
			Mat t;
			cvtColor( (*forwardOrder[ c ]).bestPatch, t, CV_BGR2HLS );
			ssd = 0.0;
			ssd = VpSSD( t.clone(), SSDImg.rowRange( (*forwardOrder[ c ]).nodePos.y, (*forwardOrder[ c ]).nodePos.y + patchSize ).colRange( (*forwardOrder[ c ]).nodePos.x, (*forwardOrder[ c ]).nodePos.x + patchSize ).clone(), tempMaskImg.rowRange( (*forwardOrder[ c ]).nodePos.y, (*forwardOrder[ c ]).nodePos.y + patchSize ).colRange( (*forwardOrder[ c ]).nodePos.x, (*forwardOrder[ c ]).nodePos.x + patchSize ).clone() );
			if ( ( ssd <= ( -0.5 * Bconf ) ) && ( (*forwardOrder[ c ]).confidentValue >= cThreshold ) )
			{
				c++;				
			}
			else
			{
				forwardOrder.erase( forwardOrder.begin()+c );
				boundaryNum--;
			}
		}	
		vector<nodePBP*>( forwardOrder ).swap( forwardOrder );
	}
}

void modifiedPriorityBP::getNodeInitialBeliefAndPriority()
{
	time_t nStart = time( NULL );
	// calculate belief	
	char fName[200];
	sprintf(fName, "C:/Completion Result/%s/nodeInitialVp",fileName.c_str());
	_mkdir(fName);
	// 清空資料夾裡的所有檔案
	CFileFind cFindfile;
	sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/*.*",fileName.c_str() );
	BOOL bfind = cFindfile.FindFile(fName);
	while( bfind )
	{
		bfind = cFindfile.FindNextFile();
		if( cFindfile.IsDots() )
			continue;
		DeleteFile(cFindfile.GetFilePath());
	} 
	cFindfile.Close();
	float temp=0.0, bMax, epsilon = 0.3;
	Mat target, patch, mask, oppositeMask, intensity, saturation;
	vector<Mat> ch;
	int patchNum=candidates.size();
	for ( int i=0 ; i < boundaryNum ; i++ )
	{		
		cout << "getNodeInitialBeliefAndPriority(" << i + 1<< "/"<< boundaryNum<<")"<< "\r";
		sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d).txt",fileName.c_str(), boundaryNodes[ i ].nodePos.x , boundaryNodes[ i ].nodePos.y );
		FILE *file=fopen(fName,"w");
		sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d)_candidate.txt",fileName.c_str(), boundaryNodes[ i ].nodePos.x , boundaryNodes[ i ].nodePos.y );
		FILE *file1=fopen(fName,"w");		
		// 計算出node所有patch的Vp，因為是第一次所以belief=Vp
		target.~Mat();
		mask.~Mat();
		oppositeMask.~Mat();
		SSDImg.rowRange( boundaryNodes[ i ].nodePos.y, boundaryNodes[ i ].nodePos.y + patchSize ).colRange( boundaryNodes[ i ].nodePos.x, boundaryNodes[ i ].nodePos.x + patchSize ).clone().copyTo( target );
		tempMaskImg.rowRange( boundaryNodes[ i ].nodePos.y, boundaryNodes[ i ].nodePos.y + patchSize ).colRange( boundaryNodes[ i ].nodePos.x, boundaryNodes[ i ].nodePos.x + patchSize).clone().copyTo( mask );
		bitwise_not( mask, oppositeMask );
		vector<Mat>().swap( ch );
		split( target, ch );
		// 計算target平均intensity和saturation的可能範圍
		Scalar iUpBound, iLowBound, sUpBound, sLowBound, iAvg;
		int iUpCluster, iLowCluster, sUpCluster, sLowCluster;			
		// 下限
		ch[ 1 ].convertTo( intensity, CV_32FC1 );
		intensity.setTo( 0, oppositeMask );
		intensity /= 255.0;
		iLowBound = mean( intensity );
		iLowCluster = decideCluster( iLowBound.val[ 0 ] );
		intensity.~Mat();
		ch[ 2 ].convertTo( saturation, CV_32FC1 );
		saturation.setTo( 0, oppositeMask );
		saturation /= 255.0;
		sLowBound = mean( saturation );
		sLowCluster = decideCluster( sLowBound.val[ 0 ] );
		saturation.~Mat();
		// 上限
		ch[ 1 ].convertTo( intensity, CV_32FC1 );
		intensity.setTo( 255, oppositeMask );
		intensity /= 255.0;
		iUpBound = mean( intensity );
		iUpCluster = decideCluster( iUpBound.val[ 0 ] );
		intensity.~Mat();
		ch[ 2 ].convertTo( saturation, CV_32FC1 );
		saturation.setTo( 255, oppositeMask );
		saturation /= 255.0;
		sUpBound = mean( saturation );
		sUpCluster = decideCluster( sUpBound.val[ 0 ] );
		saturation.~Mat();
		// 計算target中已知pixel的亮度符合條件的比例
		ch[ 1 ].convertTo( intensity, CV_32FC1 );
		intensity /= 255.0;
		int row = intensity.rows, col = intensity.cols;
		float iRatio = 0;
		for ( int i = 0; i < row; i++ )
		{
			for ( int j = 0; j < col; j++ )
			{
				if ( mask.at<uchar>( i, j ) )
				{
					if ( ( intensity.at<float>( i, j ) <= epsilon ) || ( intensity.at<float>( i, j ) >= ( 1.0 - epsilon ) ) )
					{
						iRatio++;
					}
				}
			}
		}
		iRatio /= (float)countNonZero( mask );
		intensity.~Mat();
		for ( int k=0 ; k < patchNum ; k++ )
		{		
			patch.~Mat();	
			SSDImg.rowRange(candidates[k].patchPos.y, candidates[k].patchPos.y + patchSize).colRange(candidates[k].patchPos.x, candidates[k].patchPos.x  + patchSize).clone().copyTo( patch );						
			// 一旦candidate落在範圍內，則計算SSD並且視為此點的candidate。反之，則把此patch去掉
			// 一旦iRatio超過某個比例代表saturation不可靠，所以只用intensity來決定candidate
			if ( iRatio >= 0.5 )
			{
				if ( ( candidates[ k ].iCluster >= iLowCluster ) && ( candidates[ k ].iCluster <= iUpCluster ) )
				{
					temp=VpSSD( target.clone(), patch.clone(), mask.clone() );
					boundaryNodes[ i ].belief.push_back( -temp ); 
					fprintf( file,"%f\n", temp );
					fprintf( file1,"%d,%d\n", candidates[k].patchPos.x, candidates[k].patchPos.y );
				} 
			} 
			else
			{
				if ( ( candidates[ k ].iCluster >= iLowCluster ) && ( candidates[ k ].iCluster <= iUpCluster ) && ( candidates[ k ].sCluster >= sLowCluster ) && ( candidates[ k ].sCluster <= sUpCluster ) )
				{
					temp=VpSSD( target.clone(), patch.clone(), mask.clone() );
					boundaryNodes[ i ].belief.push_back( -temp ); 
					fprintf( file,"%f\n", temp );
					fprintf( file1,"%d,%d\n", candidates[k].patchPos.x, candidates[k].patchPos.y );
				} 
			}									
		}		
		// 計算confidenceMap
		setNodeConfidence();
		int reservedpatchNum = boundaryNodes[ i ].belief.size();
		// 如果留下來的candidates數量不足Lmin個，則隨機選取補足Lmin個
		if ( reservedpatchNum < Lmin )
		{
			srand( time(NULL) );
			int num = Lmin - reservedpatchNum, patchLoc;
			for ( int i=0 ; i < num ; i++ )
			{
				patchLoc = rand() % candiNum;
				SSDImg.rowRange( candidates[ patchLoc ].patchPos.y, candidates[ patchLoc ].patchPos.y + patchSize).colRange( candidates[ patchLoc ].patchPos.x, candidates[ patchLoc ].patchPos.x  + patchSize).clone().copyTo( patch );	
				temp=VpSSD( target.clone(), patch.clone(), mask.clone() );
				fprintf( file,"%f\n", temp );
				fprintf( file1,"%d,%d\n", candidates[ patchLoc ].patchPos.x, candidates[ patchLoc ].patchPos.y );
			}
			boundaryNodes[ i ].priority=0.0;
		} 
		else
		{
			boundaryNodes[ i ].activeLabelNum = reservedpatchNum;
			// relative belief
			bMax=0;
			bMax=*max_element( boundaryNodes[ i ].belief.begin(), boundaryNodes[ i ].belief.end() );
			for ( int k=0 ; k < reservedpatchNum ; k++ )
			{
				boundaryNodes[ i ].belief.at(k)-=bMax; 
			}
			// priority
			float confusionSet=0.0;
			for (int k=0 ; k < reservedpatchNum ; k++ )
			{
				if ( boundaryNodes[ i ].belief.at(k)>=Bconf)
				{
					confusionSet++;
				}
			}
			if ( confusionSet )
			{
				boundaryNodes[ i ].priority= (1.0/confusionSet) * boundaryNodes[ i ].confidentValue;
			} 
			else
			{
				boundaryNodes[ i ].priority=0.0;
			}		
		}			
		vector<float>().swap( boundaryNodes[ i ].belief); //釋放記憶體
		fclose(file);
		fclose(file1);
	}	
	time_t nEnd = time( NULL );
	cout << endl << "共耗時 " << ( nEnd - nStart ) << "s" << endl;
}

void SaveMatInfoPBP(char *F_Name,Mat mat) //存Mat資料(TXT)
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

void modifiedPriorityBP::labelPruning( nodePBP* node )
{
	// 初始化, if needed
	if ( !(*node).alreadyVisited )
	{
		char fName[200];
		char* token;
		sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d)_candidate.txt",fileName.c_str(), (*node).nodePos.x , (*node).nodePos.y );
		(*node).alreadyVisited = true;
		// 設置點的candidates
		vector<patchPBP>().swap((*node).nodeCandidate); 
		int x = 0, y = 0;
		fstream fin;
		fin.open(fName, ios::in );
		while( fin.getline( fName, sizeof(fName) ) )
		{
			x = 0;
			y = 0;
			token = strtok( fName, "," );
			x = atoi(token);
			token = strtok( NULL, "," );
			y = atoi(token);
			patchPBP patch( Point( x, y ) );
			(*node).nodeCandidate.push_back(patch);	
		}
		fin.close();
		// 替鄰居，初始化
		if ( (*node).upperNeighbor != nullptr && (*node).upEdge == nullptr ) 
		{
			if ( !(*node).upperNeighbor->nodeCandidate.size() )
			{
				// 設置點的candidates
				vector<patchPBP>().swap((*node).upperNeighbor->nodeCandidate); 
				sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d)_candidate.txt",fileName.c_str(), (*node).upperNeighbor->nodePos.x , (*node).upperNeighbor->nodePos.y );
				fin.open(fName, ios::in );
				while( fin.getline( fName, sizeof(fName) ) )
				{
					x = 0;
					y = 0;
					token = strtok( fName, "," );
					x = atoi(token);
					token = strtok( NULL, "," );
					y = atoi(token);
					patchPBP patch( Point( x, y ) );
					(*node).upperNeighbor->nodeCandidate.push_back(patch);	
				}
				fin.close();				
			}
			edgePBP* edge = new edgePBP( (*node).nodePos, (*node).upperNeighbor->nodePos );
			(*node).upEdge = edge;
			(*node).upperNeighbor->lowEdge = edge;
		}
		if ( (*node).lowerNeighbor != nullptr && (*node).lowEdge == nullptr ) 
		{
			if ( !(*node).lowerNeighbor->nodeCandidate.size() )
			{
				// 設置點的candidates
				vector<patchPBP>().swap((*node).lowerNeighbor->nodeCandidate); 
				sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d)_candidate.txt",fileName.c_str(), (*node).lowerNeighbor->nodePos.x , (*node).lowerNeighbor->nodePos.y );
				fin.open(fName, ios::in );
				while( fin.getline( fName, sizeof(fName) ) )
				{
					x = 0;
					y = 0;
					token = strtok( fName, "," );
					x = atoi(token);
					token = strtok( NULL, "," );
					y = atoi(token);
					patchPBP patch( Point( x, y ) );
					(*node).lowerNeighbor->nodeCandidate.push_back(patch);	
				}
				fin.close();				
			}
			edgePBP* edge = new edgePBP( (*node).nodePos, (*node).lowerNeighbor->nodePos );
			(*node).lowEdge = edge;
			(*node).lowerNeighbor->upEdge = edge;
		}
		if ( (*node).leftNeighbor != nullptr && (*node).leftEdge == nullptr ) 
		{
			if ( !(*node).leftNeighbor->nodeCandidate.size() )
			{
				// 設置點的candidates
				vector<patchPBP>().swap((*node).leftNeighbor->nodeCandidate); 
				sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d)_candidate.txt",fileName.c_str(), (*node).leftNeighbor->nodePos.x , (*node).leftNeighbor->nodePos.y );
				fin.open(fName, ios::in );
				while( fin.getline( fName, sizeof(fName) ) )
				{
					x = 0;
					y = 0;
					token = strtok( fName, "," );
					x = atoi(token);
					token = strtok( NULL, "," );
					y = atoi(token);
					patchPBP patch( Point( x, y ) );
					(*node).leftNeighbor->nodeCandidate.push_back(patch);	
				}
				fin.close();				
			}
			edgePBP* edge = new edgePBP( (*node).nodePos, (*node).leftNeighbor->nodePos );
			(*node).leftEdge = edge;
			(*node).leftNeighbor->rightEdge = edge;
		}
		if ( (*node).rightNeighbor != nullptr && (*node).rightEdge == nullptr ) 
		{
			if ( !(*node).rightNeighbor->nodeCandidate.size() )
			{
				// 設置點的candidates
				vector<patchPBP>().swap((*node).rightNeighbor->nodeCandidate); 
				sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d)_candidate.txt",fileName.c_str(), (*node).rightNeighbor->nodePos.x , (*node).rightNeighbor->nodePos.y );
				fin.open(fName, ios::in );
				while( fin.getline( fName, sizeof(fName) ) )
				{
					x = 0;
					y = 0;
					token = strtok( fName, "," );
					x = atoi(token);
					token = strtok( NULL, "," );
					y = atoi(token);
					patchPBP patch( Point( x, y ) );
					(*node).rightNeighbor->nodeCandidate.push_back(patch);	
				}
				fin.close();				
			}
			edgePBP* edge = new edgePBP( (*node).nodePos, (*node).rightNeighbor->nodePos );
			(*node).rightEdge = edge;
			(*node).rightNeighbor->leftEdge = edge;
		}
	}
	// 讀入belief
	if ( !(*node).beliefExist )
	{
		(*node).beliefExist = true;
		vector<float>().swap((*node).belief);
		fstream fin;
		char fName[200];
		sprintf(fName, "C:/Completion Result/%s/nodeInitialVp/MrfNodes(%d,%d).txt",fileName.c_str(), (*node).nodePos.x, (*node).nodePos.y  );
		fin.open(fName, ios::in );
		int count=0;
		while( fin.getline( fName, sizeof(fName) ) )
		{
			double value=strtod( fName, NULL );
			(*node).belief.push_back( -value );
			(*node).nodeCandidate[count].Vp=value;
			count++; 
		}
		fin.close();
	}

	// 最少保證留下Lmin個candidates
	if ( (*node).nodeCandidate.size() > Lmin ) 
	{
		// relative belief
		float bMax=0.0;
		int patchNum=(*node).nodeCandidate.size();
		vector<float> relativeBelief;
		relativeBelief = (*node).belief;
		bMax=*max_element( relativeBelief.begin(), relativeBelief.end() );
		for ( int k=0 ; k < patchNum ; k++ )
		{
			relativeBelief.at(k)-=bMax; 
		}
		// 篩選Lmax個candidates
		int activeNum=0, nonActiveNum=0, beliefNum=relativeBelief.size();
		for ( int i=0 ; i < beliefNum; i++ )
		{
			float maxBelief=0;
			int patchIndex=-1;
			patchIndex=( max_element( relativeBelief.begin()+activeNum, relativeBelief.end() - nonActiveNum ) - ( relativeBelief.begin()+activeNum ) ) + activeNum; //從剩下的patch中找最大的belief，因為是位址所以要相減得到相對位置。之後再加上已存在的activeNum來得到原始位置
			maxBelief=relativeBelief.at(patchIndex);
			if ( maxBelief > Bprune )
			{
				// 新patch只要和已存在的patches差異性夠大，就可以成為candidates
				for ( int j=0 ; j < activeNum ; j++ ) 
				{
					float d = VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ j ].patchPos.y, (*node).nodeCandidate[ j ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ j ].patchPos.x, (*node).nodeCandidate[ j ].patchPos.x + patchSize).clone(), SSDImg.rowRange( (*node).nodeCandidate[ patchIndex ].patchPos.y, (*node).nodeCandidate[ patchIndex ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ patchIndex ].patchPos.x, (*node).nodeCandidate[ patchIndex ].patchPos.x + patchSize).clone() );
					if ( d < ( -5 * Bconf ) )
					{
						// 把non-active label移到後方
						swap( (*node).nodeCandidate[patchIndex], (*node).nodeCandidate[(*node).nodeCandidate.size()-1-nonActiveNum] ); 
						swap( (*node).belief[patchIndex], (*node).belief[(*node).belief.size()-1-nonActiveNum] );
						swap( relativeBelief[patchIndex], relativeBelief[(*node).belief.size()-1-nonActiveNum] );
						nonActiveNum++;
						break;
					}	
					if ( j == ( activeNum - 1 ) ) 
					{
						//把active label移到前方
						swap( (*node).nodeCandidate[patchIndex], (*node).nodeCandidate[activeNum] ); 
						swap( (*node).belief[patchIndex], (*node).belief[activeNum] );
						swap( relativeBelief[patchIndex], relativeBelief[activeNum] );
						activeNum++;
						break;
					}
				}	
				if ( activeNum == 0 ) 
				{
					//把active label移到前方
					swap( (*node).nodeCandidate[patchIndex], (*node).nodeCandidate[activeNum] ); 
					swap( (*node).belief[patchIndex], (*node).belief[activeNum] );
					swap( relativeBelief[patchIndex], relativeBelief[activeNum] );
					activeNum++;
				}
			} 
			if ( activeNum == Lmax )
			{
				break;
			}
		}
		// 如果active label數量小於Lmin，則補至Lmin個
		if ( activeNum < Lmin )
		{
			nonActiveNum=0;
			int num=Lmin-activeNum;
			for ( int i=0 ; i < num ; i++ )
			{
				int patchIndex=-1;
				patchIndex=( max_element( relativeBelief.begin()+activeNum, relativeBelief.end() - nonActiveNum ) - ( relativeBelief.begin()+activeNum ) ) + activeNum; //從剩下的patch中找最大的belief，因為是位址所以要相減得到相對位置。之後再加上已存在的activeNum來得到原始位置
				swap( (*node).nodeCandidate[patchIndex], (*node).nodeCandidate[activeNum] ); //把active label移到前方
				swap( (*node).belief[patchIndex], (*node).belief[activeNum] );
				swap( relativeBelief[patchIndex], relativeBelief[activeNum] );
				activeNum++;				
			}
		} 
		// 把non-active label去掉
		(*node).nodeCandidate.erase( (*node).nodeCandidate.begin()+activeNum, (*node).nodeCandidate.end() );
		vector<patchPBP>((*node).nodeCandidate).swap((*node).nodeCandidate);  // 重新調整vector為現有的大小，並且釋放其刪除的元素所占用的記憶體
		(*node).belief.erase( (*node).belief.begin()+activeNum, (*node).belief.end() );
		vector<float>((*node).belief).swap((*node).belief);
		vector<float>().swap( relativeBelief );
		(*node).activeLabelNum=activeNum;
		// 把留下的candidates存檔
		//char fName[ 200 ];
		//sprintf(fName, "C:/Completion Result/%s/nodeCandidatesAfterPruning%d",fileName.c_str(), patchSize );
		//_mkdir(fName);
		//sprintf(fName, "C:/Completion Result/%s/nodeCandidatesAfterPruning%d/MrfNodes(%d,%d)",fileName.c_str(), patchSize, (*node).nodePos.x, (*node).nodePos.y  );
		//_mkdir(fName);
		//sprintf(fName, "C:/Completion Result/%s/nodeCandidatesAfterPruning%d/MrfNodes(%d,%d)/target.bmp",fileName.c_str(), patchSize, (*node).nodePos.x, (*node).nodePos.y );
		//imwrite( fName, inputImg.rowRange( (*node).nodePos.y, (*node).nodePos.y + patchSize ).colRange( (*node).nodePos.x, (*node).nodePos.x + patchSize ).clone() );
		//for ( int i =0; i < activeNum; i++ )
		//{
		//	sprintf(fName, "C:/Completion Result/%s/nodeCandidatesAfterPruning%d/MrfNodes(%d,%d)/%f.bmp",fileName.c_str(), patchSize, (*node).nodePos.x, (*node).nodePos.y,  (*node).belief.at( i ) );
		//	imwrite( fName, inputImg.rowRange( (*node).nodeCandidate[ i ].patchPos.y, (*node).nodeCandidate[ i ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ i ].patchPos.x, (*node).nodeCandidate[ i ].patchPos.x + patchSize ).clone() );
		//}		
	}
	// 改變Vpq
	/*char fName[ 200 ];
	sprintf(fName, "C:/Completion Result/%s/Vpq",fileName.c_str() );
	_mkdir(fName);
	sprintf(fName, "C:/Completion Result/%s/Vpq/MrfNodes(%d,%d)",fileName.c_str(), (*node).nodePos.x, (*node).nodePos.y  );
	_mkdir(fName);*/
	if ( (*node).upEdge != nullptr )
	{
		if ( (*node).upEdge->checkRowOrNot( (*node).nodePos ) ) // row 代表本點的patch
		{
			int row = (*node).nodeCandidate.size(), col = (*node).upperNeighbor->nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					// only overlap area
					float t=VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ i ].patchPos.y, (*node).nodeCandidate[ i ].patchPos.y + ( patchSize - gap ) ).colRange( (*node).nodeCandidate[ i ].patchPos.x, (*node).nodeCandidate[ i ].patchPos.x + patchSize ).clone(), SSDImg.rowRange( (*node).upperNeighbor->nodeCandidate[ j ].patchPos.y + gap, (*node).upperNeighbor->nodeCandidate[ j ].patchPos.y + patchSize ).colRange( (*node).upperNeighbor->nodeCandidate[ j ].patchPos.x, (*node).upperNeighbor->nodeCandidate[ j ].patchPos.x + patchSize ).clone() );
					if ( t ==0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).upEdge->Vpq.~Mat();
			temp.copyTo( (*node).upEdge->Vpq );
		} 
		else // row代表鄰居點的patch
		{
			int row = (*node).upperNeighbor->nodeCandidate.size(), col = (*node).nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					// only overlap area
					float t=VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ j ].patchPos.y, (*node).nodeCandidate[ j ].patchPos.y + ( patchSize - gap ) ).colRange( (*node).nodeCandidate[ j ].patchPos.x, (*node).nodeCandidate[ j ].patchPos.x + patchSize).clone(), SSDImg.rowRange( (*node).upperNeighbor->nodeCandidate[ i ].patchPos.y + gap, (*node).upperNeighbor->nodeCandidate[ i ].patchPos.y + patchSize ).colRange( (*node).upperNeighbor->nodeCandidate[ i ].patchPos.x, (*node).upperNeighbor->nodeCandidate[ i ].patchPos.x + patchSize).clone() );
					if ( t ==0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).upEdge->Vpq.~Mat();
			temp.copyTo( (*node).upEdge->Vpq );
		}			
		//sprintf(fName, "C:/Completion Result/%s/Vpq/MrfNodes(%d,%d)/UpEdge.txt",fileName.c_str(), (*node).nodePos.x, (*node).nodePos.y  );
		//SaveMatInfoPBP( fName, (*node).upEdge->Vpq );
	}
	if ( (*node).lowEdge != nullptr )
	{
		if ( (*node).lowEdge->checkRowOrNot( (*node).nodePos ) ) // row 代表本點的patch
		{
			int row = (*node).nodeCandidate.size(), col = (*node).lowerNeighbor->nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					float t = VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ i ].patchPos.y + gap, (*node).nodeCandidate[ i ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ i ].patchPos.x, (*node).nodeCandidate[ i ].patchPos.x + patchSize).clone(), SSDImg.rowRange( (*node).lowerNeighbor->nodeCandidate[ j ].patchPos.y, (*node).lowerNeighbor->nodeCandidate[ j ].patchPos.y + ( patchSize - gap ) ).colRange( (*node).lowerNeighbor->nodeCandidate[ j ].patchPos.x, (*node).lowerNeighbor->nodeCandidate[ j ].patchPos.x + patchSize).clone() );
					if ( t ==0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).lowEdge->Vpq.~Mat();
			temp.copyTo( (*node).lowEdge->Vpq );
		} 
		else // row代表鄰居點的patch
		{
			int row = (*node).lowerNeighbor->nodeCandidate.size(), col = (*node).nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					float t = VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ j ].patchPos.y + gap, (*node).nodeCandidate[ j ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ j ].patchPos.x, (*node).nodeCandidate[ j ].patchPos.x + patchSize).clone(), SSDImg.rowRange( (*node).lowerNeighbor->nodeCandidate[ i ].patchPos.y, (*node).lowerNeighbor->nodeCandidate[ i ].patchPos.y + ( patchSize - gap ) ).colRange( (*node).lowerNeighbor->nodeCandidate[ i ].patchPos.x, (*node).lowerNeighbor->nodeCandidate[ i ].patchPos.x + patchSize).clone() );
					if ( t ==0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).lowEdge->Vpq.~Mat();
			temp.copyTo( (*node).lowEdge->Vpq );
		}
		//sprintf(fName, "C:/Completion Result/%s/Vpq/MrfNodes(%d,%d)/lowEdge.txt",fileName.c_str(), (*node).nodePos.x, (*node).nodePos.y  );
		//SaveMatInfoPBP( fName, (*node).lowEdge->Vpq );
	}
	if ( (*node).leftEdge != nullptr )
	{
		if ( (*node).leftEdge->checkRowOrNot( (*node).nodePos ) ) // row 代表本點的patch
		{
			int row = (*node).nodeCandidate.size(), col = (*node).leftNeighbor->nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					float t = VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ i ].patchPos.y, (*node).nodeCandidate[ i ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ i ].patchPos.x, (*node).nodeCandidate[ i ].patchPos.x + ( patchSize - gap ) ).clone(), SSDImg.rowRange( (*node).leftNeighbor->nodeCandidate[ j ].patchPos.y, (*node).leftNeighbor->nodeCandidate[ j ].patchPos.y + patchSize ).colRange( (*node).leftNeighbor->nodeCandidate[ j ].patchPos.x + gap, (*node).leftNeighbor->nodeCandidate[ j ].patchPos.x + patchSize ).clone() );
					if ( t == 0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).leftEdge->Vpq.~Mat();
			temp.copyTo( (*node).leftEdge->Vpq );
		} 
		else // row代表鄰居點的patch
		{
			int row = (*node).leftNeighbor->nodeCandidate.size(), col = (*node).nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					float t = VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ j ].patchPos.y, (*node).nodeCandidate[ j ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ j ].patchPos.x, (*node).nodeCandidate[ j ].patchPos.x + ( patchSize - gap ) ).clone(), SSDImg.rowRange( (*node).leftNeighbor->nodeCandidate[ i ].patchPos.y, (*node).leftNeighbor->nodeCandidate[ i ].patchPos.y + patchSize ).colRange( (*node).leftNeighbor->nodeCandidate[ i ].patchPos.x + gap, (*node).leftNeighbor->nodeCandidate[ i ].patchPos.x + patchSize ).clone() );
					if ( t == 0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).leftEdge->Vpq.~Mat();
			temp.copyTo( (*node).leftEdge->Vpq );
		}
		//sprintf(fName, "C:/Completion Result/%s/Vpq/MrfNodes(%d,%d)/leftEdge.txt",fileName.c_str(), (*node).nodePos.x, (*node).nodePos.y  );
		//SaveMatInfoPBP( fName, (*node).leftEdge->Vpq );
	}
	if ( (*node).rightEdge != nullptr )
	{
		if ( (*node).rightEdge->checkRowOrNot( (*node).nodePos ) ) // row 代表本點的patch
		{
			int row = (*node).nodeCandidate.size(), col = (*node).rightNeighbor->nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					float t = VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ i ].patchPos.y, (*node).nodeCandidate[ i ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ i ].patchPos.x + gap, (*node).nodeCandidate[ i ].patchPos.x + patchSize ).clone(), SSDImg.rowRange( (*node).rightNeighbor->nodeCandidate[ j ].patchPos.y, (*node).rightNeighbor->nodeCandidate[ j ].patchPos.y + patchSize ).colRange( (*node).rightNeighbor->nodeCandidate[ j ].patchPos.x, (*node).rightNeighbor->nodeCandidate[ j ].patchPos.x + ( patchSize - gap ) ).clone() );
					if ( t == 0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).rightEdge->Vpq.~Mat();
			temp.copyTo( (*node).rightEdge->Vpq );
		} 
		else // row代表鄰居點的patch
		{
			int row = (*node).rightNeighbor->nodeCandidate.size(), col = (*node).nodeCandidate.size();
			Mat temp( row, col, CV_32FC1, Scalar(0) );
			for ( int i = 0; i < row; i++ )
			{
				for ( int j = 0; j < col; j++ )
				{
					float t = VpqSSD( SSDImg.rowRange( (*node).nodeCandidate[ j ].patchPos.y, (*node).nodeCandidate[ j ].patchPos.y + patchSize ).colRange( (*node).nodeCandidate[ j ].patchPos.x + gap, (*node).nodeCandidate[ j ].patchPos.x + patchSize ).clone(), SSDImg.rowRange( (*node).rightNeighbor->nodeCandidate[ i ].patchPos.y, (*node).rightNeighbor->nodeCandidate[ i ].patchPos.y + patchSize ).colRange( (*node).rightNeighbor->nodeCandidate[ i ].patchPos.x, (*node).rightNeighbor->nodeCandidate[ i ].patchPos.x + ( patchSize - gap ) ).clone() );
					if ( t == 0.0 )
					{
						t = 1000000.0;
					}
					temp.at<float>( i, j ) = t;
				}
			}
			(*node).rightEdge->Vpq.~Mat();
			temp.copyTo( (*node).rightEdge->Vpq );
		}
		//sprintf(fName, "C:/Completion Result/%s/Vpq/MrfNodes(%d,%d)/rightEdge.txt",fileName.c_str(), (*node).nodePos.x, (*node).nodePos.y  );
		//SaveMatInfoPBP( fName, (*node).rightEdge->Vpq );
	}
}

void modifiedPriorityBP::sendMsg( nodePBP* node, bool sendWhichOne )
{
	// upper neighbor
	if ( (*node).upperNeighbor != nullptr && (*node).upperNeighbor->committed == sendWhichOne ) 
	{
		vector<float>().swap( (*node).upperNeighbor->belief );

		int pLabelNum = (*node).nodeCandidate.size(), qLabelNum = (*node).upperNeighbor->nodeCandidate.size();
		vector<float> tempMsg;
		// 對鄰居的每個candidate傳Msg，並且更新belief
		for ( int i = 0; i < qLabelNum; i++ )
		{
			vector<float>().swap( tempMsg );			
			// 為了判斷此點在Vpq中是否放在row
			if ( (*node).upEdge->checkRowOrNot( (*node).nodePos ) )  
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).upEdge->Vpq.at<float>( j, i ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].lowerMsg + (*node).nodeCandidate[ j ].leftMsg + (*node).nodeCandidate[ j ].rightMsg;
					tempMsg.push_back( temp );
				}
			} 
			else
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).upEdge->Vpq.at<float>( i, j ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].lowerMsg + (*node).nodeCandidate[ j ].leftMsg + (*node).nodeCandidate[ j ].rightMsg;
					tempMsg.push_back( temp );
				}
			}
			// send Msg
			(*node).upperNeighbor->nodeCandidate[ i ].lowerMsg = sqrtf( *min_element( tempMsg.begin(), tempMsg.end() ) );		
			// update belief
			float tempBelief=0.0-(*node).upperNeighbor->nodeCandidate[ i ].Vp - ( (*node).upperNeighbor->nodeCandidate[ i ].leftMsg + (*node).upperNeighbor->nodeCandidate[ i ].rightMsg + (*node).upperNeighbor->nodeCandidate[ i ].upperMsg + (*node).upperNeighbor->nodeCandidate[ i ].lowerMsg );
			(*node).upperNeighbor->belief.push_back( tempBelief );
		}
		// relative belief
		float bMax=0;
		vector <float> relativeBelief;
		relativeBelief = (*node).upperNeighbor->belief;
		int beliefNum=relativeBelief.size();
		bMax=*max_element( relativeBelief.begin(), relativeBelief.end() );
		for ( int k=0 ; k < beliefNum ; k++ )
		{
			relativeBelief.at(k)-=bMax; 
		}
		// update priority
		float confusionSet=0;
		for (int k=0 ; k < beliefNum ; k++ )
		{
			if ( relativeBelief.at(k) >= Bconf )
			{
				confusionSet++;
			}
		}
		if ( confusionSet )
		{
			(*node).upperNeighbor->priority= (1/confusionSet) * (*node).upperNeighbor->confidentValue;
		} 
		else
		{
			(*node).upperNeighbor->priority=0.0;
		}
	}
	// lower neighbor
	if ( (*node).lowerNeighbor != nullptr && (*node).lowerNeighbor->committed == sendWhichOne ) 
	{
		vector<float>().swap( (*node).lowerNeighbor->belief );

		int pLabelNum = (*node).nodeCandidate.size(), qLabelNum = (*node).lowerNeighbor->nodeCandidate.size();
		vector<float> tempMsg;
		// 對鄰居的每個candidate傳Msg，並且更新belief
		for ( int i = 0; i < qLabelNum; i++ )
		{
			vector<float>().swap( tempMsg );			
			// 為了判斷此點在Vpq中是否放在row
			if ( (*node).lowEdge->checkRowOrNot( (*node).nodePos ) )  
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).lowEdge->Vpq.at<float>( j, i ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].upperMsg + (*node).nodeCandidate[ j ].leftMsg + (*node).nodeCandidate[ j ].rightMsg;
					tempMsg.push_back( temp );
				}
			} 
			else
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).lowEdge->Vpq.at<float>( i, j ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].upperMsg + (*node).nodeCandidate[ j ].leftMsg + (*node).nodeCandidate[ j ].rightMsg;
					tempMsg.push_back( temp );
				}
			}
			// send Msg	
			(*node).lowerNeighbor->nodeCandidate[ i ].upperMsg = sqrtf( *min_element( tempMsg.begin(), tempMsg.end() ) );			
			// update belief
			float tempBelief=0.0-(*node).lowerNeighbor->nodeCandidate[ i ].Vp - ( (*node).lowerNeighbor->nodeCandidate[ i ].leftMsg + (*node).lowerNeighbor->nodeCandidate[ i ].rightMsg + (*node).lowerNeighbor->nodeCandidate[ i ].upperMsg + (*node).lowerNeighbor->nodeCandidate[ i ].lowerMsg );
			(*node).lowerNeighbor->belief.push_back( tempBelief );
		}
		// relative belief
		float bMax=0;
		vector <float> relativeBelief;
		relativeBelief = (*node).lowerNeighbor->belief;
		int beliefNum=relativeBelief.size();
		bMax=*max_element( relativeBelief.begin(), relativeBelief.end() );
		for ( int k=0 ; k < beliefNum ; k++ )
		{
			relativeBelief.at(k)-=bMax; 
		}
		// update priority
		float confusionSet=0;
		for (int k=0 ; k < beliefNum ; k++ )
		{
			if ( relativeBelief.at(k) >= Bconf )
			{
				confusionSet++;
			}
		}
		if ( confusionSet )
		{
			(*node).lowerNeighbor->priority=(1/confusionSet) * (*node).lowerNeighbor->confidentValue;
		} 
		else
		{
			(*node).lowerNeighbor->priority=0.0;
		}
	}
	// left neighbor
	if ( (*node).leftNeighbor != nullptr && (*node).leftNeighbor->committed == sendWhichOne ) 
	{
		vector<float>().swap( (*node).leftNeighbor->belief );

		int pLabelNum = (*node).nodeCandidate.size(), qLabelNum = (*node).leftNeighbor->nodeCandidate.size();
		vector<float> tempMsg;
		// 對鄰居的每個candidate傳Msg，並且更新belief
		for ( int i = 0; i < qLabelNum; i++ )
		{
			vector<float>().swap( tempMsg );			
			// 為了判斷此點在Vpq中是否放在row
			if ( (*node).leftEdge->checkRowOrNot( (*node).nodePos ) )  
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).leftEdge->Vpq.at<float>( j, i ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].upperMsg + (*node).nodeCandidate[ j ].lowerMsg + (*node).nodeCandidate[ j ].rightMsg;
					tempMsg.push_back( temp );
				}
			} 
			else
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).leftEdge->Vpq.at<float>( i, j ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].upperMsg + (*node).nodeCandidate[ j ].lowerMsg + (*node).nodeCandidate[ j ].rightMsg;
					tempMsg.push_back( temp );
				}
			}
			// send Msg
			(*node).leftNeighbor->nodeCandidate[ i ].rightMsg = sqrtf( *min_element( tempMsg.begin(), tempMsg.end() ) );	
			// update belief
			float tempBelief=0.0-(*node).leftNeighbor->nodeCandidate[ i ].Vp - ( (*node).leftNeighbor->nodeCandidate[ i ].leftMsg + (*node).leftNeighbor->nodeCandidate[ i ].rightMsg + (*node).leftNeighbor->nodeCandidate[ i ].upperMsg + (*node).leftNeighbor->nodeCandidate[ i ].lowerMsg );
			(*node).leftNeighbor->belief.push_back( tempBelief );
		}
		// relative belief
		float bMax=0;
		vector <float> relativeBelief;
		relativeBelief = (*node).leftNeighbor->belief;
		int beliefNum = relativeBelief.size();
		bMax=*max_element( relativeBelief.begin(), relativeBelief.end() );
		for ( int k=0 ; k < beliefNum ; k++ )
		{
			relativeBelief.at(k)-=bMax; 
		}
		// update priority
		float confusionSet=0;
		for (int k=0 ; k < beliefNum ; k++ )
		{
			if ( relativeBelief.at(k) >= Bconf )
			{
				confusionSet++;
			}
		}
		if ( confusionSet )
		{
			(*node).leftNeighbor->priority=(1/confusionSet) * (*node).leftNeighbor->confidentValue;
		} 
		else
		{
			(*node).leftNeighbor->priority=0.0;
		}
	}
	// right neighbor
	if ( (*node).rightNeighbor != nullptr && (*node).rightNeighbor->committed == sendWhichOne ) 
	{
		vector<float>().swap( (*node).rightNeighbor->belief );

		int pLabelNum = (*node).nodeCandidate.size(), qLabelNum = (*node).rightNeighbor->nodeCandidate.size();
		vector<float> tempMsg;
		// 對鄰居的每個candidate傳Msg，並且更新belief
		for ( int i = 0; i < qLabelNum; i++ )
		{
			vector<float>().swap( tempMsg );			
			// 為了判斷此點在Vpq中是否放在row
			if ( (*node).rightEdge->checkRowOrNot( (*node).nodePos ) )  
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).rightEdge->Vpq.at<float>( j, i ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].upperMsg + (*node).nodeCandidate[ j ].lowerMsg + (*node).nodeCandidate[ j ].leftMsg;
					tempMsg.push_back( temp );
				}
			} 
			else
			{
				// 找出nodeP對nodeQ的patchQ最贊成的分數
				for ( int j = 0; j < pLabelNum; j++ ) 
				{
					float temp=0;
					temp = (*node).rightEdge->Vpq.at<float>( i, j ) + (*node).nodeCandidate[ j ].Vp + (*node).nodeCandidate[ j ].upperMsg + (*node).nodeCandidate[ j ].lowerMsg + (*node).nodeCandidate[ j ].leftMsg;
					tempMsg.push_back( temp );
				}
			}
			// send Msg
			(*node).rightNeighbor->nodeCandidate[ i ].leftMsg = sqrtf( *min_element( tempMsg.begin(), tempMsg.end() ) );
			// update belief
			float tempBelief=0.0-(*node).rightNeighbor->nodeCandidate[ i ].Vp - ( (*node).rightNeighbor->nodeCandidate[ i ].leftMsg + (*node).rightNeighbor->nodeCandidate[ i ].rightMsg + (*node).rightNeighbor->nodeCandidate[ i ].upperMsg + (*node).rightNeighbor->nodeCandidate[ i ].lowerMsg );
			(*node).rightNeighbor->belief.push_back( tempBelief );
		}
		// relative belief
		float bMax=0;
		vector <float> relativeBelief;
		relativeBelief = (*node).rightNeighbor->belief;
		int beliefNum=relativeBelief.size();
		bMax=*max_element( relativeBelief.begin(), relativeBelief.end() );
		for ( int k=0 ; k < beliefNum ; k++ )
		{
			relativeBelief.at(k)-=bMax; 
		}
		// update priority
		float confusionSet=0;
		for (int k=0 ; k < beliefNum ; k++ )
		{
			if ( relativeBelief.at(k) >= Bconf )
			{
				confusionSet++;
			}
		}
		if ( confusionSet )
		{
			(*node).rightNeighbor->priority=(1/confusionSet) * (*node).rightNeighbor->confidentValue;
		} 
		else
		{
			(*node).rightNeighbor->priority=0.0;
		}
	}
}

void modifiedPriorityBP::setNodeConfidence()
{
	int n = boundaryNodes.size(), pn = patchSize * patchSize;
	float c;
	Scalar v;
	cMin = 1;
	cMax = 0;
	for ( int i = 0; i < n; i++ )
	{
		v = sum( confidentMap.rowRange( boundaryNodes[ i ].nodePos.y, boundaryNodes[ i ].nodePos.y + patchSize ).colRange( boundaryNodes[ i ].nodePos.x, boundaryNodes[ i ].nodePos.x + patchSize ).clone() );
		c = (float)v.val[ 0 ];
		c /= (float)pn;
		if ( c > cMax )
		{
			cMax = c;
		}
		if ( c < cMin )
		{
			cMin = c;
		}
		boundaryNodes[ i ].confidentValue =  c;
	}
}

void modifiedPriorityBP::updateConfidentMap( int row, int col, float cv )
{
	confidentMap.rowRange( row, row + patchSize ).colRange( col, col + patchSize ).setTo( cv, tempMaskImg.rowRange( row, row + patchSize ).colRange( col, col + patchSize ).clone() );

}

void modifiedPriorityBP::fillMissingRegion( char* fName, bool isModified )
{
	cout << "fillMissingRegion"<< endl;
	outputImg.~Mat();
	Mat tempImg;
	inputImg.copyTo( tempImg );
	vector<Mat> inputCh;
	split( tempImg, inputCh );
	// 根據maskImg把missingRegion部分清零
	if ( !isModified )
	{
		for ( int i = 0; i < imgH; i++ )
		{
			for ( int j = 0; j < imgW; j++ )
			{
				if ( maskImg.at<uchar>( i, j ) == 0 )
				{				
					inputCh[ 0 ].at<uchar>( i, j ) = 0;
					inputCh[ 1 ].at<uchar>( i, j ) = 0;
					inputCh[ 2 ].at<uchar>( i, j ) = 0;				
				}
			}
		}
	}	
	for ( int i = 0; i < boundaryNum; i++ )
	{
		updateConfidentMap( ( *forwardOrder[ i ] ).nodePos.y, ( *forwardOrder[ i ] ).nodePos.x, ( *forwardOrder[ i ] ).confidentValue );
		int absoluteX, absoluteY;
		Mat temp;
		( *forwardOrder[ i ] ).bestPatch.copyTo( temp );
		vector<Mat> tempCh;
		split( temp, tempCh );
		for ( int j = 0; j < patchSize; j++ )
		{
			absoluteY = ( *forwardOrder[ i ] ).nodePos.y + j;
			for ( int k = 0; k < patchSize; k++ )
			{
				absoluteX = ( *forwardOrder[ i ] ).nodePos.x + k;
				if ( tempMaskImg.at<uchar>( absoluteY, absoluteX ) == 0 )
				{
					inputCh[ 0 ].at<uchar>( absoluteY, absoluteX ) = tempCh[ 0 ].at<uchar>( j, k );					
					inputCh[ 1 ].at<uchar>( absoluteY, absoluteX ) = tempCh[ 1 ].at<uchar>( j, k );					
					inputCh[ 2 ].at<uchar>( absoluteY, absoluteX ) = tempCh[ 2 ].at<uchar>( j, k );
					tempMaskImg.at<uchar>( absoluteY, absoluteX ) = 255;
				}
			}
		}			
	}
	merge( inputCh, outputImg );
	outputImg.copyTo( inputImg );
	outputImg.copyTo( SSDImg );
	cvtColor( SSDImg, SSDImg, CV_BGR2HLS );
	imwrite( fName, outputImg );
}