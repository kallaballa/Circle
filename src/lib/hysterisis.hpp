#ifndef SRC_LIB_HYSTERISIS_HPP_
#define SRC_LIB_HYSTERISIS_HPP_

#define _BUFFER_LENGTH 3

#include <stdbool.h>

class Hysterisis {
  float peak;
  float bottom;
  float hysteresis;
  int readbufferindex;
  long readbuffer[_BUFFER_LENGTH];
  long average;
public:
  Hysterisis();
	virtual ~Hysterisis();
  void calculate();
  void set(int v);
  bool isUp();
  bool isDown();
};

#endif /* SRC_LIB_HYSTERISIS_HPP_ */
