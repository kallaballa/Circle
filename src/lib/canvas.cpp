#include "canvas.hpp"
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
#include <iostream>
#include "save_surf.hpp"

Canvas::Canvas(size_t screenWidth, size_t screenHeight, bool offscreen) :
  screen(NULL), screenWidth(screenWidth), screenHeight(screenHeight), offscreen(offscreen) {
  if (screenWidth > 0 && screenHeight > 0) {
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
      printf("Can't init SDL:  %s\n", SDL_GetError());
      exit(1);
    }
    atexit(SDL_Quit);

    if(!offscreen)
      screen = SDL_SetVideoMode(screenWidth, screenHeight, 32,0);
    else
      screen = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth, screenHeight, 32, 0, 0, 0, 0);

    if (screen == NULL) {
      printf("Can't set video mode: %s\n", SDL_GetError());
      exit(1);
    }
  }
}

void Canvas::fillRectangle(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    Sint16 xv[4] = { x, x + w, x + w, x };
    Sint16 yv[4] = { y, y, y + h,  y + h};
    filledPolygonRGBA(screen,xv,yv,4,r,g,b,a);
}


void Canvas::update() {
  if(!offscreen)
    SDL_Flip(screen);
}

void Canvas::save(const string& filename) {
  png_save_surface(filename.c_str(), screen);
}

void Canvas::putpixel(int x, int y, Uint32 pixel) {
	int bpp = screen->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *) screen->pixels + y * screen->pitch + x * bpp;
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

void Canvas::draw(cv::Mat& m, const size_t& magnification, const size_t& offX, const size_t& offY) {
	this->fillRectangle(0, 0, this->screenWidth, this->screenHeight, 0, 0,
			0, 255);

	SDL_Surface* surface = this->getSurface();
	Uint32 *p = NULL;

	for (off_t i = 0; i < m.rows; i++) {
		if (i >= surface->w)
			continue;
		for (off_t j = 0; j < m.cols; j++) {
			if (j >= surface->h)
				continue;
			for (off_t k = 0; k < magnification; k++) {
				for (off_t l = 0; l < magnification; l++) {
					this->putpixel(offX + j * magnification + l, offY + i * magnification + k , m.at<int32_t>(i, j));
				}
			}
		}
	}

	this->update();
}
