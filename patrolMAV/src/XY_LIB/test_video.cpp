#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/video.hpp"
#include "getTargetLoc.hpp"
#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;
using namespace cv;

int main()
{
	cv::VideoCapture cap("20160311150924.avi");
	//cap.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	//cap.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
	//waitKey(2000);
	cv::VideoWriter dataWriter("data.avi",  CV_FOURCC('M','J','P','G'), 5, Size(1280, 720), true);
	cv::VideoWriter resultWriter("result.avi", CV_FOURCC('M','J','P','G'), 5, Size(640, 360), true);

	xyVision::GetTarget sample("config_1_10.ini");
    //sample.setTimeModel(xyVision::NIGHT);

	double rate = cap.get(CV_CAP_PROP_FPS);
	cout << "frame rate " << rate;
	int rate_int = (int)rate;
	int n_frames = (int)cap.get(CV_CAP_PROP_FRAME_COUNT);
	namedWindow("result");
	cv::Mat img;
	cv::Mat rectified;
	int k = 200;
	while(1)
	{
		//if (k > 10000)
		//	break;
		////if (k == 21)
		////{
		////	sample.clearStates();
		////}
		//char ch[10];
		//sprintf(ch, "%.5d", k);
		//string file_name = "E:\\lbs\\xunyi\\data\\image_2_18\\2016-2-18 (1)\\image-1\\" + string(ch) + ".jpg";
		//img = imread(file_name);
		k = k + 1;

	    cap >> img;
		dataWriter << img;
		if (img.empty())	break;
		if(waitKey(20) >= 0) break;
		sample << img;
		rectified = sample.rectified;
		if (sample.isDetected)
		{
			// draw rotated rectangle
			Point2f vertices[4];
			sample.box.points(vertices);
            if (sample.targetInfo.targetVersion == 1 || sample.targetInfo.targetVersion == 2)
            {
                line(rectified, Point((int)vertices[0].x, (int)vertices[0].y), Point((int)vertices[1].x, (int)vertices[1].y), Scalar( 255, 0, 0 ), 3);
                line(rectified, Point((int)vertices[1].x, (int)vertices[1].y), Point((int)vertices[2].x, (int)vertices[2].y), Scalar( 255, 0, 0 ), 3);
                line(rectified, Point((int)vertices[2].x, (int)vertices[2].y), Point((int)vertices[3].x, (int)vertices[3].y), Scalar( 255, 0, 0 ), 3);
                line(rectified, Point((int)vertices[3].x, (int)vertices[3].y), Point((int)vertices[0].x, (int)vertices[0].y), Scalar( 255, 0, 0 ), 3);
            }
			if (sample.targetInfo.targetVersion == 3)
			{
                Mat center = Mat(sample.getCurrentLoc());
                if (center.rows != 3)
                {
                    center = center.t();
                }
                Mat centerPixel = Mat(sample.cameraInfo.newCameraMatrix) * (center / center.at<float>(2));
                circle(rectified, Point((int)centerPixel.at<float>(0), (int)centerPixel.at<float>(1)), 15, Scalar(255, 0, 0), 3);
			}
			// show location
			Point3f location = sample.getCurrentLoc();
			char ch[30];
			sprintf(ch, "%.2f", location.x);
			putText(rectified, "X = " + string(ch), Point(200, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
			sprintf(ch, "%.2f", location.y);
			putText(rectified, "Y = " + string(ch), Point(200, 100), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
			sprintf(ch, "%.2f", location.z);
			putText(rectified, "Z = " + string(ch), Point(200, 170), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
			sprintf(ch, "%d", sample.frameCounter);
			putText(rectified, "FrameCounter = " + string(ch), Point(200, 250), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
			resultWriter << rectified.clone();
			imshow("result", rectified);

		}
		else
		{
			putText(rectified, "do not detect! ", Point(400, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 1);
			char ch[30];
			sprintf(ch, "%d", sample.frameCounter);
			putText(rectified, "FrameCounter = " + string(ch), Point(200, 250), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
			resultWriter << rectified.clone();
			imshow("result", rectified);
		}

	}
}