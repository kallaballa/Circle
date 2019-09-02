#ifndef CANVAS_H_
#define CANVAS_H_

#include <algorithm>
#include <string>
#include <opencv2/opencv.hpp>
#include <SDL/SDL.h>

using std::string;

class Canvas {
public:
  size_t width_;
  size_t height_;
  size_t magnification_;
  Canvas(size_t width, size_t height, size_t magnification, bool offscreen = false);
  virtual ~Canvas() {};
  void fillRectangle(Sint16 x, Sint16 y, Sint16 w, Sint16 h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
  void fillCircle(Sint16 x, Sint16 y, Sint16 r, Uint16 red, Uint16 green, Uint16 blue);

  void update();
  void save(const string& filename);
  SDL_Surface* getSurface() {
    return screen_;
  }
  void putpixel(int x, int y, Uint32 pixel);
  void draw(cv::Mat& m, const size_t& offX = 0, const size_t& off = 0);
private:
  class SDL_Surface *screen_;
  bool offscreen_;
};

#endif /* CANVAS_H_ */
