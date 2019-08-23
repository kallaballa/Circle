#ifndef SRC_BOGRON_RENDERER_HPP_
#define SRC_BOGRON_RENDERER_HPP_

#include "object.hpp"
#include "game.hpp"
#include "palette.hpp"
#include "animation.hpp"
#include <opencv2/opencv.hpp>
#include <list>

class Renderer {
	size_t width_;
	size_t height_;
	cv::Mat* frameBuffer_;
	std::list<Animation*> animations_;
	static Renderer* instance_;
	Renderer(size_t w, size_t h);

public:
	virtual ~Renderer();
	void renderAnimations(const off_t& off);
	void renderPlayer(Player& p, const RGBColor& color, const size_t& off);
	void renderGame(Game::grid_t& grid, Player& p1, Player& p2);
	void renderGameOver(Game& game);

	void addAnimation(Animation* anim) {
		animations_.push_back(anim);
	}

	void clearAnimations() {
		animations_.clear();
	}

	cv::Mat* getFrameBuffer() {
		return frameBuffer_;
	}

	static void init(size_t w, size_t h) {
		instance_ = new Renderer(w,h);
	}

	static Renderer& getInstance() {
		assert(instance_ != nullptr);
		return *instance_;
	}
};

#endif /* SRC_BOGRON_RENDERER_HPP_ */
