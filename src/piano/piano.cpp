#include <unistd.h>
#include <vector>
#include <map>
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
constexpr size_t MIN_NOTE_VAL = 48;
constexpr size_t MAX_NOTE_VAL = 72;

struct NoteState {
	uint64_t timestamp_ = 0;
	bool on_ = false;
	uint8_t velocity_;
};

std::map<size_t, NoteState> NOTE_STATE_MAP;

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
	uint64_t epoch =
	    std::chrono::system_clock::now().time_since_epoch() /
	    std::chrono::milliseconds(1);

	HSLColor hsl;
//	std::cerr << ev.note_ << '\t' << ev.on_ <<  std::endl;

	if(ev.note_ < MIN_NOTE_VAL || ev.note_ > MAX_NOTE_VAL)
		return;

	NOTE_STATE_MAP[ev.note_ - MIN_NOTE_VAL] = {epoch, ev.on_, ev.velocity_};
	cv::rectangle(m, cv::Point(0, 0), cv::Point(WIDTH, HEIGHT), { 0, 0, 0 },
			CV_FILLED);
	for (auto& p : NOTE_STATE_MAP) {
//		std::cerr << p.first << '\t' << p.second << '\t';
		size_t note = p.first;

		if (p.second.on_ && p.second.timestamp_ > epoch - 100 ) {
			hsl.h_ = (double) note / (MAX_NOTE_VAL - MIN_NOTE_VAL) * 360.0;
			hsl.s_ = 99;
			hsl.l_ = 70;
			RGBColor rgb(hsl);
			cv::rectangle(m, cv::Point(note * 4, 0), cv::Point(note * 4 + 3, (double)HEIGHT * ((p.second.velocity_ + 1) / 127.0)),
					cv::Scalar(rgb.b_, rgb.g_, rgb.r_), CV_FILLED);
		} else {
			cv::rectangle(m, cv::Point(note * 4, 0), cv::Point(note * 4 + 3, HEIGHT),
					cv::Scalar(0,0,0), CV_FILLED);
		}
	}
//	std::cerr << std::endl;
}

int main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "Usage: piano <input-midi-port-nr> <output-midi-port-nr>..." << std::endl;
		exit(1);
	}
	for(size_t i = 0; i < MAX_NOTE_VAL - MIN_NOTE_VAL; ++i) {
		NOTE_STATE_MAP[i] = {0, false, 0};
	}
	DONE = false;
	signal(SIGINT, FINISH);

	Canvas* canvas = new Canvas(WIDTH * MAGNIFICATION, HEIGHT * MAGNIFICATION, false);
	MidiKeyboard midi(atoi(argv[1]));

	std::thread midiThread([&]() {
		while(true) {
			EVMTX.lock();
			EVENT = midi.receive();
			EVMTX.unlock();

			std::this_thread::yield();
			usleep( 1000 );
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
			blend(*now, *last, 0.1,*result);
		} else {
			now->copyTo(*result);
			first = false;
		}
		canvas->draw(*result);
		result->copyTo(*last);

		std::this_thread::yield();
		usleep(16667);
	}
	midiThread.join();
	return 0;
}
