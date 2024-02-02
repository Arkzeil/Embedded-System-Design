#include <fstream>
#include <sstream>
#include <iostream>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <fcntl.h>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/ioctl.h>
#include <opencv2/imgcodecs.hpp>
#include <stdint.h>
#include <unistd.h>     // for sleep
#include <stdio.h>      // for sprintf_s
#include <termios.h>    // termios, TCSANOW, ECHO, ICANON
#include <pthread.h>
#include <signal.h>

using namespace cv;
using namespace dnn;
using namespace std;

struct Net_config{
	string classesFile;  // the file of classes of that model(coco.name in this case)
	string modelConfiguration; // the model configuration file for darknet
	string modelWeights;       // the model weight file
	string netname;            // choose which model to use, yolov3 or yolo_fastest
	float confThreshold; // threshold of confidence, higher means that less object will be 
	float nmsThreshold;  // Non-maximum suppression threshold, used to suppress nearby detection when there're too much of detected objects in a region  
	int inpWidth;        // Width of network's input image
	int inpHeight;       // Height of network's input image
};

// choose which pre-trained model to use 
Net_config yolo_model_nets[2] = {	
	{"coco.names", "yolov3.cfg", "yolov3.weights", "yolov3", 0.5, 0.4, 416, 416},						// {confThreshold, nmsThreshold, inpWidth, inpHeight, classesFile, modelConfiguration, modelWeights, netname}
	{"coco.names", "yolo-fastest-xl.cfg", "yolo-fastest-xl.weights", "yolo-fastest", 0.3, 0.4, 960, 960}
};

class YOLO{							// yolo class, handle all operations of object detection
	public:
		YOLO(Net_config config);    // constructor of yolo, to get pre-trained model and other parameters
		void detect(Mat& frame);	// to conduct image preprocess, forward conduction, postprocess, and draw detection result on image 
	private:
		float confThreshold;
		float nmsThreshold;
		int inpWidth;
		int inpHeight;				
		char netname[20];			
		vector<string> classes;		// same definition as in 'Net_config' for above private member variable
		Net net;					// used for constructor to initialize member variables
		void postprocess(Mat& frame, const vector<Mat>& outs);											// Get final detection result based on confidence threahold and Non-maximum suppression threshold
		void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame);	// draw square box and put text of object name and its confidence value
};

int main(){
	YOLO yolo_model(yolo_model_nets[1]);		// choose a pre-trained model
	string imgpath = "example.png";				// specify input image
	string output_path = "example_od.png";		// specify output path of result
	Mat srcimg = imread(imgpath);				// read image in BGR format
	yolo_model.detect(srcimg);					// run yolo

	imwrite(output_path, srcimg);				// write result to given path

	return 0;
}

YOLO::YOLO(Net_config config){						// constructor of yolo
	ifstream ifs(config.classesFile.c_str());		// read coco class file
	string line;
	this->confThreshold = config.confThreshold;		
	this->nmsThreshold = config.nmsThreshold;
	this->inpWidth = config.inpWidth;
	this->inpHeight = config.inpHeight;				
	strcpy(this->netname, config.netname.c_str());	// refer to 'Net_config' for explaintion of variables

	cout << "Net use " << config.netname << endl;	// print out the chosen model

	while(getline(ifs, line)) 						// store the content of class file into vector
		this->classes.push_back(line);				
	
	this->net = readNetFromDarknet(config.modelConfiguration, config.modelWeights);		// Reads a network model stored in Darknet model files.
	this->net.setPreferableBackend(DNN_BACKEND_OPENCV);									// Ask network to use specific computation backend(opencv_dnn) where it supported.
	this->net.setPreferableTarget(DNN_TARGET_CPU);										// Ask network to make computations on specific target device.(CPU)
}

void YOLO::detect(Mat& frame){
	// blob ref:https://zhuanlan.zhihu.com/p/150821331
	Mat blob;
	vector<Mat> outs;
	vector<double> layersTimes;
	// Creates 4-dimensional blob from image. (input, output, scale, resize, rgb scalar, swapRB, crop)
	// scale is to scale pixel value, size indicates the size put into network, scalar is to subtract RGB value, swapRB:BGR->RGB, crop will crop 'size' large image from image center
	blobFromImage(frame, blob, 1 / 255.0, Size(this->inpWidth, this->inpHeight), Scalar(0, 0, 0), true, false);
	cout << "blobFromImage" << endl;
	// put processed mat into network
	this->net.setInput(blob);
	cout << "net.setInput" << endl;
	// Runs forward pass to compute output of layer listed in 'getUnconnectedOutLayersNames()'(output layer actually?)
	this->net.forward(outs, this->net.getUnconnectedOutLayersNames());
	cout << "net.forward" << endl;
	// refer to 'postprocess' function
	this->postprocess(frame, outs);

	// put text about inference time onto image
	double freq = getTickFrequency() / 1000;
	double t = net.getPerfProfile(layersTimes) / freq;
	string label = format("%s Inference time : %.2f ms", this->netname, t);
	putText(frame, label, Point(0, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
}

void YOLO::postprocess(Mat& frame, const vector<Mat>& outs){   // Remove the bounding boxes with low confidence using non-maxima suppression
	vector<int> classIds;
	vector<float> confidences;
	vector<Rect> boxes;

	for (size_t i = 0; i < outs.size(); ++i){
		// Scan through all the bounding boxes output from the network and keep only the
		// ones with high confidence scores. Assign the box's class label as the class
		// with the highest score for the box.
		float* data = (float*)outs[i].data;
		for(int j = 0; j < outs[i].rows; ++j, data += outs[i].cols){
			Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
			Point classIdPoint;
			double confidence;
			// Get the value and location of the maximum score
			minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
			if(confidence > this->confThreshold){
				int centerX = (int)(data[0] * frame.cols);
				int centerY = (int)(data[1] * frame.rows);
				int width = (int)(data[2] * frame.cols);
				int height = (int)(data[3] * frame.rows);
				int left = centerX - width / 2;
				int top = centerY - height / 2;

				classIds.push_back(classIdPoint.x);
				confidences.push_back((float)confidence);
				boxes.push_back(Rect(left, top, width, height));
			}
		}
	}

	// Perform non maximum suppression to eliminate redundant overlapping boxes with lower confidences
	vector<int> indices;
	// Performs non maximum suppression given boxes and corresponding scores.
	NMSBoxes(boxes, confidences, this->confThreshold, this->nmsThreshold, indices);
	for(size_t i = 0; i < indices.size(); ++i){
		int idx = indices[i];
		Rect box = boxes[idx];
		this->drawPred(classIds[idx], confidences[idx], box.x, box.y,
			box.x + box.width, box.y + box.height, frame);
	}
}

void YOLO::drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame){   // Draw the predicted bounding box
	int baseLine;
	// Get the label for the class name and its confidence
	string label = format("%.2f", conf);
	// Draw a rectangle displaying the bounding box
	rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 0, 255), 3);

	if(!this->classes.empty()){
		CV_Assert(classId < (int)this->classes.size());
		label = this->classes[classId] + ":" + label;
	}

	// Display the label at the top of the bounding box
	Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
	top = max(top, labelSize.height);
	//rectangle(frame, Point(left, top - int(1.5 * labelSize.height)), Point(left + int(1.5 * labelSize.width), top + baseLine), Scalar(0, 255, 0), FILLED);
	putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 255, 0), 1);
}
