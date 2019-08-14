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
#include "../lib/midiwiimote.hpp"

#include "game.hpp"
#include "palette.hpp"

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

void render() {
}

int main(int argc, char** argv) {
	if (argc != 1) {
		std::cerr << "Usage: bogron" << std::endl;
		exit(1);
	}
	DONE = false;
	signal(SIGINT, FINISH);

	MidiWiimote midi(0);
	Canvas* canvas = new Canvas(WIDTH * MAGNIFICATION, HEIGHT * MAGNIFICATION,
			false);
	cv::Mat* frameBuffer = new cv::Mat(HEIGHT, WIDTH, CV_8UC4, 0.0);
	Game game(WIDTH, HEIGHT);
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
				if(ev.btn_left_.press_) {
					std::cerr << "LEFT" << std::endl;
					game.move1(Game::Direction::DOWN);
				} else if(ev.btn_right_.press_) {
					game.move1(Game::Direction::UP);
				} else if(ev.btn_up_.press_) {
					game.move1(Game::Direction::LEFT);
				} else if(ev.btn_down_.press_) {
					game.move1(Game::Direction::RIGHT);
				}
				last1 = game.epoch();
			} else if(ev.ctrlNr_ == 1) {
				if(last2.count() + 100 > now.count()) {
					game.unlock();
					continue;
				}
				if(ev.btn_left_.press_) {
					game.move2(Game::Direction::DOWN);
				} else if(ev.btn_right_.press_) {
					game.move2(Game::Direction::UP);
				} else if(ev.btn_up_.press_) {
					game.move2(Game::Direction::LEFT);
				} else if(ev.btn_down_.press_) {
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

		game.lock();
		Game::grid_t grid = game.grid();
		Player p1 = game.player1();
		Player p2 = game.player2();
		game.unlock();

		for (size_t r = 0; r < HEIGHT; ++r) {
			for (size_t c = 0; c < WIDTH; ++c) {
				uint8_t cell = grid[r][c];
				RGBColor color(0);
				switch (cell) {
				case Object::PLAYER_1:
					color = Palette::PLAYER_1;
					break;
				case Object::PLAYER_2:
					color = Palette::PLAYER_2;
					break;
				case Object::MINE_:
					color = Palette::MINE_;
					break;
				case Object::BOMB_:
					color = Palette::BOMB_;
					break;
				case Object::NUKE_:
					color = Palette::NUKE_;
					break;
				case Object::BLANK_:
					color = Palette::BLANK_;
					break;
				default:
					assert(false);
					break;
				}
				frameBuffer->at<uint8_t>(r, c * 4) = color.b_;
				frameBuffer->at<uint8_t>(r, c * 4 + 1) = color.g_;
				frameBuffer->at<uint8_t>(r, c * 4 + 2) = color.r_;
				frameBuffer->at<uint8_t>(r, c * 4 + 3) = 0xff;
			}
		}

		frameBuffer->at<uint8_t>(p1.pos_.second, p1.pos_.first * 4) = Palette::PLAYER_1.b_;
		frameBuffer->at<uint8_t>(p1.pos_.second, p1.pos_.first * 4 + 1) = Palette::PLAYER_1.g_;
		frameBuffer->at<uint8_t>(p1.pos_.second, p1.pos_.first * 4 + 2) = Palette::PLAYER_1.r_;
		frameBuffer->at<uint8_t>(p1.pos_.second, p1.pos_.first * 4 + 3) = 0xff;

		frameBuffer->at<uint8_t>(p2.pos_.second, p2.pos_.first * 4) = Palette::PLAYER_2.b_;
		frameBuffer->at<uint8_t>(p2.pos_.second, p2.pos_.first * 4 + 1) = Palette::PLAYER_2.g_;
		frameBuffer->at<uint8_t>(p2.pos_.second, p2.pos_.first * 4 + 2) = Palette::PLAYER_2.r_;
		frameBuffer->at<uint8_t>(p2.pos_.second, p2.pos_.first * 4 + 3) = 0xff;

		if (!first) {
			blend(*frameBuffer, *last, 0.3, *result);
		} else {
			frameBuffer->copyTo(*result);
			first = false;
		}
		canvas->draw(*frameBuffer, MAGNIFICATION);
		result->copyTo(*last);

		std::this_thread::yield();
		usleep(16667);
	}

	return 0;
}
