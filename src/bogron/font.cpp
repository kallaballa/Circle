#include "font.hpp"
#include <exception>

Font::Font() {

}

Font::~Font() {

}

void Font::drawtext(cv::Mat& target, size_t x, size_t y, const string& text, RGBColor fg)
{
	using namespace cv;
	size_t offset = 0;

	for (const auto& c : text) {
		std::vector<uint8_t> letter = FONT[c];
		for (size_t i = 0; i < letter.size(); ++i) {
			const uint8_t& column = letter[i];
			for (size_t j = 0; j < 8; ++j) {
				if (column & (uint8_t) pow(2, j)) {
					target.at<uint32_t>(x + j, offset + y + i) = fg.val();
				}
			}
		}
		offset += letter.size() + 1;
	}
}

