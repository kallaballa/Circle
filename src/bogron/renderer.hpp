#ifndef SRC_BOGRON_RENDERER_HPP_
#define SRC_BOGRON_RENDERER_HPP_

#include "object.hpp"
#include "game.hpp"
#include "palette.hpp"
#include <opencv2/opencv.hpp>

class Renderer {
	size_t width_;
	size_t height_;
	cv::Mat* frameBuffer_;
public:
	Renderer(size_t w, size_t h);
	virtual ~Renderer();
	void renderPlayer(const Player& p, const RGBColor& color);
	void renderMap(Game::grid_t& grid, const Player& p1, const Player& p2);
	cv::Mat* getFrameBuffer() {
		return frameBuffer_;
	}
};

#endif /* SRC_BOGRON_RENDERER_HPP_ */
