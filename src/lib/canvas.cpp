#include "canvas.hpp"
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
#include <iostream>
#include "save_surf.hpp"
#include "color.hpp"

Canvas::Canvas(size_t width, size_t height, size_t magnification, bool offscreen) :
  width_(width), height_(height), magnification_(magnification), screen_(NULL), offscreen_(offscreen) {
  if (width > 0 && height > 0) {
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
      printf("Can't init SDL:  %s\n", SDL_GetError());
      exit(1);
    }
    atexit(SDL_Quit);

    if(!offscreen)
      screen_ = SDL_SetVideoMode(width * magnification, height * magnification, 32,0);
    else
      screen_ = SDL_CreateRGBSurface(SDL_SWSURFACE, width * magnification, height * magnification, 32, 0, 0, 0, 0);

    if (screen_ == NULL) {
      printf("Can't set video mode: %s\n", SDL_GetError());
      exit(1);
    }
  }
}

void Canvas::fillRectangle(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    Sint16 xv[4] = { x, x + w, x + w, x };
    Sint16 yv[4] = { y, y, y + h,  y + h};
    filledPolygonRGBA(screen_,xv,yv,4,r,g,b,a);
}


void Canvas::fillCircle(Sint16 x, Sint16 y, Sint16 r, Uint32 color) {
	RGBColor rgb(color);
  filledCircleRGBA(screen_,x, y, r, rgb.r_, rgb.g_, rgb.b_, 255);
}

void Canvas::update() {
  if(!offscreen_)
    SDL_Flip(screen_);
}

void Canvas::save(const string& filename) {
  png_save_surface(filename.c_str(), screen_);
}

void Canvas::putpixel(int x, int y, Uint32 pixel) {
	int bpp = screen_->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *) screen_->pixels + y * screen_->pitch + x * bpp;
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

void Canvas::draw(cv::Mat& m) {
	this->fillRectangle(0, 0, this->width_ * magnification_, this->height_ * magnification_, 0, 0,
			0, 255);

	off_t diff = ((off_t)this->width_ - m.cols) / 2	;
	SDL_Surface* surface = this->getSurface();

	for (off_t i = 0; i < m.rows; i++) {
		if (i >= surface->w)
			continue;
		for (off_t j = 0; j < m.cols; j++) {
			if (j >= surface->h)
				continue;
				fillCircle((j * magnification_) + magnification_ / 2, (i * magnification_) + magnification_ / 2, magnification_/4, m.at<uint32_t>(i, j));

//			for (off_t k = 0; k < (off_t)magnification_; k++) {
//				for (off_t l = 0; l < (off_t)magnification_; l++) {
//					this->putpixel((diff * magnification_) + offX + j * magnification_ + l, offY + i * magnification_ + k , m.at<int32_t>(i, j));
//					if(l == (off_t)magnification_ - 1)
//						this->putpixel((diff * magnification_) + offX + j * magnification_ + l, offY + i * magnification_ + k , 0);
//					if(k == (off_t)magnification_ - 1)
//						this->putpixel((diff * magnification_) + offX + j * magnification_ + l, offY + i * magnification_ + k , 0);
//
//				}
//			}
		}
	}

	this->update();
}
