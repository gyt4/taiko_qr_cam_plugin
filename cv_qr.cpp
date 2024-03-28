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
unsigned long long last_using_time = 0;
cv::VideoCapture cap;
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
    int close_delay = 0;
} cfg;

extern "C"
{
    void CamUpdate();

    __declspec(dllexport) void Init(void) {
        std::cout << "[ CAM QR ] Begin Init Plugin" << std::endl;

        if (access("cam_cfg.txt", F_OK) == 0) {
            // cam_cfg.txt exist
            std::cout << "[ CAM QR] Found cam_cfg.txt!" << std::endl;
            std::fstream cfgFile;
            cfgFile.open("cam_cfg.txt", ios::in);
            cfgFile >> cfg.cap_id;
            cfgFile >> cfg.cap_w;
            cfgFile >> cfg.cap_h;
            cfgFile >> cfg.mini_disp;
            cfgFile >> cfg.fps;
            cfgFile >> cfg.close_delay;
            cfgFile.close();
        } else {
            // cam_cfg.txt not exist
            std::cout << "[ CAM QR ] Create cam_cfg.txt!" << std::endl;
            std::fstream cfgFile;
            cfgFile.open("cam_cfg.txt", ios::out);
            cfgFile << (cfg.cap_id = 0) << " ";
            cfgFile << (cfg.cap_w = 640) << " ";
            cfgFile << (cfg.cap_h = 480) << " ";
            cfgFile << (cfg.mini_disp = 0) << " ";
            cfgFile << (cfg.fps = 5) << " ";
            cfgFile << (cfg.close_delay = 1500) << std::endl;
            cfgFile.close();
        }

        std::cout << "[ CAM QR ] camera config:" << std::endl;
        std::cout << "[ CAM QR ] cam id = " << cfg.cap_id << std::endl;
        std::cout << "[ CAM QR ] cam width = " << cfg.cap_w << std::endl;
        std::cout << "[ CAM QR ] cam height = " << cfg.cap_h << std::endl;
        std::cout << "[ CAM QR ] debug display = " << cfg.mini_disp << std::endl;
        std::cout << "[ CAM QR ] cam fps = " << cfg.fps << std::endl;
        std::cout << "[ CAM QR ] cam close delay = " << cfg.close_delay << std::endl;

        std::thread mainLoop(CamUpdate);
        mainLoop.detach();
        std::cout << "[ CAM QR ] Init Finished!" << std::endl;
    }

    __declspec(dllexport) void usingQr(void) {
        last_using_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    __declspec(dllexport) bool checkQr(void) {
        if (qr_detected) {
            std::cout << "[ CAM QR ] Found QRCode! " << std::endl;
            qr_detected = false;
            return true;
        }

        return false;
    }

    __declspec(dllexport) int getQr(int len_limit, uint8_t* buffer) {
        if (qr_buffer.size() == 0 || qr_buffer.size() > len_limit) {
            std::cout << "[ CAM QR ] Discard QR, max acceptable len: " << len_limit << ", current len: " << qr_buffer.size() << std::endl;
            return 0;
        }
        std::cout << "[ CAM QR ] GetQRCode length=" << qr_buffer.size() << std::endl;
        last_send_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        memcpy(buffer, qr_buffer.data(), qr_buffer.size());
        return qr_buffer.size();
    }

    __declspec(dllexport) void Exit(void) {
        std::cout << "[ CAM QR ] Exit" << std::endl;
        alive = false;
    }

    void CamUpdate() {
        try {
            cv::Mat img;
            cv::QRCodeDetector qrcodedetector;
            std::vector<cv::Point> points;
            
            int delayMilliseconds = cfg.close_delay;
            while (alive) {
                // std::cout << "[ CAM QR ] Main Loop Sleep for " << delayMilliseconds << "ms" << std::endl;
                if (delayMilliseconds > 0) std::this_thread::sleep_for(std::chrono::milliseconds(delayMilliseconds));
    
                unsigned long long begin = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

                if (begin - last_using_time < cfg.close_delay) {
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
                        cam_opened = true;
                        cout << "[ CAM QR ] Camera is opened!" << endl;
    
                        cap.set(cv::CAP_PROP_FRAME_WIDTH, cfg.cap_w);  // 宽度
                        cap.set(cv::CAP_PROP_FRAME_HEIGHT, cfg.cap_h); // 高度
                    }

                    if (begin - last_send_time > 5000) {
                        last_cam_time = begin;
                        cap >> img;
                        if (cfg.mini_disp) imshow("camera", img);
                        std::string information = qrcodedetector.detectAndDecode(img);
                        if (information.length() > 0) {
                            std::cout << "[ CAM QR ] Camera QR vaild, len = " << information.length() << std::endl;
                            qr_buffer.clear();
                            for (char data : information) {
                                qr_buffer.push_back(static_cast<uint8_t>(data));
                            }
                            qr_detected = true;
                            delayMilliseconds = 5000;
                            continue;
                        }
                    }
    
                    unsigned long long end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                    delayMilliseconds = (1000.0 / cfg.fps) + begin - end;
                    if (delayMilliseconds < 0) delayMilliseconds = 0;
                } else if (cam_opened) {
                    std::cout << "[ CAM QR ] Close Camera " << std::endl;
                    cap.release();
                    cam_opened = false;
                    if (cfg.mini_disp) destroyWindow("camera");
                    delayMilliseconds = cfg.close_delay;
                }
            }
            // 结束后关闭cap并关掉小窗口
            if (cam_opened) {
                std::cout << "[ CAM QR ] Close Camera " << std::endl;
                cap.release();
                cam_opened = false;
                if (cfg.mini_disp) destroyWindow("camera");
            }
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
        }
    }
}
