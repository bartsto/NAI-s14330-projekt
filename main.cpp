#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
	VideoCapture camera(0); //pobieranie wideo z kamery
	
	//Zapis obrazu z kamery na tymczasowy obraz
	Mat imgTmp;
	camera.read(imgTmp);
	
	namedWindow("Settings", CV_WINDOW_AUTOSIZE);

	int lowRange[3] = { 0,150,60 };
	int highRange[3] = { 255,255,255 };


	createTrackbar("Low Hue", "Settings", &(lowRange[0]), 255);
	createTrackbar("High Hue", "Settings", &(highRange[0]), 255);

	createTrackbar("Low Saturation", "Settings", &(lowRange[1]), 255);
	createTrackbar("High Saturation", "Settings", &(highRange[1]), 255);

	createTrackbar("Low Value", "Settings", &(lowRange[2]), 255);
	createTrackbar("High Value", "Settings", &(highRange[2]), 255);

	int iLastX = -1;
	int iLastY = -1;

	//Wytworzenie czarnego [szumów] obrazka z parametrami kamery
	Mat imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);


	while (true)
	{
		Mat imgOriginal;

		bool bSuccess = camera.read(imgOriginal); // Odczytaj przechwycony obraz z wideo kamery

		if (!bSuccess)
		{
			cout << "Brak obrazu z video kamery" << endl;
			break;
		}

				flip(imgOriginal, imgOriginal, 1);

		Mat imgHSV;
		
		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Zamiana obrazu z BRG na HSV

		Mat imgThresholded; //obraz szumów

		inRange(imgHSV, Scalar(lowRange[0], lowRange[1], lowRange[2]), Scalar(highRange[0], highRange[1], highRange[2]), imgThresholded); //Threshold, obraz progowy

		//Otwarcie morphological (usuwa, szumy i male obiekty z pierwszego planu)
		erode(imgThresholded, imgThresholded, getStructuringElement(CV_SHAPE_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(CV_SHAPE_ELLIPSE, Size(5, 5)));

		//Zamkniecie morphological (usuwa male dziury z pierwszego planu)
		dilate(imgThresholded, imgThresholded, getStructuringElement(CV_SHAPE_ELLIPSE, Size(5, 5)));
		erode(imgThresholded, imgThresholded, getStructuringElement(CV_SHAPE_ELLIPSE, Size(5, 5)));

		
		// Oblicza wszystkie momenty do trzeciego rzędu wielokąta, wyznacza srodek obiektu
		Moments oMoments = moments(imgThresholded);

		double dM01 = oMoments.m01;
		double dM10 = oMoments.m10;
		double dArea = oMoments.m00;

		// Jezeli szumy powyzej wartosci 8000, to rysuj wartosci
		if (dArea > 8000)
		{
			//wykalkuluj pozycje pilki 
			int posX = dM10 / dArea;
			int posY = dM01 / dArea;

			if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0)
			{
				line(imgLines, Point(posX, posY), Point(iLastX, iLastY), Scalar(0, 255, 0), 2, LINE_AA);
			}

			iLastX = posX;
			iLastY = posY;

		}

		imshow("Thresholded Image", imgThresholded); 

		imgOriginal = imgOriginal + imgLines;
		imshow("Original", imgOriginal);

		Mat src = imgLines;
		Mat gray;
		cvtColor(src, gray, COLOR_BGR2GRAY);
		medianBlur(gray, gray, 5);
		vector<Vec3f> circles;
		HoughCircles(gray, circles, HOUGH_GRADIENT, 1,
			gray.rows / 50,
			100, 30, 1, 40 // ostatnie dwa parametry (min_radius & max_radius)
		);
		for (size_t i = 0; i < circles.size(); i++)
		{
			Vec3i c = circles[i];
			Point center = Point(c[0], c[1]);
			// srodek okregu
			circle(src, center, 1, Scalar(0, 100, 100), 3, LINE_AA);
			// okrag
			int radius = c[2];
			circle(src, center, radius, Scalar(255, 0, 255), 1, LINE_AA);
		}
		imshow("Detected circles", src);


		if (waitKey(30) == 27) //ESC
		{
			break;
		}
	}

	return 0;
}
