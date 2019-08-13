#include <unistd.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <limits>
#include <thread>
#include <csignal>
#include <chrono>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "../lib/canvas.hpp"
#include "../lib/color.hpp"
#include "../lib/recorder.hpp"

#include "aquila/global.h"
#include "aquila/functions.h"
#include "aquila/transform/FftFactory.h"
#include "aquila/source/FramesCollection.h"
#include "aquila/tools/TextPlot.h"

constexpr off_t WIDTH = 100;
constexpr off_t HEIGHT = 10;
constexpr size_t MAGNIFICATION = 18;
constexpr size_t FFT_SIZE = 128;

using namespace std::chrono;

struct AudioSettings {
  bool normalize;
  double sampleRate;
  size_t samples;
};

AudioSettings AS = {true, 8000, 256};


typedef std::vector<double> AudioWindow;
std::mutex abmtx;
AudioWindow audio_buffer;

bool DONE;
static void FINISH(int ignore) {
	DONE = true;
}

void blend(const cv::Mat& src1, const cv::Mat& src2, const double alpha,
		const cv::Mat& dst) {
	using namespace cv;
	double beta;

	beta = (1.0 - alpha);
	addWeighted(src1, alpha, src2, beta, 0.0, dst);
}

cv::Mat last;
void render(std::vector<double>& absSpectrum,
		cv::Mat& target) {
	cv::rectangle(target, cv::Point(0, 0), cv::Point(WIDTH,HEIGHT), {0,0,0}, CV_FILLED);
	double max = 0;
	for(size_t i = 0; i < absSpectrum.size(); ++i) {
		max = std::max(absSpectrum[i], max);
	}

//	std::cerr << max << std::endl;
	if(std::round(max) < 100)
		return;
	for(size_t i = 0; i < absSpectrum.size(); ++i) {
		HSLColor hsl;
		hsl.h_ = i / (double)absSpectrum.size() * 360.0;
		hsl.s_ = 99;
		hsl.l_ = 50 - (absSpectrum[i] * 50);

		RGBColor rgb(hsl);
		double w = WIDTH/((double)FFT_SIZE / 4);
		if(absSpectrum[i] > 1)
			cv::rectangle(target, cv::Point((i - 1) * w,0), cv::Point((i - 1) * w + w - 1,HEIGHT), cv::Scalar(rgb.b_,rgb.g_,rgb.r_), CV_FILLED);
	}
	if(!last.empty()) {
		last(cv::Rect(0,0, last.cols, last.rows - 1)).copyTo(target(cv::Rect(0,1,target.cols, target.rows - 1)));
	}
	target.copyTo(last);
}
int main(int argc, char** argv) {
	if (argc != 1) {
		std::cerr << "Usage: water" << std::endl;
		exit(1);
	}
	DONE = false;
	signal(SIGINT, FINISH);

	Canvas* canvas = new Canvas(WIDTH * MAGNIFICATION, HEIGHT * MAGNIFICATION, false);


	using namespace Aquila;
  auto signalFFT = FftFactory::getFft(FFT_SIZE);
  std::mutex fbmtx;
	cv::Mat* frameBuffer = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	cv::rectangle(*frameBuffer, cv::Point(0, 0), cv::Point(WIDTH,HEIGHT), {0,0,0}, CV_FILLED);
	RecorderCallback rc = [&](AudioWindow& buffer) {
		double samplingRate = AS.sampleRate;

		SignalSource in(buffer, samplingRate);
		FramesCollection frames(in, FFT_SIZE);
			for(size_t i = 0; i < frames.count(); ++i) {
				SpectrumType spectrum = signalFFT->fft(frames.frame(i).toArray());
				std::size_t halfLength = spectrum.size() / 2;
				std::vector<double> absSpectrum(halfLength);
				for (std::size_t k = 0; k < halfLength; ++k) {
					absSpectrum[k] = std::abs(spectrum[k]);
				}

				fbmtx.lock();
				render(absSpectrum, *frameBuffer);
				fbmtx.unlock();
			}
			std::this_thread::yield();
		};
  Recorder recorder(rc, AS.sampleRate, AS.samples);
  recorder.capture();

	SDL_Event event;
	cv::Mat* last = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	cv::Mat* result = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	bool first = true;
	while (!DONE) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				exit(0);
			}
		}

		if(!first) {
			blend(*frameBuffer, *last, 0.3,*result);
		} else {
			frameBuffer->copyTo(*result);
			first = false;
		}
		canvas->draw(*result, MAGNIFICATION);
		result->copyTo(*last);

		std::this_thread::yield();
		usleep(16667);
	}

	return 0;
}
