#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <vector>
#include <fstream>
#include <chrono>
#include <unistd.h>
using namespace std;
using namespace cv;
using namespace chrono;

int cam_id = 0;
unsigned long long last_cam_time = 0;
unsigned long long last_send_time = 0;
cv::VideoCapture cap;
std::string information;

struct
{
    int cap_id = 0;
    int cap_w = 0;
    int cap_h = 0;
    int mini_disp = 0;
    int fps = 0;
} cfg;

extern "C"
{
    // ok:0 fail:1
    __declspec(dllexport) int initCam(void)
    {
        cout << "[ CAM QR ] cam qr plugin do init" << endl;

        int i = 0;
        if (access("cam_cfg.txt", F_OK) == 0)
        {
            // cam_cfg.txt exist
            cout << "cam_cfg.txt exist!" << endl;
            fstream cfgFile;
            cfgFile.open("cam_cfg.txt",ios::in);
            cfgFile >> cfg.cap_id;
            cfgFile >> cfg.cap_w;
            cfgFile >> cfg.cap_h;
            cfgFile >> cfg.mini_disp;
            cfgFile >> cfg.fps;
            cfgFile.close();
        }
        else
        {
            // cam_cfg.txt not exist
            cout << "making cam_cfg.txt!" << endl;
            fstream cfgFile;
            cfgFile.open("cam_cfg.txt",ios::out);
            cfgFile << "0 ";
            cfg.cap_id = 0;
            cfgFile << "640 ";
            cfg.cap_w = 640;
            cfgFile << "480 ";
            cfg.cap_h = 480;
            cfgFile << "0 ";
            cfg.mini_disp;
            cfgFile << "5\n";
            cfg.fps = 5;
            cfgFile.close();
        }

        cout << "camera config:\n";
        cout << "cam id = " << cfg.cap_id << endl;
        cout << "cam width = " << cfg.cap_w << endl;
        cout << "cam height = " << cfg.cap_h << endl;
        cout << "debug display = " << cfg.mini_disp << endl;
        cout << "cam fps = " << cfg.fps << endl;

        cout << "[ CAM QR ] trying to open camera" << endl;

        cap.open(cfg.cap_id);

        if (!cap.isOpened())
        {
            std::cout << "Camera cannot be opened!!\n";
            return 1;
        }
        cout << "[ CAM QR ] Camera is opened!" << endl;

        cap.set(cv::CAP_PROP_FRAME_WIDTH, cfg.cap_w);  // 宽度
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, cfg.cap_h); // 高度

        return 0;
    }

    // data got:0 no qr:1 error:2
    __declspec(dllexport) int getCamQr(unsigned char buf[], int *blen)
    {
        cv::Mat img;
        if (!cap.isOpened())
            return 2;

        bool cam_qr_vaild = 0;
        unsigned long long mse =
            duration_cast<milliseconds>(system_clock::now().time_since_epoch())
                .count();

        if (mse - last_cam_time >= (1000/cfg.fps)) //~ 5fps
        {
            last_cam_time = mse;
            Mat gray, qrcode_bin;
            QRCodeDetector qrcodedetector;
            std::vector<Point> points;
            cap >> img;
            if (cfg.mini_disp)
            {
                imshow("camera", img);
            }
            cvtColor(img, gray, COLOR_BGR2GRAY);
            if (qrcodedetector.detect(gray, points))
            {
                information = qrcodedetector.decode(gray, points, qrcode_bin);
                if (information.length())
                {
                    cam_qr_vaild = 1;
                }
            }
        }

        if (mse - last_send_time <= 5000)
            return 1;
        if (!cam_qr_vaild)
            return 1;

        std::cout << "[ CAM QR ] camera qr vaild, len = " << information.length() << std::endl;
        auto dataSize = information.length();
        *blen = dataSize;
        memcpy(buf, (char *)information.c_str(), dataSize);
        last_send_time = mse;
        return 0;
    }
}