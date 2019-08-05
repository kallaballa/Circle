#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <unistd.h>
#include <vector>
#include <deque>
#include <iostream>
#include <limits>
#include <thread>
#include <pthread.h>
#include <bitset>
#include <signal.h>
#include <opencv2/opencv.hpp>
#include <rtmidi/RtMidi.h>
#include "third/history/NLCommand.h"
#include "third/history/NLCommandGroup.h"
#include "third/history/NLHistory.h"
#include "canvas.hpp"
#include "button.hpp"
#include "average.hpp"
#include "trans.hpp"
#include "aplay/aplay.h"

AverageReadings avgRoll;
AverageReadings avgPitch;
AverageReadings avgAccX;
AverageReadings avgAccY;
AverageReadings avgAccZ;

TransMatrices TRANS_MATS;

RtMidiIn *midiin = new RtMidiIn();
std::vector<uint8_t> message;
bool done;
static void finish(int ignore){ done = true; }

typedef cv::Mat BaseGrid_t;
bool snap = false;
cv::Mat snapMat;

typedef struct _PartialGrid {
  BaseGrid_t* base;
  off_t xbegin, xend, ybegin, yend; // yend strictly not used
} PartialGrid;

void sierpinski_hollow(PartialGrid G) {
  int len = G.xend - G.xbegin + 1;
  int unit = len / 3;

  for (off_t i = G.xbegin + unit; i < G.xbegin + 2 * unit; i++) {
    for (off_t j = G.ybegin + unit; j < G.ybegin + 2 * unit; j++) {
      (*G.base).at<char>(i,j) = 0;
    }
  }
}

void sierpinski(PartialGrid G, int iterations) {
  if (iterations == 0)
    return;
  if ((iterations) == 1) {
    sierpinski_hollow(G);
    sierpinski(G, 0);
  }
  sierpinski_hollow(G);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      int length = G.xend - G.xbegin + 1;
      int unit = length / 3;
      PartialGrid q = { G.base, G.xbegin + i * unit, G.xbegin + (i + 1) * unit - 1, G.ybegin + j * unit, G.ybegin + (j + 1) * unit - 1 };
      sierpinski(q, iterations - 1);
    }
  }
}

int intpow(int base, int expo) {
  if (expo == 0) {
    return 1;
  }
  return base * intpow(base, expo - 1);
}

BaseGrid_t* allocate_grid(int n) {
  int size = intpow(3, n + 1);
  BaseGrid_t* grid = new BaseGrid_t(cv::Size(size,size), CV_8U);

  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; j++) {
      grid->at<char>(i,j) = 1;
    }
  }

  return grid;
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

void draw_grid(Canvas* canvas, BaseGrid_t* baseGrid) {
  canvas->fillRectangle(0,0, canvas->screenWidth, canvas->screenHeight, 0,0,0,255);
  cv::Mat dest;
  cv::Mat dest2;
  cv::Size size(baseGrid->cols,baseGrid->rows);

  //std::cerr << combined.at<double>(0,0) << ',' << combined.at<double>(1,0) << std::endl << combined.at<double>(0,1) << ',' << combined.at<double>(1,1) << std::endl << std::endl;
  warpAffine(*baseGrid, dest, TRANS_MATS.getMat(TransMatrices::Rotation), size, cv::INTER_LINEAR, cv::BORDER_WRAP);
  warpAffine(dest, dest2, TRANS_MATS.getMat(TransMatrices::StretchX), size, cv::INTER_LINEAR, cv::BORDER_WRAP);
  warpAffine(dest2, dest, TRANS_MATS.getMat(TransMatrices::StretchY), size, cv::INTER_LINEAR, cv::BORDER_WRAP);
  warpAffine(dest, dest2, TRANS_MATS.getMat(TransMatrices::ShearX), size, cv::INTER_LINEAR, cv::BORDER_WRAP);
  warpAffine(dest2, dest, TRANS_MATS.getMat(TransMatrices::ShearY), size, cv::INTER_LINEAR, cv::BORDER_WRAP);
  warpAffine(dest, dest2, TRANS_MATS.getMat(TransMatrices::Zoom), size, cv::INTER_LINEAR, cv::BORDER_WRAP);

  dest = dest2;
/*  std::cerr << "#########################" << std::endl;
  std::cerr << TRANS_MATS.getMat(TransMatrices::Rotation) << std::endl;
  std::cerr << TRANS_MATS.getMat(TransMatrices::StretchX) << std::endl;
  std::cerr << TRANS_MATS.getMat(TransMatrices::StretchY) << std::endl;
  std::cerr << TRANS_MATS.getMat(TransMatrices::ShearX) << std::endl;
  std::cerr << TRANS_MATS.getMat(TransMatrices::ShearY) << std::endl;
  std::cerr << "#########################" << std::endl  << std::endl;*/
//  dest = dest2;
  SDL_Surface* surface = canvas->getSurface();
  Uint32 *p;

  if(snap) {
    snap = false;
    snapMat = dest;
  }

  double colorInc = 16777215 / (((double)dest.rows) * ((double)dest.cols));

  double color = 0;
  for (off_t i = 0; i < dest.rows; i++) {
    if(i >= surface->w)
      continue;
    for (off_t j = 0; j < dest.cols; j++) {
      if(j >= surface->h)
        continue;
      Uint32 pos = (((surface->h/2 - dest.rows/2 + j) * surface->pitch) + (surface->w/2 - dest.cols/2 + i));
//      Uint32 pos = j * surface->pitch + i * sizeof(*p);
      if(pos < 0)
        continue;
      p = ((Uint32*)surface->pixels) + pos;
      if(!dest.at<char>(i,j)) {
        putpixel(surface, j, i, 0);
      } else {
        putpixel(surface, j, i, 16777215);
      }
      color += colorInc;
    }
//    Uint32 test;
//    assert(!__builtin_uadd_overflow(color, 1,&test));
//    color = test;
  }

  canvas->update();
}

int main() {
  int n = 5;
  size_t w = 1366;
  size_t h = 768;
  midiin->openPort( 0 );
  midiin->ignoreTypes( false, false, false );
  done = false;
  signal(SIGINT, finish);

  Canvas* canvas = new Canvas(w,h,false);
  char *argv[] = {"fumble", "-r", "8000", "-c", "1", "-f", "U8"};
  playback_init(7, argv);
  playback_open();
  chunk = new uint8_t[chunk_bytes];
  std::thread fractalThread([&](){
    BaseGrid_t* baseGrid = allocate_grid(n);
    PartialGrid b = { baseGrid, 0, baseGrid->rows - 1, 0, baseGrid->rows - 1};
    sierpinski(b, n);
    cv::Mat t(2,3,CV_64F, cvScalar(0.0));

    while(true) {
      draw_grid(canvas, baseGrid);
      std::this_thread::yield();
      usleep(4000);
    }

  });

  std::thread alsaThread([&](){
    while(true) {
      if(snapMat.cols > 0) {
        for(off_t i = 0; i < snapMat.cols; ++i) {
          for(off_t j = 0; j < snapMat.rows; ++j) {
            push_pcm(chunk, snapMat.at<uint8_t>(i,j) * 255);
          }
        }
      }
      else {
        usleep(10000);
      }
      std::this_thread::yield();
    }
  });

  std::thread midiThread([&]() {
    int nBytes;
    double stretchX = 0;
    double stretchY = 0;
    double shearX = 0;
    double shearY = 0;
    double rotation = 0;
    off_t zoom = 10;
    Button btn_up;
    Button btn_down;
    Button btn_left;
    Button btn_right;
    Button btn_a;
    Button btn_b;
    Button btn_minus;
    Button btn_plus;
    Button btn_home;
    Button btn_1;
    Button btn_2;

    while ( !done ) {
      midiin->getMessage( &message );

      nBytes = message.size();
    	std::cerr << "message size: " << message.size() << std::endl;

      if(nBytes == 10) {
      	int ctrlNr = message[1];
      	std::cerr << "ctrl: " << ctrlNr << std::endl;
        string button;
        double roll = avgRoll.smooth(message[2]);
        double pitch = avgPitch.smooth(message[3]);
        double x_a = avgAccX.smooth(message[4]);
        double y_a = avgAccY.smooth(message[5]);
        double z_a = avgAccZ.smooth(message[6]);
//        double roll = message[1];
//        double pitch = message[2];
//        double x_a = message[3];
//        double y_a = message[4];
//        double z_a = message[5];
        uint16_t buttons = (((uint16_t) message[7])) | ((uint16_t) message[8] << 8);


        btn_up.update(buttons & MASK_BTN_UP);
        btn_down.update(buttons & MASK_BTN_DOWN);
        btn_left.update(buttons & MASK_BTN_LEFT);
        btn_right.update(buttons & MASK_BTN_RIGHT);
        btn_a.update(buttons & MASK_BTN_A);
        btn_b.update(buttons & MASK_BTN_B);
        btn_minus.update(buttons & MASK_BTN_MINUS);
        btn_plus.update(buttons & MASK_BTN_PLUS);
        btn_home.update(buttons & MASK_BTN_HOME);
        btn_1.update(buttons & MASK_BTN_1);
        btn_2.update(buttons & MASK_BTN_2);

//        std::cerr << "btn_up:" << btn_up << " btn_down:" << btn_down << " btn_left:" << btn_left << " btn_right:" << btn_right << " btn_a:" << btn_a << " btn_b:" << btn_b << " btn_minus:" << btn_minus << " btn_plus:" << btn_plus << " btn_home:" << btn_home << " btn_1:" << btn_1 << " btn_2:" << btn_2 << std::endl;
//        std::cerr << "x_a: " << x_a << " y_a:" << y_a << " z_a:" << z_a << std::endl;
        if(btn_b.press_) {
       //   std::cerr << "trans" << std::endl;
          stretchX = (roll/ 256.0) * M_PI;
          TRANS_MATS.set(TransMatrices::StretchX,0,0,1);
          TRANS_MATS.set(TransMatrices::StretchX,1,0,0);
          TRANS_MATS.set(TransMatrices::StretchX,0,1,0);
          TRANS_MATS.set(TransMatrices::StretchX,1,1,sin(stretchX));

          stretchY = (pitch/ 128) * M_PI;
          TRANS_MATS.set(TransMatrices::StretchY,0,0,sin(stretchY));
          TRANS_MATS.set(TransMatrices::StretchY,1,0,0);
          TRANS_MATS.set(TransMatrices::StretchY,0,1,0);
          TRANS_MATS.set(TransMatrices::StretchY,1,1,1);

          shearX = (y_a/ 128.0) * M_PI + M_PI_2;
          TRANS_MATS.set(TransMatrices::ShearX,0,0,1);
          TRANS_MATS.set(TransMatrices::ShearX,1,0,0);
          TRANS_MATS.set(TransMatrices::ShearX,0,1,sin(shearX));
          TRANS_MATS.set(TransMatrices::ShearX,1,1,1);

          shearY = (x_a / 128.0) * M_PI + M_PI_2;
          TRANS_MATS.set(TransMatrices::ShearY,0,0,1);
          TRANS_MATS.set(TransMatrices::ShearY,1,0,sin(shearY));
          TRANS_MATS.set(TransMatrices::ShearY,0,1,0);
          TRANS_MATS.set(TransMatrices::ShearY,1,1,1);

          rotation = (z_a/ 128.0) * M_PI - 1.9 + M_PI;
          TRANS_MATS.set(TransMatrices::Rotation,0,0,cos(rotation));
          TRANS_MATS.set(TransMatrices::Rotation,1,0,sin(rotation));
          TRANS_MATS.set(TransMatrices::Rotation,0,1,-sin(rotation));
          TRANS_MATS.set(TransMatrices::Rotation,1,1,cos(rotation));

//          std::cerr << "stretchX:" << stretchX << " stretchY:" << stretchY << " shearX:" << shearX << " shearY:" << shearY << " rotation:" << rotation << std::endl;
        }

        if(btn_a.release_) {
          TRANS_MATS.saveState();
          snap = true;
        }

        if(btn_left.release_) {
          TRANS_MATS.undo();
          snap = true;
        }

        if(btn_right.release_) {
          TRANS_MATS.redo();
          snap = true;
        }

        if(btn_home.release_) {
          TRANS_MATS.reset();
          snap = true;
        }

        if(btn_plus.press_) {
          ++zoom;
          if(zoom >= 30)
            zoom = 10;

          TRANS_MATS.set(TransMatrices::Zoom,0,0,zoom * 0.1);
          TRANS_MATS.set(TransMatrices::Zoom,1,0,0);
          TRANS_MATS.set(TransMatrices::Zoom,0,1,0);
          TRANS_MATS.set(TransMatrices::Zoom,1,1,zoom * 0.1);
        }

        if(btn_minus.press_) {
          --zoom;
          TRANS_MATS.set(TransMatrices::Zoom,0,0,zoom * 0.1);
          TRANS_MATS.set(TransMatrices::Zoom,1,0,0);
          TRANS_MATS.set(TransMatrices::Zoom,0,1,0);
          TRANS_MATS.set(TransMatrices::Zoom,1,1,zoom * 0.1);
        }
      }
      std::this_thread::yield();
      usleep( 10000 );
    }
  });

  SDL_Event event;
  while(true){
    if(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        exit(0);
      }
    }
    std::this_thread::yield();
    usleep(100000);
  }

  return 0;
}
