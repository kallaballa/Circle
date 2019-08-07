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
#include <opencv2/opencv.hpp>
#include <rtmidi/RtMidi.h>
#include <cstdlib>
#include <cstdio>
#include <png.h>
#include "third/history/NLCommand.h"
#include "third/history/NLCommandGroup.h"
#include "third/history/NLHistory.h"
#include "canvas.hpp"
#include "button.hpp"
#include "average.hpp"
#include "trans.hpp"
#include "color.hpp"

constexpr size_t WIDTH = 100;
constexpr size_t HEIGHT = 10;

std::mutex vpMutex;
struct ViewPort {
	off64_t x_ = 0;
	off64_t y_ = 0;
	double speedX_ = 0;
	double speedY_ = 0;
};

ViewPort VIEWPORT;

AverageReadings avgRoll;
AverageReadings avgPitch;
AverageReadings avgAccX;
AverageReadings avgAccY;
AverageReadings avgAccZ;

RtMidiIn *midiin = new RtMidiIn();
std::vector<uint8_t> message;
bool done;
static void finish(int ignore){ done = true; }

std::mutex eventMutex;

struct WMEvent {
	int ctrlNr_ = 0;
  double roll_ = 0;
  double pitch_ = 0;
	double x_a_ = 0;
	double y_a_ = 0;
	double z_a_ = 0;
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

WMEvent event;
HSLColor hsl = { 0, 50, 50};

void blend(const cv::Mat& src1, const cv::Mat& src2, const double alpha, const cv::Mat& dst)
{
	using namespace cv;
   double beta; double input;

   beta = ( 1.0 - alpha );
   addWeighted( src1, alpha, src2, beta, 0.0, dst);
}

void extractROI(const cv::Mat& texture, cv::Mat& view) {
	//FIXME ############### off_t instead of size_t for coordinates; ################

	vpMutex.lock();

	if(VIEWPORT.x_ >= texture.cols)
		VIEWPORT.x_ %= texture.cols;
	else if(VIEWPORT.x_ < 0)
		VIEWPORT.x_ = texture.cols + VIEWPORT.x_;

	if(VIEWPORT.y_ >= texture.rows)
		VIEWPORT.y_ %= texture.rows;
	else if(VIEWPORT.y_ < 0)
		VIEWPORT.y_ = texture.rows + VIEWPORT.y_;

	off64_t vx = VIEWPORT.x_;
	off64_t vy = VIEWPORT.y_;

	vpMutex.unlock();

	for(off64_t c = 0; c < WIDTH; ++c) {
		for(off64_t r = 0; r < HEIGHT; ++r) {
			off64_t x = vx + c;
			off64_t y = vy + r;
			if(x >= texture.cols) {
				x %= texture.cols;
			} else if(x < 0) {
				x = texture.cols + x;
			}
			if(y >= texture.rows){
				y %= texture.rows;
			} else if(y < 0) {
				y = texture.rows + y;
			}

			for(off64_t vc = 0; vc < WIDTH; ++vc) {
				for(off64_t vr = 0; vr < HEIGHT; ++vr) {
					off64_t px = x + vc;
					off64_t py = y + vr;

					if (px >= texture.cols) {
						px %= texture.cols;
					} else if (px < 0) {
						px = texture.cols + px;
					}
					if(py >= texture.rows) {
						py %= texture.rows;
					}	else if(py < 0) {
						py = texture.rows + py;
					}
//					std::cerr << vr << '/' << vc << '\t' << px << '/' << py << std::endl;
//					std::cerr << view.cols << '/' << view.rows << '\t' << texture.cols << '/' << texture.rows << std::endl;

//					std::cerr << view.type() << "==" << texture.type() << std::endl;
//					std::cerr << view.depth() << "==" << texture.depth() << std::endl;

					view.at<int32_t>(vr,vc) = texture.at<int32_t>(py, px);
				}
			}
		}
	}
}

void colorAt(cv::Mat& m, int x, int y )
{
	eventMutex.lock();
	hsl.h_ = x * 2 + event.x_a_;
	hsl.s_ = (150 - event.y_a_);
//	hsl.adjustLightness((50 - z_a) / 10);
	eventMutex.unlock();

	RGBColor rgb(hsl);
  m.at<int32_t>(y,x) = rgb.val();
}

void make_rainbow(cv::Mat& m) {
  for (off_t i = 0; i < m.rows; i++) {
    for (off_t j = 0; j < m.cols; j++) {
    	colorAt(m, j, i);
    }
  }
}

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
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
        *(Uint32 *)p = pixel;
        break;
    }
}

void draw(Canvas* canvas, cv::Mat& m) {
  canvas->fillRectangle(0,0, canvas->screenWidth, canvas->screenHeight, 0,0,0,255);

  SDL_Surface* surface = canvas->getSurface();
  Uint32 *p = NULL;

  for (off_t i = 0; i < m.rows; i++) {
    if(i >= surface->w)
      continue;
    for (off_t j = 0; j < m.cols; j++) {
      if(j >= surface->h)
        continue;
//      std::cerr << std::hex << m.at<int32_t>(i,j) << std::dec << std::endl;
      for (off_t k = 0; k < 5; k++) {
        for (off_t l = 0; l < 5; l++) {
        	putpixel(surface, j * 5 + l, i * 5 + k, m.at<int32_t>(i,j));
        }
      }
    }
  }

  canvas->update();
}

int main(int argc, char** argv) {
	if(argc != 2) {
		std::cerr << "Usage: slide <png-file>" << std::endl;
		exit(1);
	}
	cv::Mat textureRGB = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
	cv::Mat texture;
	cv::cvtColor(textureRGB, texture, CV_RGB2RGBA);
//	texture.convertTo(texture, CV_32S);

  midiin->openPort( 0 );
  midiin->ignoreTypes( false, false, false );
  done = false;
  signal(SIGINT, finish);

  Canvas* canvas = new Canvas(WIDTH * 5, HEIGHT * 10,false);

  std::thread midiThread([&]() {
    int nBytes;

    while ( !done ) {
      midiin->getMessage( &message );

      nBytes = message.size();

      if(nBytes == 10) {
      	eventMutex.lock();
      	event.ctrlNr_ = message[1];
//      	event.roll_ = avgRoll.smooth(message[2]);
//      	event.pitch_ = avgPitch.smooth(message[3]);
//      	event.x_a_ = avgAccX.smooth(message[4]);
//      	event.y_a_ = avgAccY.smooth(message[5]);
//      	event.z_a_ = avgAccZ.smooth(message[6]);
      	event.roll_ = message[2];
      	event.pitch_ = message[3];
      	event.x_a_ = message[4];
      	event.y_a_ = message[5];
      	event.z_a_ = message[6];
        uint16_t buttons = (((uint16_t) message[7])) | ((uint16_t) message[8] << 8);

        event.btn_up_.update(buttons & MASK_BTN_UP);
        event.btn_down_.update(buttons & MASK_BTN_DOWN);
        event.btn_left_.update(buttons & MASK_BTN_LEFT);
        event.btn_right_.update(buttons & MASK_BTN_RIGHT);
        event.btn_a_.update(buttons & MASK_BTN_A);
        event.btn_b_.update(buttons & MASK_BTN_B);
        event.btn_minus_.update(buttons & MASK_BTN_MINUS);
        event.btn_plus_.update(buttons & MASK_BTN_PLUS);
        event.btn_home_.update(buttons & MASK_BTN_HOME);
        event.btn_1_.update(buttons & MASK_BTN_1);
        event.btn_2_.update(buttons & MASK_BTN_2);
      	eventMutex.unlock();
      }

      std::this_thread::yield();
      usleep( 10000 );
    }
  });

	std::thread slideThread([&]() {
		while(true) {
			eventMutex.lock();
			off_t xa = (event.roll_ - 64);
			off_t ya = (event.pitch_ - 64) * -1;
			std::cerr << "ya:" <<  ya << std::endl;

			eventMutex.unlock();
			vpMutex.lock();

				VIEWPORT.speedX_ += xa;

				VIEWPORT.speedY_ += ya;

			if(std::abs(VIEWPORT.speedX_) / 20 < 0.01)
				VIEWPORT.speedX_ = 0;
			if(std::abs(VIEWPORT.speedY_) / 50 < 0.01)
				VIEWPORT.speedY_ = 0;

			VIEWPORT.x_ += ceil(VIEWPORT.speedX_ / 20);
			VIEWPORT.y_ += ceil(VIEWPORT.speedY_ / 50);

			if(VIEWPORT.x_ >= texture.cols) {
				VIEWPORT.x_ = VIEWPORT.x_ % texture.cols;
			} else if(VIEWPORT.x_ < 0) {
				VIEWPORT.x_ = texture.cols + VIEWPORT.x_;
			}
			if(VIEWPORT.y_ >= texture.rows) {
				VIEWPORT.y_ = VIEWPORT.y_ % texture.rows;
			} else if(VIEWPORT.y_ < 0) {
				VIEWPORT.y_ = texture.rows + VIEWPORT.y_;
			}
			VIEWPORT.speedX_ /= 2;
			VIEWPORT.speedY_ /= 2;
			vpMutex.unlock();
			std::this_thread::yield();
			usleep(40000);
		}
	});

  SDL_Event event;
  cv::Mat* view = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
  cv::Mat* last = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
  cv::Mat *dst = new cv::Mat(HEIGHT, WIDTH, CV_8UC4);
  while(!done) {
    if(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        exit(0);
      }
    }
    extractROI(texture, *view);
    if(!last->empty()) {
    	for(size_t i = 1; i < 4; ++i) {
    		blend(*last, *view, 1.0/i, *dst);
        draw(canvas, *dst);
        usleep(10000);
    	}
    }
    draw(canvas, *view);
    view->copyTo(*last);
    std::this_thread::yield();
    usleep(10000);
  }

  return 0;
}
