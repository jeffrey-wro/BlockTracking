#include <iostream>
#include <opencv2/opencv.hpp>

#include "MyRio.h"
#include "I2C.h"
#include "ImageSender.h"
#include "Motor_Controller.h"
#include "Utils.h"

using namespace std;
using namespace cv;

#define ENABLE_SERVER 1

extern NiFpga_Session myrio_session;
NiFpga_Status status;

int main()
{

    Mat test;
    cout << "Nicks program!" << endl;

#if ENABLE_SERVER
    ImageSender imageSender;

    cout << "Initilizating Image sender - start client now" << endl;
    if (imageSender.init() < 0)
    {
        cerr << "Image sender Initilization failed" << endl;
        ;
        return EXIT_FAILURE;
    }
    else
    {
        cout << "Image sender Started..." << endl;
    }
#endif

    VideoCapture capWebcam(0); // open the default camera
    if (!capWebcam.isOpened())
    {
        cerr << "Webcam initilization failed" << endl;
        return -1;
    }
    else
    {
        cout << "Webcame Started..." << endl;
    } // check if we succeeded

    status = MyRio_Open();
    if (MyRio_IsNotSuccess(status))
    {
        cerr << "Unable to open MyRIO Session" << endl;
        return status;
    }
    else
    {
        cout << "Opened Session to FPGA Personanilty..." << endl;
    }

    MyRio_I2c i2c;
    status = Utils::setupI2CB(&myrio_session, &i2c);

    Motor_Controller mc = Motor_Controller(&i2c);
    mc.controllerEnable(DC);

    int volt = mc.readBatteryVoltage(1);
    cout << "MCController - Battery Voltage: " << volt << endl;

    Mat frame;
    Mat hsv;
    Mat mask;
    Mat result;
    Mat thresh;

    vector<vector<Point>> contours; // Vector for storing contour
    vector<Vec4i> hierarchy;

    for (;;)
    {
        capWebcam >> frame;

        cvtColor(frame, hsv, COLOR_BGR2HSV);

        inRange(hsv, Scalar(20, 50, 100), Scalar(45, 255, 255), mask);

        threshold(mask, thresh, 40, 255, 0);

        findContours(thresh, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE); // Find the contours in the image

        int largest_area = 0;
        int largest_contour_index = 0;
        Rect bounding_rect;

        for (size_t i = 0; i < contours.size(); i++) // iterate through each contour.
        {
            double a = contourArea(contours[i], false); //  Find the area of contour
            if (a > largest_area)
            {
                largest_area = a;
                largest_contour_index = i;                 //Store the index of largest contour
                bounding_rect = boundingRect(contours[i]); // Find the bounding rectangle for biggest contour
            }
        }

        Scalar color(255, 255, 255);
        //drawContours(frame, contours, largest_contour_index, color, FILLED, 8, hierarchy); // Draw the largest contour using previously stored index.
        rectangle(frame, bounding_rect, Scalar(0, 255, 0), 1, 8, 0);

        int recCtrX = bounding_rect.x + bounding_rect.width / 2;
        int recCtrY = bounding_rect.y + bounding_rect.height / 2;

        circle(frame, Point(recCtrX, recCtrY), (int)3, Scalar(255, 0, 0), 2);

        int speed = 60;

        int percent = 40;
        int leftThreshold = frame.cols / 100.0 * percent;
        int rigthThreshold = frame.cols - (frame.cols / 100.0 * percent);

        line(frame, Point(leftThreshold, 0), Point(leftThreshold, frame.rows), Scalar(255, 0, 0), 2);
        line(frame, Point(rigthThreshold, 0), Point(rigthThreshold, frame.rows), Scalar(255, 0, 0), 2);

        cout << "Rectangle - X: " << recCtrX << " Y: " << recCtrY << endl;

        if (recCtrX < leftThreshold)
        {
            mc.setMotorSpeeds(DC, speed, speed);
            //cout << "Left";
        }
        else if (recCtrX > rigthThreshold)
        {
            mc.setMotorSpeeds(DC, -1 * speed, -1 * speed);
            //cout << "Right";
        } else {
            mc.setMotorSpeeds(DC, 0, 0);
        }
        // else
        // {
        //     //cout << "Forward";
        //     mc.setMotorSpeeds(DC, speed, -1 * speed);
        // }

#if ENABLE_SERVER
        imageSender.send(&frame);
#endif
    }
}