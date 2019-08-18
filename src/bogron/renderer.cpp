#include "renderer.hpp"
#include "font.hpp"

void overlayImage(cv::Mat* src, cv::Mat* overlay, const cv::Point& location)
{
	using namespace cv;
	using namespace std;
    for (int y = max(location.y, 0); y < src->rows; ++y)
    {
        int fY = y - location.y;

        if (fY >= overlay->rows)
            break;

        for (int x = max(location.x, 0); x < src->cols; ++x)
        {
            int fX = x - location.x;

            if (fX >= overlay->cols)
                break;

            double opacity = ((double)overlay->data[fY * overlay->step + fX * overlay->channels() + 3]) / 255;

            for (int c = 0; opacity > 0 && c < src->channels(); ++c)
            {
                unsigned char overlayPx = overlay->data[fY * overlay->step + fX * overlay->channels() + c];
                unsigned char srcPx = src->data[y * src->step + x * src->channels() + c];
                src->data[y * src->step + src->channels() * x + c] = srcPx * (1. - opacity) + overlayPx * opacity;
            }
        }
    }
}

Renderer::Renderer(size_t w, size_t h) : width_(w), height_(h), frameBuffer_(new cv::Mat(h, w, CV_8UC4, 0.0)) {
}

Renderer::~Renderer() {
	// TODO Auto-generated destructor stub
}

void Renderer::renderPlayer(const Player& p, const RGBColor& color) {
	Font f;
	f.drawtext(*frameBuffer_, 0,8,"54321", 0.8, 1, RGBColor{255,0,0});

	cv::Mat layer(height_, width_, CV_8UC4, cv::Scalar(0,0,0,0));

	HSLColor hsl2(color);
	hsl2.l_ = 70 / (6 - p.lifes_);
	hsl2.s_ = 99;
	RGBColor rgb2(hsl2);
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4) = rgb2.b_;
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4 + 1) = rgb2.g_;
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4 + 2) = rgb2.r_;
	layer.at<uint8_t>(p.pos_.second, p.pos_.first * 4 + 3) = 0xFF;

	if(p.hasBomb_ || p.hasNuke_) {
		RGBColor haloColor(0,0,0);
		if(p.hasBomb_)
			haloColor = Palette::BOMB_;
		else
			haloColor = Palette::NUKE_;

		off_t y = p.pos_.second;
		off_t x = p.pos_.first * 4 + 4;
		if(x < width_ * 4) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second;
		x = p.pos_.first * 4 - 4;
		if(x > 0) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second + 1;
		x = p.pos_.first * 4;
		if(y < height_) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second - 1;
		x = p.pos_.first * 4;
		if(y > 0) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}
	}
	overlayImage(frameBuffer_, &layer, cv::Point{0,0});
//	blend(layer, copy, 0.5, *frameBuffer);
}

void Renderer::renderMap(Game::grid_t& grid, const Player& p1, const Player& p2) {
	for (size_t r = 0; r < height_; ++r) {
		for (size_t c = 0; c < width_; ++c) {
			uint8_t& cell = grid[r][c];
			RGBColor color(0);
			switch (cell) {
			case Object::PLAYER_1:
				color = Palette::PLAYER_1;
				break;
			case Object::PLAYER_2:
				color = Palette::PLAYER_2;
				break;
			case Object::MINE_:
				color = Palette::MINE_;
				break;
			case Object::BOMB_:
				color = Palette::BOMB_;
				break;
			case Object::NUKE_:
				color = Palette::NUKE_;
				break;
			case Object::BLANK_:
				color = Palette::BLANK_;
				break;
			case Object::EXPLOSION_BOMB_:
			case Object::EXPLOSION_NUKE_:
				color = Palette::BLAST_;
				break;
			case Object::BLAST_:
				color = Palette::BLAST_;
				cell = Object::BLANK_;
				break;
			default:
				assert(false);
				break;
			}
			frameBuffer_->at<uint8_t>(r, c * 4) = color.b_;
			frameBuffer_->at<uint8_t>(r, c * 4 + 1) = color.g_;
			frameBuffer_->at<uint8_t>(r, c * 4 + 2) = color.r_;
			if(color.r_ == Palette::BLANK_.r_ && color.g_ == Palette::BLANK_.g_ && color.b_ == Palette::BLANK_.b_)
				frameBuffer_->at<uint8_t>(r, c * 4 + 3) = 0x00;
			else
				frameBuffer_->at<uint8_t>(r, c * 4 + 3) = 0xff;
		}
	}
	renderPlayer(p1, Palette::PLAYER_1);
	renderPlayer(p2, Palette::PLAYER_2);
}
