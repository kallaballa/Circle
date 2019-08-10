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
#include "../lib/button.hpp"
#include "../lib/color.hpp"
#include "../lib/midikeyboard.hpp"

using namespace std::chrono;

constexpr off_t WIDTH = 100;
constexpr off_t HEIGHT = 10;
constexpr size_t MAGNIFICATION = 18;

std::mutex EVMTX;
NoteEvent EVENT;

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

void drawNote(const NoteEvent& ev, cv::Mat& m) {
	HSLColor hsl;
	double note = ev.note_ - 48;
	hsl.h_ = note / 19.0 * 360.0;
	hsl.s_ = 99;
	hsl.l_ = 50;
	RGBColor rgb(hsl);
	std::cerr << ev.note_ << std::endl;
	cv::rectangle(m, cv::Point(0, 0), cv::Point(WIDTH,HEIGHT), {0,0,0}, CV_FILLED);
	cv::rectangle(m, cv::Point(note * 4, 0), cv::Point(note * 4 + 3,HEIGHT), cv::Scalar(rgb.b_,rgb.g_,rgb.r_), CV_FILLED);
}

int main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "Usage: piano <input-midi-port-nr> <output-midi-port-nr>..." << std::endl;
		exit(1);
	}
	DONE = false;
	signal(SIGINT, FINISH);

	Canvas* canvas = new Canvas(WIDTH * MAGNIFICATION, HEIGHT * MAGNIFICATION * 2, false);
	MidiKeyboard midi(atoi(argv[1]));

	std::thread midiThread([&]() {
		while(true) {
			EVMTX.lock();
			EVENT = midi.receive();
			EVMTX.unlock();

			std::this_thread::yield();
			usleep( 10000 );
		}
	});

	SDL_Event event;
	cv::Mat* last = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	cv::Mat* now = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	cv::Mat* result = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	bool first = true;
	while (!DONE) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				exit(0);
			}
		}
		drawNote(EVENT, *now);

		if(!first) {
			blend(*now, *last, 0.05,*result);
		} else {
			now->copyTo(*result);
			first = false;
		}
		canvas->draw(*result, MAGNIFICATION);
		result->copyTo(*last);

		std::this_thread::yield();
		usleep(16667);
	}

	return 0;
}
