#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/videoio.hpp"

#include <iostream>
#include <stdexcept>

using namespace std;
using namespace cv;

/*
 *
 * * Classes
 * * Como melhorar a detecção do punho na imagem: filtrar pela cor da mão + pela
 * forma da mão. pegar a imagem de ROIS localizadas nas áreas adjacentes
 *
 *
 *
 *
 *
 *
 *
 */

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

  unsigned long long int framesLeft = 0;
  unsigned long long int framesRight = 0;

  void drawTransRect(Mat frame, Scalar color, double alpha, Rect region) {
    Mat roi = frame(region);
    Mat rectImg(roi.size(), CV_8UC3, color);
    addWeighted(rectImg, alpha, roi, 1.0 - alpha, 0, roi);
  }

  void processFrame(Mat frame) {
    flip(frame, frame, 1);
    processedFrame = frame.clone();

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

    for (Rect &r : fists) {
      rectangle(
          frame, Point(cvRound(r.x + 1), cvRound(r.y)),
          Point(cvRound((r.x + r.width - 1)), cvRound((r.y + r.height - 1))),
          color, 3);

      handleIntersection(r);
    }

    drawTransRect(frame, Scalar(128, 0, 0), 0.5, leftRect);
    drawTransRect(frame, Scalar(0, 0, 128), 0.5, rightRect);

    putText(frame, "Taiko no Tatsujin!", Point(200, 80), FONT_HERSHEY_SIMPLEX,
            1, Scalar(0, 0, 0), 4);
  }

  void showFrames(Mat &frame) {
    // imshow("Processed Frame", processedFrame);
    imshow("Normal Frame", frame);
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

class Pontuacao {};


// class FileHandler {
//   static vector<string> split(const string &str, const char &delim) {
//     stringstream ss(str);
//     vector<string> tokens;
//     string token;
//
//     while (getline(ss, token, delim)) {
//         tokens.push_back(token);
//     }
//
//     return tokens;
// }
// public:
//     FileHandler() = default;
//
//     static Pokedex* readFile(const string &fileName) {
//         ifstream file(fileName.c_str());
//         const auto pokedex = new Pokedex();
//
//         if (!file.is_open()) {
//             // cerr << "Erro ao abrir o arquivo de entrada." << endl;
//             return pokedex;
//         }
//
//         string line;
//         int lastId;
//
//         getline(file, line);
//         getline(file, line);
//
//         if (line.empty()) {
//             return {};
//         }
//
//         vector<string> tokens;
//
//         tokens = split(line, ',');
//
//         file.close();
//
//         pokedex->nextId = lastId + 1;
//
//         return pokedex;
//     }
//
//     static void writeFile(const string &fileName, const Pokedex &pokedex) {
//         ofstream file(fileName.c_str(), ios::trunc);
//
//         if (!file.is_open()) {
//             cerr << "Erro ao abrir arquivo pokedex.csv para salvar a pokédex." << endl;
//         }
//
//         file << "Number of Left Clicks, Left of Right Clicks:q" << endl;
//
//         pokedex.saveToFile(file);
//
//         file.close();
//     }
// };

string wName = "Taiko";

int main(int argc, const char **argv) {

  // system("mplayer
  // /home/aluno/Downloads/Projeto-Derzu-LuArCla/BARRA-DE-FERRO-CAINDO-ESTOURADO.mp3
  // &");

  char key = 0;

  string cascadeName = "fist.xml";

  CapturaVideo cap;

  try {
    cap = CapturaVideo(2); // Open the webcam
  } catch (const cameraException &e) {

    cerr << "Error: " << e.what() << endl;

    return -1;
  }

  auto proc = ProcessamentoImg(cascadeName);

  if (cap.isOpened()) {
    namedWindow(wName, WINDOW_KEEPRATIO);
    while (1) {
      Mat frame = cap.getFrame();
      // resize(frame, frame, Size(1280,720), 0, 0, INTER_LINEAR_EXACT); //
      // Resize the frame to a fixed size

      if (frame.empty())
        break;
      if (key == 0) // just first time
        resizeWindow(wName, frame.cols, frame.rows);

      proc.detectAndDraw(frame);
      proc.showFrames(frame);

      key = (char)waitKey(10);
      if (key == 27 || key == 'q' || key == 'Q')
        break;
    }
  }

  return 0;
}
