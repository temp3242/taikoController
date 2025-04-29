#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/videoio.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace cv;

/*
 * Ideias:
 *
 * Como melhorar a detecção do punho na imagem: filtrar pela cor da mão + pela
 * forma da mão. pegar a imagem de ROIS localizadas nas áreas adjacentes
 */

string wName = "Taiko";

class cameraException : public runtime_error {
public:
    // Constructor for string-based error message (e.g., file error)
    cameraException(const string &file)
            : runtime_error("Error opening file: " + file) {}

    // Constructor for integer-based error message (e.g., camera ID error)
    cameraException(int src)
            : runtime_error("Error opening camera #" + to_string(src)) {}
};

class ProcessamentoImg {

    CascadeClassifier cascade;
    Mat processedFrame;

    Rect leftRect = Rect(50, 100, 300, 200);
    Rect rightRect = Rect(450, 100, 300, 200);

    void drawTransRect(Mat frame, Scalar color, double alpha, Rect region) {
        Mat roi = frame(region);
        Mat rectImg(roi.size(), CV_8UC3, color);
        addWeighted(rectImg, alpha, roi, 1.0 - alpha, 0, roi);
    }

    void processFrame(Mat frame) {
        flip(frame, frame, 1);
        processedFrame = frame.clone();

        processedFrame = processedFrame(Rect(0, 100, 800, 300));

        cvtColor(processedFrame, processedFrame, COLOR_BGR2GRAY);
        equalizeHist(processedFrame, processedFrame);
    }

    void handleIntersection(Rect r) {

        if ((r & leftRect).area() > 0) {
            system("ydotool click C0 > /dev/null 2>&1"); // left click
            framesLeft++;
        }
        if ((r & rightRect).area() > 0) {
            system("ydotool click C1 > /dev/null 2>&1"); // right click
            framesRight++;
        }
    }

public:
    unsigned long long int framesLeft = 0;
    unsigned long long int framesRight = 0;
    unsigned long long int highscore = 0;

    ProcessamentoImg(string cascadeName) {
        if (!cascade.load(cascadeName)) {
            cout << "ERROR: Could not load classifier cascade: " << cascadeName
                 << endl;
        }
    }

    void detectAndDraw(Mat &frame) {

        processFrame(frame);

        vector<Rect> fists;
        Scalar color = Scalar(255, 0, 0);

        cascade.detectMultiScale(processedFrame, fists, 1.3, 2,
                                 0
                                 //|CASCADE_FIND_BIGGEST_OBJECT
                                 //|CASCADE_DO_ROUGH_SEARCH
                                 | CASCADE_SCALE_IMAGE);

        for (Rect &r: fists) {
            rectangle(
                    frame, Point(cvRound(r.x + 1), cvRound(r.y)),
                    Point(cvRound((r.x + r.width - 1)), cvRound((r.y + r.height - 1))),
                    color, 3);

            handleIntersection(r);
        }

        drawTransRect(frame, Scalar(128, 0, 0), 0.5, leftRect);
        drawTransRect(frame, Scalar(0, 0, 128), 0.5, rightRect);

        putText(frame, "Taiko no Tatsujin!", Point(250, 80), FONT_HERSHEY_SIMPLEX,
                1, Scalar(0, 0, 0), 4);

        putText(frame, "Points: " + to_string((framesLeft + framesRight) / 2),
                Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 0, 0), 4);

        putText(frame, "High Score: " + to_string(highscore), Point(0, 8),
                FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 0, 0), 4);
    }

    void showFrames(Mat &frame) {
        // imshow("Processed Frame", processedFrame);
        imshow(wName, frame);
    }
};

class CapturaVideo {
    VideoCapture capture;

public:
    CapturaVideo(string src) {
        if (!capture.open(src))
            throw cameraException(src); // fail report stuff
    }

    CapturaVideo(int src) {
        if (!capture.open(src))
            throw cameraException(src); // fail report stuff
    }

    CapturaVideo() = default; // Default constructor

    bool isOpened() {
        return capture
                .isOpened(); // Check if the video capture is opened successfully
    }

    Mat getFrame() {
        Mat frame;
        capture >> frame; // Read a new frame from video
        return frame;
    }
};

class FileHandler {

public:
    FileHandler() = default;

    static void savePoints(const string &path, unsigned long long int points) {
        ofstream file(path);

        if (!file.is_open()) {
            cerr << "Error opening file: " << path << endl;
            return;
        }

        file << points;
        file.close();
    }

    static unsigned long long int loadPoints(const string &path) {
        ifstream file(path);

        if (!file.is_open()) {
            cerr << "Error opening file: " << path << endl;
            return 0;
        }
        string line;
        unsigned long long int points;
        getline(file, line);
        points = stoull(line);
        file.close();
        return points;
    }
};


bool confirmExit(CapturaVideo &cap) {
    while (true) {

        Mat frame = cap.getFrame();
        if (frame.empty())
            break;

        flip(frame, frame, 1);

        // write "Tem certeza que deseja sair?" on the frame and "Pressione Enter para continuar ou Q para sair" below it
        putText(frame, "Tem certeza que deseja sair?", Point(200, 150), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0), 4);
        putText(frame, "Pressione Enter para continuar ou Q para sair", Point(100, 250), FONT_HERSHEY_SIMPLEX, 0.85,
                Scalar(0, 0, 0), 4);

        imshow(wName, frame);

        char key = (char) waitKey(10); // Wait for a key press indefinitely
        if (key == 'q' || key == 'Q' || key == 13) {
            if (key == 13) {
                return false;
            }
            string command = "mplayer ";
            command += filesystem::current_path();
            command += "/narutoflautatriste.mp3 &";
            system(command.c_str());
            return true;
        }
    }
    
    return true;
}

int main(int argc, const char **argv) {

    char key = 0;

    string cascadeName = "fist.xml";

    CapturaVideo cap;

    try {
        cap = CapturaVideo(0); // Open the webcam
    } catch (const cameraException &e) {

        cerr << "Error: " << e.what() << endl;

        return -1;
    }

    auto proc = ProcessamentoImg(cascadeName);

    unsigned long long int highScore = FileHandler::loadPoints("highscore.txt");
    proc.highscore = highScore;

    if (cap.isOpened()) {
        namedWindow(wName, WINDOW_KEEPRATIO);
        while (1) {
            Mat frame = cap.getFrame();
            if (frame.empty())
                break;
            
            if (key == 0) resizeWindow(wName, frame.cols, frame.rows);

            flip(frame, frame, 1);

            putText(frame, "Taiko no Tatsujin!", Point(250, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0), 4);
            putText(frame, "Pressione Enter para comecar", Point(200, 150), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0),
                    4);
            imshow(wName, frame);

            key = (char) waitKey(10); // Wait for a key press indefinitely
            if (key == 13) {
                string command = "mplayer ";
                command += filesystem::current_path();
                command += "/BARRA-DE-FERRO-CAINDO-ESTOURADO.mp3 &";
                system(command.c_str());
                break;
            } else if (key == 27 || key == 'q' || key == 'Q') {
                bool exit = confirmExit(cap);
                if (exit) {
                    return 0;
                }
            }
        }

        while (1) {
            Mat frame = cap.getFrame();

            if (frame.empty())
                break;

            proc.detectAndDraw(frame);
            proc.showFrames(frame);

            key = (char) waitKey(10);
            if (key == 27 || key == 'q' || key == 'Q') {
                bool exit = confirmExit(cap);
                if (exit)
                    break;
            }
        }

        if (highScore < (proc.framesLeft + proc.framesRight) / 2) {
            highScore = (proc.framesLeft + proc.framesRight) / 2;
            FileHandler::savePoints("highscore.txt", highScore);
        }
    }

    return 0;
}
