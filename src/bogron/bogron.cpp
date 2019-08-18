#include <unistd.h>
#include <vector>
#include <iostream>
#include <cstdint>
#include <mutex>
#include <limits>
#include <thread>
#include <csignal>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "../lib/canvas.hpp"
#include "../lib/color.hpp"
#include "../lib/midiwiimote.hpp"

#include "game.hpp"
#include "palette.hpp"
#include "renderer.hpp"
#include "font.hpp"


constexpr off_t WIDTH = 100;
constexpr off_t HEIGHT = 10;
constexpr size_t MAGNIFICATION = 18;

using namespace std::chrono;

bool DONE = false;
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



int main(int argc, char** argv) {
	if (argc != 1) {
		std::cerr << "Usage: bogron" << std::endl;
		exit(1);
	}

	DONE = false;

	signal(SIGINT, FINISH);

	MidiWiimote midi(0);
	Canvas* canvas = new Canvas(WIDTH, HEIGHT, MAGNIFICATION, false);
	Game game(WIDTH / 1.3, HEIGHT);
	game.setSpeed(1000);

	std::thread gameThread([&]() {
		while(!DONE) {
			game.lock();
			game.step();
			game.unlock();
			std::this_thread::yield();
			usleep(1000000);
		}
	});

	Renderer r(WIDTH / 1.3, HEIGHT);
	std::thread rendererThread([&]() {
		while(!DONE) {
			game.lock();
			r.renderMap(game.grid(), game.player1(), game.player2());
			game.unlock();
			std::this_thread::yield();
			usleep(16667);
		}
	});

	std::thread midiThread([&]() {
		WMEvent ev;
		milliseconds last1 = game.epoch();
		milliseconds last2 = game.epoch();
		milliseconds now;
		while(!DONE) {
			ev = midi.receive();
			now = game.epoch();
			game.lock();
			if(ev.ctrlNr_ == 0) {
				if(last1.count() + 100 > now.count()) {
					game.unlock();
					continue;
				}

				if(ev.btn_2_.press_) {
					game.explode1();
				}
				if(ev.btn_left_.press_) {
					game.move1(Game::Direction::DOWN);
				}
				if(ev.btn_left_.press_) {
					game.move1(Game::Direction::DOWN);
				}
				if(ev.btn_right_.press_) {
					game.move1(Game::Direction::UP);
				}
				if(ev.btn_up_.press_) {
					game.move1(Game::Direction::LEFT);
				}
				if(ev.btn_down_.press_) {
					game.move1(Game::Direction::RIGHT);
				}
				last1 = game.epoch();
			} else if(ev.ctrlNr_ == 1) {
				if(last2.count() + 100 > now.count()) {
					game.unlock();
					continue;
				}
				if(ev.btn_2_.press_) {
					game.explode2();
				}
				if(ev.btn_left_.press_) {
					game.move2(Game::Direction::DOWN);
				}
				if(ev.btn_left_.press_) {
					game.move2(Game::Direction::DOWN);
				}
				if(ev.btn_right_.press_) {
					game.move2(Game::Direction::UP);
				}
				if(ev.btn_up_.press_) {
					game.move2(Game::Direction::LEFT);
				}
				if(ev.btn_down_.press_) {
					game.move2(Game::Direction::RIGHT);
				}
				last2 = game.epoch();
			}
			game.unlock();
			std::this_thread::yield();
		}
	});

	SDL_Event event;
	cv::Mat* last = new cv::Mat(HEIGHT, WIDTH, CV_8UC4, 0.0);
	cv::Mat* result = new cv::Mat(HEIGHT, WIDTH, CV_8UC4, 0.0);
	bool first = true;
	while (!DONE) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				exit(0);
			}
		}

		if (!first) {
			blend(*r.getFrameBuffer(), *last, 0.3, *result);
		} else {
			r.getFrameBuffer()->copyTo(*result);
			first = false;
		}
		canvas->draw(*result);
		result->copyTo(*last);

		std::this_thread::yield();
		usleep(16667);
	}

	return 0;
}
