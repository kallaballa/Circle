/*
 * font.hpp
 *
 *  Created on: Aug 17, 2019
 *      Author: elchaschab
 */

#ifndef SRC_BOGRON_FONT_HPP_
#define SRC_BOGRON_FONT_HPP_

#include "../lib/color.hpp"

#include <string>
#include <opencv2/opencv.hpp>

class Font {
public:
	Font();
	virtual ~Font();
	void drawtext(cv::Mat& target, size_t x, size_t y, const char* text, double scale, size_t thickness, RGBColor fg);
};
#endif /* SRC_BOGRON_FONT_HPP_ */
