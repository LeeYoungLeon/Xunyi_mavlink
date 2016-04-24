#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "getTargetLoc.hpp"
#include <stdio.h>
#include <iostream>
#include <time.h>

using namespace cv;
using namespace std;

int main()
{
	clock_t start,finish;
	double totaltime;

	xyVision::GetTarget sample("config_1_10.ini");
	//sample.useGPU = true;
	int nTest = 100;
	
	//Mat img = cv::imread("no.png");
	Mat img = cv::imread("1_21.jpg");
	start=clock();
	for (int k = 0; k < nTest; k++)
	{
		sample << img;
		if (!sample.isDetected )
			cout << "no target !" << endl;
		//cout << sample.frameCounter << endl;
	}
	finish = clock();
	totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
    cout << "Target Location" << endl;
	cout << Mat(sample.getCurrentLoc())<<endl;

	cout << "Average running time for a frame " << totaltime / nTest << endl;
	getchar();

}
