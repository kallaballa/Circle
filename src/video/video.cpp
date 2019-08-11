#include <unistd.h>
#include <vector>
#include <iostream>
#include <limits>
#include <csignal>
#include <opencv2/opencv.hpp>
#include "../lib/canvas.hpp"

constexpr off_t WIDTH = 100;
constexpr off_t HEIGHT = 10;
constexpr size_t MAGNIFICATION = 18;

bool DONE;
static void FINISH(int ignore) {
	DONE = true;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Usage: video <video-file>" << std::endl;
		exit(1);
	}
	cv::VideoCapture capture(argv[1]);
	double fps = capture.get(CV_CAP_PROP_FPS);

	if (!capture.isOpened()) {
		std::cerr << "Error opening video stream or file" << std::endl;
		return -1;
	}
	DONE = false;
	signal(SIGINT, FINISH);

	Canvas* canvas = new Canvas(WIDTH * MAGNIFICATION, HEIGHT * MAGNIFICATION, false);

	SDL_Event event;
	bool first = true;
	cv::Mat frame;
	cv::Mat frameRGBA;
	cv::Mat scaled(HEIGHT, WIDTH, CV_8UC4);
	cv::rectangle(scaled, cv::Point(0, 0), cv::Point(WIDTH,HEIGHT), {0,0,0}, CV_FILLED);

	while (!DONE) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				exit(0);
			}
		}
		capture >> frame;
		cv::cvtColor(frame, frameRGBA, CV_RGB2RGBA);
		if(frame.empty()) {
			capture.set(CV_CAP_PROP_POS_AVI_RATIO, 0);
			continue;
		}

		double scaledW = 0;
		double scaledH = 0;
		double ratioW = frameRGBA.cols / (double)WIDTH;
		double ratioH = frameRGBA.rows / (double)HEIGHT;
		double ratio = std::max(ratioW, ratioH);
		scaledW = frameRGBA.cols  / ratio;
		scaledH = frameRGBA.rows  / ratio;

		cv::resize(frameRGBA, scaled(cv::Rect(0,0,scaledW, scaledH)), {scaledW, scaledH}, 0, 0, cv::INTER_CUBIC);
		cv::Mat tile = scaled(cv::Rect(0,0, scaledW, scaledH));
		for(size_t i = 0; i < floor(WIDTH / scaledW); ++i)
			tile.copyTo(scaled(cv::Rect(scaledW * i,0,scaledW, scaledH)));
		canvas->draw(scaled, MAGNIFICATION);

		usleep(1000000 / fps);
	}

	return 0;
}
