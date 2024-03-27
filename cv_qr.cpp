#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <vector>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <thread>
using namespace std;
using namespace cv;
using namespace chrono;

int cam_id = 0;
unsigned long long last_cam_time = 0;
unsigned long long last_send_time = 0;
unsigned long long last_send4_time = 0;
cv::VideoCapture cap;
std::string information;
bool cam_opened = false;
bool qr_detected = false;
std::vector<uint8_t> qr_buffer = {};

bool alive = true;

struct {
    int cap_id = 0;
    int cap_w = 0;
    int cap_h = 0;
    int mini_disp = 0;
    int fps = 0;
} cfg;

extern "C"
{
    void CamUpdate();

    __declspec(dllexport) void Init(void) {
        std::cout << "[ CAM QR ] Begin Init Plugin" << std::endl;

        if (access("cam_cfg.txt", F_OK) == 0) {
            // cam_cfg.txt exist
            std::cout << "cam_cfg.txt exist!" << std::endl;
            std::fstream cfgFile;
            cfgFile.open("cam_cfg.txt", ios::in);
            cfgFile >> cfg.cap_id;
            cfgFile >> cfg.cap_w;
            cfgFile >> cfg.cap_h;
            cfgFile >> cfg.mini_disp;
            cfgFile >> cfg.fps;
            cfgFile.close();
        } else {
            // cam_cfg.txt not exist
            std::cout << "making cam_cfg.txt!" << std::endl;
            std::fstream cfgFile;
            cfgFile.open("cam_cfg.txt", ios::out);
            cfgFile << (cfg.cap_id = 0) << " ";
            cfgFile << (cfg.cap_w = 640) << " ";
            cfgFile << (cfg.cap_h = 480) << " ";
            cfgFile << (cfg.mini_disp = 0) << " ";
            cfgFile << (cfg.fps = 5) << std::endl;
            cfgFile.close();
        }

        std::cout << "camera config:\n";
        std::cout << "cam id = " << cfg.cap_id << std::endl;
        std::cout << "cam width = " << cfg.cap_w << std::endl;
        std::cout << "cam height = " << cfg.cap_h << std::endl;
        std::cout << "debug display = " << cfg.mini_disp << std::endl;
        std::cout << "cam fps = " << cfg.fps << std::endl;

        std::thread mainLoop(CamUpdate);
        mainLoop.detach();
        std::cout << "[ CAM QR ] Init Finished!" << std::endl;
    }

    __declspec(dllexport) void usingQr(void) {
        last_send4_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    __declspec(dllexport) bool checkQr(void) {
        if (qr_detected) {
            std::cout << "[ CAM QR ] Found QRCode! " << std::endl;
            qr_detected = false;
            return true;
        }

        return false;
    }

    __declspec(dllexport) std::vector<uint8_t> getQr(void) {
        std::cout << "[ CAM QR ] GetQRCode length=" << qr_buffer.size() << std::endl;
        return qr_buffer;
    }

    // __declspec(dllexport) void Exit(void) {
    //     std::cout << "[ CAM QR ] Exit" << std::endl;
    // }

    void CamUpdate() {
        int delayMilliseconds = 1000.0 / cfg.fps;
        while (alive) {
            std::cout << "[ CAM QR ] Main Loop Sleep for " << delayMilloseconds << "ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMilliseconds));

            unsigned long long mse =
            duration_cast<milliseconds>(system_clock::now().time_since_epoch())
                .count();

            if (mse - last_send4_time < 1500) {
                if (!cam_opened) {
                    std::cout << "[ CAM QR ] Opening camera..." << std::endl;

                    cap.open(cfg.cap_id);

                    if (!cap.isOpened()) {
                        cam_opened = false;
                        std::cout << "[ CAM QR ] Camera cannot be opened!!" << std::endl;
                        std::cout << "[ CAM QR ] Retry is Scheduled After 5s!!" << std::endl;
                        delayMilliseconds = 5000;
                        continue;
                    }
                    cam_opened = 1;
                    cout << "[ CAM QR ] Camera is opened!" << endl;

                    cap.set(cv::CAP_PROP_FRAME_WIDTH, cfg.cap_w);  // 宽度
                    cap.set(cv::CAP_PROP_FRAME_HEIGHT, cfg.cap_h); // 高度
                }

                if (mse - last_send_time > 5000) {
                    last_cam_time = mse;
                    cv::Mat img, gray, qrcode_bin;
                    cv::QRCodeDetector qrcodedetector;
                    std::vector<Point> points;
                    cap >> img;
                    if (cfg.mini_disp) {
                        imshow("camera", img);
                    }
                    cvtColor(img, gray, COLOR_BGR2GRAY);
                    if (qrcodedetector.detect(gray, points)) {
                        information = qrcodedetector.decode(gray, points, qrcode_bin);
                        if (information.length()) {
                            std::cout << "[ CAM QR ] camera qr vaild, len = " << information.length() << std::endl;
                            qr_buffer.clear();
                            for (char data : information) {
                                qr_buffer.push_back(static_cast<uint8_t>(data));
                            }
                            qr_detected = true;
                            delayMilliseconds = 5000;
                            continue;
                        }
                    }
                }

                delayMilliseconds = 1000.0 / cfg.fps;
            } else if (cam_opened) {
                std::cout << "[ CAM QR ] after send4 too long closing camera " << std::endl;
                cap.release();
                cam_opened = 0;
                if (cfg.mini_disp)
                    destroyWindow("camera");
                delayMilliseconds = 1500;
            }
        }
    }
}
