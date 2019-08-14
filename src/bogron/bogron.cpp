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

constexpr off_t WIDTH = 100;
constexpr off_t HEIGHT = 10;
constexpr size_t MAGNIFICATION = 18;

using namespace std::chrono;

bool DONE = false;
static void FINISH(int ignore) {
	DONE = true;
}

void overlayImage(cv::Mat* src, cv::Mat* overlay, const cv::Point& location)
{
	using namespace cv;
	using namespace std;
    for (int y = max(location.y, 0); y < src->rows; ++y)
    {
        int fY = y - location.y;

        if (fY >= overlay->rows)
            break;

        for (int x = max(location.x, 0); x < src->cols; ++x)
        {
            int fX = x - location.x;

            if (fX >= overlay->cols)
                break;

            double opacity = ((double)overlay->data[fY * overlay->step + fX * overlay->channels() + 3]) / 255;

            for (int c = 0; opacity > 0 && c < src->channels(); ++c)
            {
                unsigned char overlayPx = overlay->data[fY * overlay->step + fX * overlay->channels() + c];
                unsigned char srcPx = src->data[y * src->step + x * src->channels() + c];
                src->data[y * src->step + src->channels() * x + c] = srcPx * (1. - opacity) + overlayPx * opacity;
            }
        }
    }
}

void blend(const cv::Mat& src1, const cv::Mat& src2, const double alpha,
		const cv::Mat& dst) {
	using namespace cv;
	double beta;

	beta = (1.0 - alpha);
	addWeighted(src1, alpha, src2, beta, 0.0, dst);
}

void renderPlayer(cv::Mat* frameBuffer, const Player& p, const RGBColor& color) {
	cv::Mat layer(HEIGHT, WIDTH, CV_8UC4, cv::Scalar(0,0,0,0));
	cv::Mat copy(HEIGHT, WIDTH, CV_8UC4);

	frameBuffer->copyTo(copy);
	HSLColor hsl2(color);
	hsl2.l_ = 70 / (6 - p.lifes_);
	hsl2.s_ = 99;
	RGBColor rgb2(hsl2);
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4) = rgb2.b_;
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4 + 1) = rgb2.g_;
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4 + 2) = rgb2.r_;
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4 + 3) = 0xFF;

	if(p.hasBomb_ || p.hasNuke_) {
		RGBColor haloColor(0,0,0);
		if(p.hasBomb_)
			haloColor = Palette::BOMB_;
		else
			haloColor = Palette::NUKE_;

		off_t y = p.pos_.second;
		off_t x = p.pos_.first * 4 + 4;
		if(x < WIDTH * 4) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second;
		x = p.pos_.first * 4 - 4;
		if(x > 0) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second + 1;
		x = p.pos_.first * 4;
		if(y < HEIGHT) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second - 1;
		x = p.pos_.first * 4;
		if(y > 0) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}
	}
	overlayImage(frameBuffer, &layer, cv::Point{0,0});
//	blend(layer, copy, 0.5, *frameBuffer);
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
				if(color.r_ == Palette::BLANK_.r_ && color.g_ == Palette::BLANK_.g_ && color.b_ == Palette::BLANK_.b_)
					frameBuffer->at<uint8_t>(r, c * 4 + 3) = 0x00;
				else
					frameBuffer->at<uint8_t>(r, c * 4 + 3) = 0xff;
			}
		}

		renderPlayer(frameBuffer, p1, Palette::PLAYER_1);
		renderPlayer(frameBuffer, p2, Palette::PLAYER_2);

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
