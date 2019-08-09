/*
 * Button.hpp
 *
 *  Created on: Jul 29, 2017
 *      Author: elchaschab
 */

#ifndef SRC_BUTTON_HPP_
#define SRC_BUTTON_HPP_

const uint16_t MASK_BTN_UP = 0b00000001;
const uint16_t MASK_BTN_DOWN = 0b00000010;
const uint16_t MASK_BTN_LEFT = 0b00000100;
const uint16_t MASK_BTN_RIGHT = 0b00001000;
const uint16_t MASK_BTN_A = 0b00010000;
const uint16_t MASK_BTN_B = 0b00100000;
const uint16_t MASK_BTN_MINUS = 0b01000000;
const uint16_t MASK_BTN_PLUS = (((uint16_t)0b00000001) << 8);
const uint16_t MASK_BTN_HOME = (((uint16_t)0b00000010) << 8);
const uint16_t MASK_BTN_1 = (((uint16_t)0b00000100) << 8);
const uint16_t MASK_BTN_2 = (((uint16_t)0b00001000) << 8);

struct Button {
  bool last_ = false;
  bool press_ = false;
  bool release_ = false;

  void update(bool u) {
    if(u) {
      press_ = true;
      release_ = false;
    } else if (last_) {
      press_ = false;
      release_ = true;
    } else {
      press_ = false;
      release_ = false;
    }
    last_ = u;
  }
};





#endif /* SRC_BUTTON_HPP_ */
