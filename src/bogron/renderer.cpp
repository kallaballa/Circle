#include "renderer.hpp"
#include "font.hpp"

Renderer* Renderer::instance_ = nullptr;

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

            double opacity = ((double)overlay->data[fY * overlay->step + fX * overlay->channels() + 3]) / 255.0;

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
	delete frameBuffer_;
}

void Renderer::renderAnimations(const off_t& off) {
	if(animations_.empty())
		return;

	cv::Mat layer(height_, width_, CV_8UC4, cv::Scalar(0,0,0,0));

	for(auto it = animations_.begin(); it != animations_.end(); ++it) {
		Animation* anim = *it;
		cv::Mat f;
		if(anim->next(f)) {
			off_t wf = f.cols;
			off_t hf = f.rows;
			off_t xf = 0;
			off_t yf = 0;

			off_t wl = f.cols;
			off_t hl = f.rows;
			off_t xl = off + anim->pos_.first - (f.cols / 2);
			off_t yl = anim->pos_.second - (f.rows / 2);

			if(xl < 0) {
				wf -= std::abs(xl);
				xf += std::abs(xl);
				wl = wf;
				xl = 0;
			}

			if(xl > (layer.cols - wf)) {
				wf -= xl - (layer.cols - wf);
				xf += xl - (layer.cols - wf);
				wl = wf;
				xl = layer.cols - wf;
			}

			if(yl < 0) {
				hf -= std::abs(yl);
				yf += std::abs(yl);
				hl = hf;
				yl = 0;
			}

			if(yl > (layer.rows - hf)) {
				hf -= yl - (layer.rows - hf);
				yf += yl - (layer.rows - hf);
				hl = hf;
				yl = layer.rows - hf;
			}

			f(cv::Rect(xf,yf,wf,hf)).copyTo(layer(cv::Rect(xl,yl,wl,hl)));
		} else {
			delete anim;
			it = animations_.erase(it);
		}
	}
	overlayImage(frameBuffer_, &layer, cv::Point{0,0});
}
void Renderer::renderPlayer(Player& p, const RGBColor& color, const size_t& off) {
	if(p.isExploding) {
		p.isExploding = false;
		addAnimation(new VideoAnimation("./anim/explosion.gif", p.pos_.first, p.pos_.second, false));
	}
	cv::Mat layer(height_, width_, CV_8UC4, cv::Scalar(0,0,0,0));

	layer.at<uint8_t>(p.pos_.second, (off + p.pos_.first) * 4) = color.b_;
	layer.at<uint8_t>(p.pos_.second, (off + p.pos_.first) * 4 + 1) = color.g_;
	layer.at<uint8_t>(p.pos_.second, (off + p.pos_.first) * 4 + 2) = color.r_;
	layer.at<uint8_t>(p.pos_.second, (off + p.pos_.first) * 4 + 3) = 0xFF;

	if(p.hasBomb_ || p.hasNuke_) {
		RGBColor haloColor(0,0,0);
		if(p.hasBomb_)
			haloColor = Palette::BOMB_;
		else if(p.hasNuke_)
			haloColor = Palette::NUKE_;
		else
			haloColor = Palette::SHIELD_;

		off_t y = p.pos_.second;
		off_t x = (off + p.pos_.first) * 4 + 4;
		if(x < width_ * 4) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second;
		x = (off + p.pos_.first) * 4 - 4;
		if(x >= 0) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second + 1;
		x = (off + p.pos_.first) * 4;
		if(y < height_) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}

		y = p.pos_.second - 1;
		x = (off + p.pos_.first) * 4;
		if(y >= 0) {
			layer.at<uint8_t>(y, x) = haloColor.b_;
			layer.at<uint8_t>(y, x + 1) = haloColor.g_;
			layer.at<uint8_t>(y, x + 2) = haloColor.r_;
			layer.at<uint8_t>(y, x + 3) = 0x7F;
		}
	}
	overlayImage(frameBuffer_, &layer, cv::Point{0,0});
}

void Renderer::renderGame(Game::grid_t& grid, Player& p1, Player& p2) {
	size_t diff2 = floor((width_ - grid.front().size()) / 2.0);

	for (size_t r = 0; r < grid.size(); ++r) {
		for (size_t c = 0; c < grid.front().size(); ++c) {
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
			case Object::SHIELD_:
				color = Palette::SHIELD_;
				break;
			case Object::NUKE_:
				color = Palette::NUKE_;
				break;
			case Object::BLANK_:
				color = Palette::BLANK_;
				break;
			case Object::EXPLOSION_BOMB_:
			case Object::EXPLOSION_NUKE_:
				color = Palette::BLANK_;
				break;
			case Object::BLAST_:
				color = Palette::BLANK_;
				cell = Object::BLANK_;
				break;
			default:
				assert(false);
				break;
			}
			frameBuffer_->at<uint8_t>(r, (diff2 + c) * 4) = color.b_;
			frameBuffer_->at<uint8_t>(r, (diff2 + c) * 4 + 1) = color.g_;
			frameBuffer_->at<uint8_t>(r, (diff2 + c) * 4 + 2) = color.r_;
			if(color.r_ == Palette::BLANK_.r_ && color.g_ == Palette::BLANK_.g_ && color.b_ == Palette::BLANK_.b_)
				frameBuffer_->at<uint8_t>(r, (diff2 + c) * 4 + 3) = 0x00;
			else
				frameBuffer_->at<uint8_t>(r, (diff2 + c) * 4 + 3) = 0xff;
		}
	}
	renderPlayer(p1, Palette::PLAYER_1, diff2);
	renderPlayer(p2, Palette::PLAYER_2, diff2);
	renderAnimations(diff2);
	Font f;
	cv::rectangle(*frameBuffer_, cv::Rect(0,0,13,height_), cv::Scalar(0x70,0x70,0x70), -1);
	cv::rectangle(*frameBuffer_, cv::Rect(diff2 + grid.front().size(),0,diff2 + grid.front().size() + 13,height_), cv::Scalar(0x70,0x70,0x70), -1);
	f.drawtext(*frameBuffer_, 0,1,(char)0x03 + std::to_string(p1.lifes()), Palette::PLAYER_1);
	f.drawtext(*frameBuffer_, 0,diff2 + grid.front().size() + 1,(char)0x03 + std::to_string(p2.lifes()), Palette::PLAYER_2);
}

void Renderer::renderGameOver(Game& game) {
	size_t diff2 = floor((width_ - game.grid().front().size()) / 2.0);
	string msg;
	if (game.player1().isDead())
		msg = "Player 2 wins!";
	else
		msg = "Player 1 wins!";

	cv::rectangle(*frameBuffer_, cv::Rect(0,0,width_,height_), cv::Scalar(0x70,0x70,0x70), -1);
	Font f;
	f.drawtext(*frameBuffer_, 0, diff2 + 10 ,msg, RGBColor(255,0,0));
}
