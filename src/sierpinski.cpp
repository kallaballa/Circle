#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <unistd.h>
#include <vector>
#include <deque>
#include <iostream>
#include <mutex>
#include <limits>
#include <thread>
#include <pthread.h>
#include <bitset>
#include <csignal>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <rtmidi/RtMidi.h>
#include <SDL/SDL_mixer.h>
#include <cstdlib>
#include <cstdio>
#include <png.h>
#include "third/history/NLCommand.h"
#include "third/history/NLCommandGroup.h"
#include "third/history/NLHistory.h"
#include "canvas.hpp"
#include "button.hpp"
#include "average.hpp"
#include "color.hpp"

using namespace std::chrono;

constexpr off_t WIDTH = 100;
constexpr off_t HEIGHT = 10;

std::mutex VP_MTX;
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

ViewPort VIEWPORT;

RtMidiIn *MIDI_IN = new RtMidiIn();
std::vector<uint8_t> MESSAGE;
bool DONE;
static void FINISH(int ignore) {
	DONE = true;
}

std::mutex EVENT_MUTEX;

struct WMEvent {
	int ctrlNr_ = 0;
	double roll_ = 0;
	double pitch_ = 0;
	double xa_ = 0;
	double ya_ = 0;
	double za_ = 0;
	Button btn_up_;
	Button btn_down_;
	Button btn_left_;
	Button btn_right_;
	Button btn_a_;
	Button btn_b_;
	Button btn_minus_;
	Button btn_plus_;
	Button btn_home_;
	Button btn_1_;
	Button btn_2_;
};

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
	//FIXME ############### off_t instead of size_t for coordinates; ################

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
//					std::cerr << vr << '/' << vc << '\t' << px << '/' << py << std::endl;
//					std::cerr << view.cols << '/' << view.rows << '\t' << texture.cols << '/' << texture.rows << std::endl;

//					std::cerr << view.type() << "==" << texture.type() << std::endl;
//					std::cerr << view.depth() << "==" << texture.depth() << std::endl;

					view.at<int32_t>(vr, vc) = texture.at<int32_t>(py, px);
				}
			}
		}
	}
	cv::Mat viewRGB(view.cols, view.rows, CV_8UC4);

	for (size_t c = 0; c < view.cols; ++c) {
		for (size_t r = 0; r < view.rows; ++r) {
			uint32_t& p = view.at<uint32_t>(r, c);

//			std::cerr << '1' << std::hex << p << std::dec << std::endl;
			RGBColor before(p);
//			std::cerr << "RGB:" << std::hex << before.r_ << before.g_ << before.b_ << std::dec << std::endl;
//			std::cerr << '2' << std::hex << before.val() << std::dec << std::endl;
			before.r_ += h * 2;
			before.g_ += s * 2;
			before.b_ += l * 2;
			if(before.r_ > 255)
				before.r_ = 255;
			if(before.g_ > 255)
				before.g_ = 255;
			if(before.b_ > 255)
				before.b_ = 255;


			//			HSLColor hsl(before);
////			hsl.adjustHue(h);
//			//hsl.adjustSaturation(s);
//			//hsl.adjustLightness(l);
//			RGBColor after(hsl);
//			std::cerr << '3' << std::hex << before.val() << std::dec << std::endl;
//
			p = before.val();
		}
	}
}

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;
	switch (bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(Uint16 *) p = pixel;
		break;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(Uint32 *) p = pixel;
		break;
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
//      std::cerr << std::hex << m.at<int32_t>(i,j) << std::dec << std::endl;
			for (off_t k = 0; k < 20; k++) {
				for (off_t l = 0; l < 20; l++) {
					putpixel(surface, j * 20 + l, i * 20 + k, m.at<int32_t>(i, j));
				}
			}
		}
	}

	canvas->update();
}

class Sound {
private:
	std::vector<Mix_Chunk*> chunks;
public:
	Sound() {
	  //Initialize SDL_mixer
	  if( Mix_OpenAudio( 44100, AUDIO_U8, 1, 512 ) == -1 )
	  {
	  	std::cerr << "Error opening audio device" << std::endl;
	      throw std::exception();
	  }
	}

	~Sound() {
		for (auto& c : chunks)
			Mix_FreeChunk(c);

		Mix_CloseAudio();

	}

	//Loads the wav file and returns it's index
	size_t load(const string& filename) {
	  Mix_Chunk* effect = Mix_LoadWAV(filename.c_str());
	  if(effect == NULL)
	  	throw std::exception();
	  chunks.push_back(effect);
	  return chunks.size() - 1;
	}

	void play(size_t idx) {
		if (Mix_Playing(1) != 0)
			return;
		if ( Mix_PlayChannel( -1, chunks[idx], 0 ) == -1) {
			throw std::exception();
		}
	}
};

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: slide <png-file-1> <png-file-2> ..." << std::endl;
		exit(1);
	}

	for (off_t i = 1; i < argc; ++i) {
		cv::Mat textureRGB = cv::imread(argv[i], CV_LOAD_IMAGE_COLOR);
		cv::Mat texture;
		cv::cvtColor(textureRGB, texture, CV_RGB2RGBA);
		TEXTURES.push_back(texture);
	}

	MIDI_IN->openPort(0);
	MIDI_IN->ignoreTypes(false, false, false);
	DONE = false;
	signal(SIGINT, FINISH);

	Canvas* canvas = new Canvas(WIDTH * 20, HEIGHT * 40, false);
	Sound snd;
	snd.load("swing.wav");
	std::thread midiThread([&]() {
		int nBytes;

		while ( !DONE ) {
			MIDI_IN->getMessage( &MESSAGE );

			nBytes = MESSAGE.size();

			if(nBytes == 10) {
				EVENT_MUTEX.lock();
				EVENT.ctrlNr_ = MESSAGE[1];
//      	event.roll_ = avgRoll.smooth(message[2]);
//      	event.pitch_ = avgPitch.smooth(message[3]);
//      	event.x_a_ = avgAccX.smooth(message[4]);
//      	event.y_a_ = avgAccY.smooth(message[5]);
//      	event.z_a_ = avgAccZ.smooth(message[6]);
			EVENT.roll_ = MESSAGE[2];
			EVENT.pitch_ = MESSAGE[3];
			EVENT.xa_ = MESSAGE[4];
			EVENT.ya_ = MESSAGE[5];
			EVENT.za_ = MESSAGE[6];
			uint16_t buttons = (((uint16_t) MESSAGE[7])) | ((uint16_t) MESSAGE[8] << 8);

			EVENT.btn_up_.update(buttons & MASK_BTN_UP);
			EVENT.btn_down_.update(buttons & MASK_BTN_DOWN);
			EVENT.btn_left_.update(buttons & MASK_BTN_LEFT);
			EVENT.btn_right_.update(buttons & MASK_BTN_RIGHT);
			EVENT.btn_a_.update(buttons & MASK_BTN_A);
			EVENT.btn_b_.update(buttons & MASK_BTN_B);
			EVENT.btn_minus_.update(buttons & MASK_BTN_MINUS);
			EVENT.btn_plus_.update(buttons & MASK_BTN_PLUS);
			EVENT.btn_home_.update(buttons & MASK_BTN_HOME);
			EVENT.btn_1_.update(buttons & MASK_BTN_1);
			EVENT.btn_2_.update(buttons & MASK_BTN_2);

			if(EVENT.btn_right_.release_)
			++TEXTURE_IDX;
			else if(EVENT.btn_left_.release_)
			--TEXTURE_IDX;

			if(TEXTURE_IDX >= TEXTURES.size())
			TEXTURE_IDX = 0;
			else if(TEXTURE_IDX < 0)
			TEXTURE_IDX = TEXTURES.size() - 1;

			EVENT_MUTEX.unlock();
		}

		std::this_thread::yield();
		usleep( 10000 );
	}
});
	double lastTotal = -1;
	milliseconds lastMillis = duration_cast< milliseconds >(
	    system_clock::now().time_since_epoch()
	);
	std::thread slideThread([&]() {
		while(!DONE) {
			EVENT_MUTEX.lock();
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
			usleep(40000);
		}
	});

	SDL_Event event;
	cv::Mat* view = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
//  cv::Mat* last = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
//  cv::Mat *dst = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
	while (!DONE) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				exit(0);
			}
		}

		TEXTURE_MTX.lock();
		extractROI(TEXTURES[TEXTURE_IDX], *view);
		TEXTURE_MTX.unlock();
//    if(!last->empty()) {
//    	for(size_t i = 1; i < 4; ++i) {
//    		blend(*last, *view, 1.0/4 - i, *dst);
//        draw(canvas, *dst);
//        usleep(10000);
//    	}
//    }
		draw(canvas, *view);
//    view->copyTo(*last);
		std::this_thread::yield();
		usleep(40000);
	}

	return 0;
}
