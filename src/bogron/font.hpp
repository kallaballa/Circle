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
//	static unsigned char DIGITS_SMALL[] = {
//	    0x1F, 0x11, 0x1F,   // 0
//	    0x12, 0x1F, 0x10,   // 1
//	    0x1D, 0x15, 0x17,   // 2
//	    0x15, 0x15, 0x1F,   // 3
//	    0x07, 0x04, 0x1F,   // 4
//	    0x17, 0x15, 0x1D,   // 5
//	    0x1F, 0x15, 0x1D,   // 6
//	    0x01, 0x01, 0x1F,   // 7
//	    0x1F, 0x15, 0x1F,   // 8
//	    0x17, 0x15, 0x1F    // 9
//	};

	std::vector<std::vector<uint8_t>> FONT = {
		{0x01, 0x00}
		,{0x00, 0x20, 0x44, 0x40, 0x44, 0x20, 0x00, 0x00} // HAPPY SMILEY
		,{0x00, 0x40, 0x24, 0x20, 0x24, 0x40, 0x00, 0x00} // SAD SMILEY
		,{0x18, 0x24, 0x48, 0x24, 0x18} // HEART
		,{0xD8, 0x34, 0xDE, 0x34, 0xD8} // INVADER 1
		,{0xFA, 0xB4, 0x3C, 0xB4, 0xFA} // INVADER 2
		,{0x9C, 0xF6, 0x1E, 0xF6, 0x9C} // INVADER 3
		,{0xF8, 0x74, 0xFC, 0x74, 0xF8} // GHOST
		,{0x18, 0xEC, 0x5C, 0xEC, 0x18} // SKULL

		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}
		,{0x00}

		,{0x00, 0x00, 0x00}              // <space>
		,{0xBE}                          // !
		,{0x06, 0x00, 0x06}              // "
		,{0x28, 0xFE, 0x28, 0xFE, 0x28}  // #
		,{0x48, 0x54, 0xFE, 0x54, 0x24}  // $
		,{0x46, 0x26, 0x10, 0xC8, 0xC4}  // %
		,{0x6C, 0x92, 0xAC, 0x40, 0xA0}  // &
		,{0x0A, 0x06}                    // '
		,{0x38, 0x44, 0x82}              // (
		,{0x82, 0x44, 0x38}              // )
		,{0x44, 0x28, 0xD6, 0x28, 0x44}  // *
		,{0x10, 0x10, 0x7C, 0x10, 0x10}  // +
		,{0x80, 0x40}                    // ,
		,{0x10, 0x10, 0x10, 0x10}        // -
		,{0x80}                          // .
		,{0xC0, 0x20, 0x10, 0x08, 0x06}  // /
		,{0x7C, 0x82, 0x82, 0x7C}        // 0
		,{0x84, 0xFE, 0x80}              // 1
		,{0xC4, 0xA2, 0x92, 0x8C}        // 2
		,{0x44, 0x82, 0x92, 0x6C}        // 3
		,{0x30, 0x28, 0x24, 0xFE}        // 4
		,{0x4E, 0x8A, 0x8A, 0x72}        // 5
		,{0x78, 0x94, 0x92, 0x62}        // 6
		,{0x02, 0xE2, 0x1A, 0x06}        // 7
		,{0x6C, 0x92, 0x92, 0x6C}        // 8
		,{0x0C, 0x92, 0x52, 0x3C}        // 9
		,{0x50}                          // :
		,{0x80, 0x50}                    // ;
		,{0x10, 0x28, 0x44, 0x82}        // <
		,{0x28, 0x28, 0x28, 0x28}        // =
		,{0x82, 0x44, 0x28, 0x10}        // >
		,{0x04, 0xA2, 0x12, 0x0C}        // ?
		,{0x64, 0x92, 0xF2, 0x82, 0x7C}  // @
		,{0xFC, 0x12, 0x12, 0xFC}        // A
		,{0xFE, 0x92, 0x92, 0x6C}        // B
		,{0x7C, 0x82, 0x82, 0x44}        // C
		,{0xFE, 0x82, 0x82, 0x7C}        // D
		,{0xFE, 0x92, 0x92, 0x82}        // E
		,{0xFE, 0x12, 0x12, 0x02}        // F
		,{0x7C, 0x82, 0x92, 0x74}        // G
		,{0xFE, 0x10, 0x10, 0xFE}        // H
		,{0x82, 0xFE, 0x82}              // I
		,{0x40, 0x80, 0x80, 0x7E}        // J
		,{0xFE, 0x10, 0x28, 0xC6}        // K
		,{0xFE, 0x80, 0x80, 0x80}        // L
		,{0xFE, 0x04, 0x18, 0x04, 0xFE}  // M
		,{0xFE, 0x0C, 0x30, 0xFE}        // N
		,{0x7C, 0x82, 0x82, 0x7C}        // O
		,{0xFE, 0x12, 0x12, 0x0C}        // P
		,{0x7C, 0x82, 0x42, 0xBC}        // Q
		,{0xFE, 0x12, 0x32, 0xCC}        // R
		,{0x4C, 0x92, 0x92, 0x64}        // S
		,{0x02, 0x02, 0xFE, 0x02, 0x02}  // T
		,{0x7E, 0x80, 0x80, 0x7E}        // U
		,{0x0E, 0x30, 0xC0, 0x30, 0x0E}  // V
		,{0x7E, 0x80, 0x70, 0x80, 0x7E}  // W
		,{0xC6, 0x28, 0x10, 0x28, 0xC6}  // X
		,{0x06, 0x08, 0xF0, 0x08, 0x06}  // Y
		,{0xC2, 0xB2, 0x8A, 0x86}        // Z
		,{0xFE, 0x82, 0x82}              // [
		,{0x06, 0x08, 0x10, 0x20, 0xC0}  // backslash
		,{0x82, 0x82, 0xFE}              // ]
		,{0x04, 0x02, 0x04}              // ^
		,{0x80, 0x80, 0x80, 0x80}        // _
		,{0x06, 0x08}                    // `
		,{0x40, 0xA8, 0xA8, 0xF0}        // a
		,{0xFE, 0x88, 0x88, 0x70}        // b
		,{0x70, 0x88, 0x88, 0x50}        // c
		,{0x70, 0x88, 0x88, 0xFE}        // d
		,{0x70, 0xA8, 0xA8, 0x90}        // e
		,{0x08, 0xFC, 0x0A, 0x02}        // f
		,{0x10, 0xA8, 0xA8, 0x78}        // g
		,{0xFE, 0x10, 0x10, 0xE0}        // h
		,{0xFA}                          // i
		,{0x40, 0x80, 0x7A}              // j
		,{0xFE, 0x10, 0x28, 0xC4}        // k
		,{0x82, 0xFE, 0x80}              // l
		,{0xF8, 0x08, 0xF0, 0x08, 0xF0}  // m
		,{0xF8, 0x08, 0x08, 0xF0}        // n
		,{0x70, 0x88, 0x88, 0x70}        // o
		,{0xF8, 0x48, 0x48, 0x30}        // p
		,{0x30, 0x48, 0x48, 0xF8}        // q
		,{0xF0, 0x08, 0x08}              // r
		,{0x90, 0xA8, 0xA8, 0x48}        // s
		,{0x08, 0x7E, 0x88, 0x88}        // t
		,{0x78, 0x80, 0x80, 0xF8}        // u
		,{0x18, 0x60, 0x80, 0x60, 0x18}  // v
		,{0x78, 0x80, 0x60, 0x80, 0x78}  // w
		,{0x88, 0x50, 0x20, 0x50, 0x88}  // x
		,{0x18, 0xA0, 0xA0, 0x78}        // y
		,{0xC8, 0xA8, 0x98, 0x88}        // z
		,{0x10, 0x6C, 0x82}              // {
		,{0xFF}                          // |
		,{0x82, 0x6C, 0x10}              // }
		,{0x10, 0x08, 0x10, 0x20, 0x10}  // ~
	};
public:
	Font();
	virtual ~Font();
	void drawtext(cv::Mat& target, size_t x, size_t y, const string& text, RGBColor fg);
};
#endif /* SRC_BOGRON_FONT_HPP_ */
