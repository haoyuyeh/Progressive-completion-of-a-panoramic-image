#include "stdafx.h"
#include "edgePBP.h"
using namespace cv;
using namespace std;

bool edgePBP::checkRowOrNot( Point p )
{
	if ( p== row )
	{
		return true;
	} 
	else
	{
		return false;
	}
}