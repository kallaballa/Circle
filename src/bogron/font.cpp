#include "font.hpp"
#include <exception>

Font::Font() {

}

Font::~Font() {

}

void Font::drawtext(cv::Mat& target, size_t x, size_t y, const char* text, double scale, size_t thickness, RGBColor fg)
{
	using namespace cv;
	putText(target, std::string(text), Point2f(x,y), FONT_HERSHEY_PLAIN, scale,  Scalar(fg.r_,fg.g_,fg.b_,255), thickness, 4, false);
}

