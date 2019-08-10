#include "recorder.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <thread>

#include <sys/time.h>
#include <ctime>

using std::cerr;
using std::endl;

Recorder::Recorder(RecorderCallback callback, uint32_t sampleRate, uint32_t samplesPerFrame) :
    callback_(callback),
    sampleRate_(sampleRate),
    samplesPerFrame_(samplesPerFrame){
  const ALCchar * devices;
  int i;

  std::cerr << "Opening capture device:" << std::endl;
  captureDev_ = alcCaptureOpenDevice(NULL, sampleRate, AL_FORMAT_MONO8, samplesPerFrame);
  if (captureDev_ == NULL) {
    std::cerr << "Unable to open device!:" << std::endl;
    exit(1);
  }

  devices = alcGetString(captureDev_, ALC_CAPTURE_DEVICE_SPECIFIER);
  std::cerr << "opened device" << devices << std::endl;
}

Recorder::~Recorder() {
  alcCaptureStop(captureDev_);
  alcCaptureCloseDevice(captureDev_);
}

std::vector<std::string> Recorder::list() {
  std::vector<std::string> result;
  const ALCchar * devices;
  const ALCchar * ptr;
  printf("Available capture devices:\n");
  devices = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
  ptr = devices;

  while (*ptr) {
    result.push_back(ptr);
    ptr += strlen(ptr) + 1;
  }
  return result;
}

void Recorder::capture() {
  std::thread captureThread([&](){
  alcCaptureStart(captureDev_);
  while (true) {
    alcGetIntegerv(captureDev_, ALC_CAPTURE_SAMPLES, 1, &samplesAvailable);

    if (samplesAvailable > 0) {
      alcCaptureSamples(captureDev_, captureBuffer, samplesAvailable);

      for(size_t i = 0; i < samplesAvailable; i++) {
        if(buffer.size() >= samplesPerFrame_) {
          callback_(buffer);
          buffer.clear();
        }

        buffer.push_back(((double)captureBuffer[i]));
      }
    }

//    usleep(10000);
  }
  });
  captureThread.detach();
}

