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
#include "../lib/midiwiimote.hpp"
#include "../lib/sound.hpp"

using namespace std::chrono;

constexpr off_t WIDTH = 100;
constexpr off_t HEIGHT = 10;
constexpr size_t MAGNIFICATION = 18;

bool DONE;
static void FINISH(int ignore) {
	DONE = true;
}

struct ViewPort {
	off64_t x_ = 0;
	off64_t y_ = 0;
	double speedX_ = 0;
	double speedY_ = 0;
	double xa_ = 0;
	double ya_ = 0;
	double za_ = 0;
	double hue_ = 0;
	double saturation_ = 0;
	double lightness_ = 0;
};


std::mutex VP_MTX;
ViewPort VIEWPORT;
std::mutex EVENT_MUTEX;
WMEvent EVENT;
std::mutex TEXTURE_MTX;
std::vector<cv::Mat> TEXTURES;
off_t TEXTURE_IDX = 0;

void blend(const cv::Mat& src1, const cv::Mat& src2, const double alpha,
		const cv::Mat& dst) {
	using namespace cv;
	double beta;

	beta = (1.0 - alpha);
	addWeighted(src1, alpha, src2, beta, 0.0, dst);
}

void extractROI(const cv::Mat& texture, cv::Mat& view) {
	VP_MTX.lock();

	if (VIEWPORT.x_ >= texture.cols)
		VIEWPORT.x_ %= texture.cols;
	else if (VIEWPORT.x_ < 0)
		VIEWPORT.x_ = texture.cols + VIEWPORT.x_;

	if (VIEWPORT.y_ >= texture.rows)
		VIEWPORT.y_ %= texture.rows;
	else if (VIEWPORT.y_ < 0)
		VIEWPORT.y_ = texture.rows + VIEWPORT.y_;

	off64_t vx = VIEWPORT.x_;
	off64_t vy = VIEWPORT.y_;
	double h = VIEWPORT.hue_;
	double s = VIEWPORT.lightness_;
	double l = VIEWPORT.saturation_;

	VP_MTX.unlock();

	for (off64_t c = 0; c < WIDTH; ++c) {
		for (off64_t r = 0; r < HEIGHT; ++r) {
			off64_t x = vx + c;
			off64_t y = vy + r;
			if (x >= texture.cols) {
				x %= texture.cols;
			} else if (x < 0) {
				x = texture.cols + x;
			}
			if (y >= texture.rows) {
				y %= texture.rows;
			} else if (y < 0) {
				y = texture.rows + y;
			}

			for (off64_t vc = 0; vc < WIDTH; ++vc) {
				for (off64_t vr = 0; vr < HEIGHT; ++vr) {
					off64_t px = x + vc;
					off64_t py = y + vr;

					if (px >= texture.cols) {
						px %= texture.cols;
					} else if (px < 0) {
						px = texture.cols + px;
					}
					if (py >= texture.rows) {
						py %= texture.rows;
					} else if (py < 0) {
						py = texture.rows + py;
					}
					view.at<int32_t>(vr, vc) = texture.at<int32_t>(py, px);
				}
			}
		}
	}
	cv::Mat viewRGB(view.cols, view.rows, CV_8UC4);

	for (size_t c = 0; c < view.cols; ++c) {
		for (size_t r = 0; r < view.rows; ++r) {
			uint32_t& p = view.at<uint32_t>(r, c);

			RGBColor before(p);
			before.r_ += h * 2;
			before.g_ += s * 2;
			before.b_ += l * 2;

			if (before.r_ > 255)
				before.r_ = 255;
			if (before.g_ > 255)
				before.g_ = 255;
			if (before.b_ > 255)
				before.b_ = 255;

			p = before.val();
		}
	}
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
					canvas->putpixel(j * MAGNIFICATION + l, i * MAGNIFICATION + k, m.at<int32_t>(i, j));
				}
			}
		}
	}

	canvas->update();
}



int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: slide <png-file-1> <png-file-2> ..." << std::endl;
		exit(1);
	}
	DONE = false;
	signal(SIGINT, FINISH);

	for (off_t i = 1; i < argc; ++i) {
		cv::Mat textureRGB = cv::imread(argv[i], CV_LOAD_IMAGE_COLOR);
		cv::Mat texture;
		cv::cvtColor(textureRGB, texture, CV_RGB2RGBA);
		TEXTURES.push_back(texture);
	}

	Canvas* canvas = new Canvas(WIDTH * MAGNIFICATION, HEIGHT * MAGNIFICATION * 2, false);
	MidiWiimote midi(0);
	Sound snd;
	snd.load("swing.wav");
	std::thread midiThread([&]() {
		while ( !DONE ) {
			EVENT_MUTEX.lock();
			EVENT = midi.receive();
			std::cerr << EVENT << std::endl;
			EVENT_MUTEX.unlock();

			std::this_thread::yield();
			usleep( 10000 );
		}
	});
	double lastTotal = -1;
	milliseconds lastMillis = duration_cast<milliseconds>(
			system_clock::now().time_since_epoch());
	std::thread slideThread(
			[&]() {
				while(!DONE) {
					EVENT_MUTEX.lock();
					if(EVENT.btn_right_.release_)
					++TEXTURE_IDX;
					else if(EVENT.btn_left_.release_)
					--TEXTURE_IDX;

					if(TEXTURE_IDX >= TEXTURES.size())
					TEXTURE_IDX = 0;
					else if(TEXTURE_IDX < 0)
					TEXTURE_IDX = TEXTURES.size() - 1;

					off_t roll = (EVENT.roll_ - 64);
					off_t pitch = (EVENT.pitch_ - 64) * -1;
					double xa = EVENT.xa_ - 50;
					double ya = EVENT.ya_ - 50;
					double za = EVENT.za_ - 50;
					if(xa < 20)
					xa = 0;
					if(ya < 20)
					ya = 0;
					if(za < 20)
					za = 0;
					EVENT_MUTEX.unlock();
					VP_MTX.lock();

					VIEWPORT.speedX_ += roll;

					VIEWPORT.speedY_ += pitch;

					if(std::abs(VIEWPORT.speedX_) / 20 < 0.01)
					VIEWPORT.speedX_ = 0;
					if(std::abs(VIEWPORT.speedY_) / 50 < 0.01)
					VIEWPORT.speedY_ = 0;

					VIEWPORT.x_ += ceil(VIEWPORT.speedX_ / 20);
					VIEWPORT.y_ += ceil(VIEWPORT.speedY_ / 50);

					VIEWPORT.hue_ += xa;
					VIEWPORT.lightness_ += ya;
					VIEWPORT.saturation_ += za;

					double total = xa + ya + za;
					milliseconds millis = duration_cast< milliseconds >(
							system_clock::now().time_since_epoch()
					);
					if(lastTotal == -1 || (total - lastTotal > 25 && (millis - lastMillis).count() > 250)) {
						snd.play(0);
						lastMillis = millis;
					}
					lastTotal = total;
					TEXTURE_MTX.lock();
					if(VIEWPORT.x_ >= TEXTURES[TEXTURE_IDX].cols) {
						VIEWPORT.x_ = VIEWPORT.x_ % TEXTURES[TEXTURE_IDX].cols;
					} else if(VIEWPORT.x_ < 0) {
						VIEWPORT.x_ = TEXTURES[TEXTURE_IDX].cols + VIEWPORT.x_;
					}
					if(VIEWPORT.y_ >= TEXTURES[TEXTURE_IDX].rows) {
						VIEWPORT.y_ = VIEWPORT.y_ % TEXTURES[TEXTURE_IDX].rows;
					} else if(VIEWPORT.y_ < 0) {
						VIEWPORT.y_ = TEXTURES[TEXTURE_IDX].rows + VIEWPORT.y_;
					}
					TEXTURE_MTX.unlock();

					VIEWPORT.speedX_ /= 2;
					VIEWPORT.speedY_ /= 2;
					VIEWPORT.hue_ /= 2;
					VIEWPORT.lightness_ /= 2;
					VIEWPORT.saturation_ /= 2;

					VP_MTX.unlock();
					std::this_thread::yield();
					usleep(16667);
				}
			});

	SDL_Event event;
	cv::Mat* view = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);

	while (!DONE) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				exit(0);
			}
		}

		TEXTURE_MTX.lock();
		extractROI(TEXTURES[TEXTURE_IDX], *view);
		TEXTURE_MTX.unlock();

		draw(canvas, *view);
		std::this_thread::yield();
		usleep(16667);
	}

	return 0;
}
