#include <bogron/bitmapfont.hpp>
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



void intro(Canvas* canvas, Game& game) {
	BitmapFont f;
	cv::Mat* introFrame = new cv::Mat(HEIGHT, WIDTH, CV_8UC4, 0.0);
	HSLColor hsl(0,0,0);
	game.playFx(2);
	for(size_t i = 0; i < 40; ++i) {
			hsl.l_ = i * 2 + 20;
			f.drawtext(*introFrame,0, (WIDTH / 2) - (f.calcWidth("LET") / 2), "LET", RGBColor(hsl));
			canvas->draw(*introFrame);
			usleep(16667);
		}
		(*introFrame) *= 0;
		for(size_t i = 0; i < 50; ++i) {
			hsl.l_ = i * 2;
			f.drawtext(*introFrame,0,(WIDTH / 2) - (f.calcWidth("THE BATTLE") / 2),"THE BATTLE", RGBColor(hsl));
			canvas->draw(*introFrame);
			usleep(16667);
		}
		(*introFrame) *= 0;

		for(size_t i = 0; i < 50; ++i) {
			hsl.l_ = i * 2;
			f.drawtext(*introFrame,0,(WIDTH / 2) - (f.calcWidth("BEGIN") / 2),"BEGIN", RGBColor(hsl));
			canvas->draw(*introFrame);
			usleep(16667);
		}
}

int main(int argc, char** argv) {
	if (argc != 1) {
		std::cerr << "Usage: bogron" << std::endl;
		exit(1);
	}

	DONE = false;
	signal(SIGINT, FINISH);

	Canvas* canvas = new Canvas(WIDTH, HEIGHT, MAGNIFICATION, false);
	Renderer::init(WIDTH, HEIGHT);
	Renderer& r = Renderer::getInstance();
	Game game(WIDTH / 1.3, HEIGHT);

	MidiWiimote midi(0);

	std::thread canvasThread([&]() {
		cv::Mat* last = new cv::Mat(HEIGHT, WIDTH, CV_8UC4, 0.0);
		cv::Mat* result = new cv::Mat(HEIGHT, WIDTH, CV_8UC4, 0.0);
		bool first = true;

		while (!DONE) {
			game.lock();
			if(!first && !game.isOver()) {
				blend(*r.getFrameBuffer(), *last, 0.3, *result);
			} else {
				(*last) *= 0;
				r.getFrameBuffer()->copyTo(*result);
				first = false;
			}
			canvas->draw(*result);
			result->copyTo(*last);
			game.unlock();
			std::this_thread::yield();
			usleep(16667);
		}
	});


	while (!DONE) {
		game.lock();
		game.reset();
		r.clear();
		intro(canvas, game);
		game.startMusic();
		game.unlock();
		std::thread gameThread([&]() {
			while(!DONE) {
				game.lock();
				if(!game.isOver())
					game.step();
				else {
					game.unlock();
					break;
				}
				game.unlock();
				std::this_thread::yield();
				usleep(1000000);
			}
		});

		std::thread rendererThread([&]() {
			while(!DONE) {
				game.lock();
				if(!game.isOver()) {
					r.renderGame(game.grid(), game.player1(), game.player2());
				}
				else {
					game.unlock();
					break;
				}

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

				if(game.isOver()) {
					r.renderGameOver(game);
					if(ev.btn_home_.press_) {
						game.unlock();
						break;
					} else {
						game.unlock();
						continue;
					}
				}

				if(ev.btn_minus_.press_) {
					game.pauseMusic();
				}

				if(ev.btn_plus_.press_) {
					game.startMusic();
				}

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
				usleep(40000);
			}
		});

		SDL_Event event;

		while (!DONE) {
			if(game.isOver())
				break;

			if (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					exit(0);
				}
			}

			std::this_thread::yield();
			usleep(16667);
		}

		gameThread.join();
		rendererThread.join();
		midiThread.join();
	}
	return 0;
}
