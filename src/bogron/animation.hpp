#ifndef SRC_BOGRON_ANIMATION_HPP_
#define SRC_BOGRON_ANIMATION_HPP_

#include <opencv2/opencv.hpp>
#include <string>

class Animation {
public:
	std::pair<off_t, off_t> pos_;
	bool loop_;
	bool ended_ = false;
	Animation(int x, int y, bool loop) : pos_(x,y), loop_(loop) {

	}
	virtual ~Animation() {};
	virtual bool next(cv::Mat& m) = 0;
};

class VideoAnimation : public Animation {
private:
	cv::VideoCapture capture_;
public:
	VideoAnimation(const std::string& filename, int x, int y, bool loop);
	virtual ~VideoAnimation() {};
	virtual bool next(cv::Mat& m);
};

#endif /* SRC_BOGRON_ANIMATION_HPP_ */
