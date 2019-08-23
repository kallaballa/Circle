#include "animation.hpp"

VideoAnimation::VideoAnimation(const std::string& filename, int x, int y, bool loop) : Animation(x,y,loop), capture_(filename) {}

bool VideoAnimation::next(cv::Mat& m) {
	capture_ >> m;

	if(loop_ && m.empty()) {
		capture_.set(CV_CAP_PROP_POS_AVI_RATIO, 0);
		capture_ >> m;
 	}

	if(!m.empty()) {
		cv::cvtColor(m, m, CV_RGB2RGBA);

		for(size_t c = 0; c < m.cols; ++c) {
			for(size_t r = 0; r < m.rows; ++r) {
				if(m.at<uint8_t>(r, c * 4) == 0 && m.at<uint8_t>(r, c * 4 + 1) == 0 && m.at<uint8_t>(r, c * 4 + 2) == 0)
					m.at<uint8_t>(r, c * 4 + 3) = 0x00;
				else
					m.at<uint8_t>(r, c * 4 + 3) = 0x7f;
			}
		}
	}

	return (ended_ = !m.empty());
}
