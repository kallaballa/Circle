#include <unistd.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <limits>
#include <thread>
#include <csignal>
#include <chrono>
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
constexpr size_t FFT_SIZE = 32;

using namespace std::chrono;

struct AudioSettings {
  bool normalize;
  double sampleRate;
  size_t samples;
};

AudioSettings AS = {true, 44100, 1024};


typedef std::vector<double> AudioWindow;
std::mutex abmtx;
AudioWindow audio_buffer;

bool DONE;
static void FINISH(int ignore) {
	DONE = true;
}

double mix_channels(AudioWindow& buffer) {
	double avg = 0;
	for (double& d : buffer) {
		avg += d;
	}
	avg /= buffer.size();

	return avg;
}

void blend(const cv::Mat& src1, const cv::Mat& src2, const double alpha,
		const cv::Mat& dst) {
	using namespace cv;
	double beta;

	beta = (1.0 - alpha);
	addWeighted(src1, alpha, src2, beta, 0.0, dst);
}

void draw(Canvas* canvas, cv::Mat& m) {
	canvas->fillRectangle(0, 0, canvas->screenWidth, canvas->screenHeight, 0, 0,
			0, 255);

	SDL_Surface* surface = canvas->getSurface();
	Uint32 *p = NULL;

	for (off_t i = 0; i < m.rows; i++) {
		if (i >= surface->w)
			continue;
		for (off_t j = 0; j < m.cols; j++) {
			if (j >= surface->h)
				continue;
			for (off_t k = 0; k < MAGNIFICATION; k++) {
				for (off_t l = 0; l < MAGNIFICATION; l++) {
					canvas->putpixel(j * MAGNIFICATION + l, i * MAGNIFICATION + k + (HEIGHT * MAGNIFICATION / 2), m.at<int32_t>(i, j));
				}
			}
		}
	}

	canvas->update();
}

void render(std::vector<double>& absSpectrum,
		cv::Mat& target) {
	cv::rectangle(target, cv::Point(0, 0), cv::Point(WIDTH,HEIGHT), {0,0,0}, CV_FILLED);
	double max = 0;
	for(size_t i = 0; i < absSpectrum.size(); ++i) {
		max = std::max(absSpectrum[i], max);
	}
	for(size_t i = 0; i < absSpectrum.size(); ++i) {
		absSpectrum[i] = absSpectrum[i] / max * HEIGHT;
	}
	for(size_t i = 0; i < absSpectrum.size(); ++i) {
		HSLColor hsl;
		hsl.h_ = i / (double)absSpectrum.size() * 360.0;
		hsl.s_ = 99;
		hsl.l_ = 50;
		RGBColor rgb(hsl);
		cv::rectangle(target, cv::Point(i * (WIDTH/(double)FFT_SIZE),0), cv::Point(i * (WIDTH/(double)FFT_SIZE) + (WIDTH/(double)FFT_SIZE) - 1,absSpectrum[i]), cv::Scalar(rgb.b_,rgb.g_,rgb.r_), CV_FILLED);
	}
}
int main(int argc, char** argv) {
	if (argc != 1) {
		std::cerr << "Usage: spectro" << std::endl;
		exit(1);
	}
	DONE = false;
	signal(SIGINT, FINISH);

	Canvas* canvas = new Canvas(WIDTH * MAGNIFICATION, HEIGHT * MAGNIFICATION * 2, false);

  RecorderCallback rc = [&](AudioWindow& buffer) {
    abmtx.lock();
    audio_buffer = buffer;
    abmtx.unlock();
  };
  Recorder recorder(rc, AS.sampleRate, AS.sampleRate);
  recorder.capture();
	using namespace Aquila;
  auto signalFFT = FftFactory::getFft(16);
  std::mutex fbmtx;
	cv::Mat* frameBuffer = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	cv::rectangle(*frameBuffer, cv::Point(0, 0), cv::Point(WIDTH,HEIGHT), {0,0,0}, CV_FILLED);

  std::thread t([&]() {
  try {
		auto start = std::chrono::system_clock::now();
		size_t f = 0;
		while(true) {
			abmtx.lock();
			AudioWindow copy = audio_buffer;
			abmtx.unlock();

			double samplingRate = AS.sampleRate;

			SignalSource in(copy, samplingRate);
			FramesCollection frames(in, FFT_SIZE);
//pragma omp parallel for ordered schedule(dynamic)
			for(size_t i = 0; i < frames.count(); ++i) {
				SpectrumType spectrum = signalFFT->fft(frames.frame(i).toArray());
				std::size_t halfLength = spectrum.size() / 2;
				std::vector<double> absSpectrum(halfLength);
				for (std::size_t k = 0; k < halfLength; ++k) {
					absSpectrum[k] = std::abs(spectrum[k]);
//					std::cerr << absSpectrum[k] << '\t';
				}
//				std::cerr << std::endl;
				fbmtx.lock();
				render(absSpectrum, *frameBuffer);
				fbmtx.unlock();
//				if(f % 100 == 0) {
//					auto dur = std::chrono::duration_cast<microseconds>(
//													std::chrono::system_clock::now() - start);
//					std::cerr << ((double)f / ((double)dur.count() / 1000000)) << std::endl;
//					f = 0;
//					start = std::chrono::system_clock::now();
//				}
//				++f;
			}
		}
		} catch (std::exception& ex) {
			std::cerr << ex.what() << std::endl;
		}
  });

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

//		if(!first) {
//			blend(*frameBuffer, *last, 0.05,*result);
//		} else {
			frameBuffer->copyTo(*result);
			first = false;
//		}
		draw(canvas, *result);
		result->copyTo(*last);

		std::this_thread::yield();
		usleep(16667);
	}

	return 0;
}
