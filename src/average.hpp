/*
 * Average.hpp
 *
 *  Created on: Jul 29, 2017
 *      Author: elchaschab
 */

#ifndef SRC_AVERAGE_HPP_
#define SRC_AVERAGE_HPP_

const int NUM_READINGS = 10;

class AverageReadings {
private:
  double readings_[NUM_READINGS];
  int readIndex_ = 0;
  double total_ = 0;
public:
  double smooth(double v) {
    total_ = total_ - readings_[readIndex_];
    readings_[readIndex_] = v;
    total_ = total_ + readings_[readIndex_];
    readIndex_ = readIndex_ + 1;

    if (readIndex_ >= NUM_READINGS) {
      readIndex_ = 0;
    }

    // calculate the average:
    return total_ / NUM_READINGS;
  }
};




#endif /* SRC_AVERAGE_HPP_ */
