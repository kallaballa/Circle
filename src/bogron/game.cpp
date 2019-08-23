#include "game.hpp"
#include "effects.hpp"
#include "renderer.hpp"

Game::Game(size_t width, size_t height) : width_(width), height_(height), player1_(width / 4, height / 2,5), player2_(width * 0.75,height / 2,5) {
	for (size_t r = 0; r < height_; ++r) {
		grid_.push_back(column_t(width_, Object::BLANK_));
	}

	for(const auto& fn : Effects::filenames) {
		snd_.loadFx(fn);
	}
}

Game::~Game() {
}

void Game::lock() {
	gridMtx_.lock();
}

void Game::unlock() {
	gridMtx_.unlock();
}

void Game::checkPlayer(Player& p) {
	auto& cell = grid_[p.pos_.second][p.pos_.first];

	switch(cell) {
	case Object::MINE_:
		snd_.play(Effects::Index::EXPLOSION_);
		cell = Object::BLANK_;
		p.kill();
		break;
	case Object::BOMB_:
		cell = Object::BLANK_;
		p.hasNuke_ = false;
		p.hasBomb_ = true;
		p.hasShield_ = false;
		break;
	case Object::NUKE_:
		cell = Object::BLANK_;
		p.hasNuke_ = true;
		p.hasBomb_ = false;
		p.hasShield_ = false;
		break;
	case Object::SHIELD_:
		cell = Object::BLANK_;
		p.hasNuke_ = false;
		p.hasBomb_ = false;
		p.hasShield_ = true;
		break;
	case Object::BLAST_:
		snd_.play(Effects::Index::EXPLOSION_);
		cell = Object::BLANK_;
		p.kill();
		break;
	default:
		break;
	}
	Renderer::getInstance().renderGame(grid_, player1_, player2_);
}

void Game::step() {
//	grid_t old_grid = grid_;
	this->checkPlayer(player1_);
	this->checkPlayer(player2_);

	column_t back = grid_.back();
	column_t front;
	for(size_t c = 0; c < width_; ++c) {
		front.push_back(spawn_.roulette());
	}
	grid_.pop_back();
	grid_.push_front(front);
	this->checkPlayer(player1_);
	this->checkPlayer(player2_);
}

void Game::explode(Player& pActive, Player& pPassive) {
	if(pActive.hasBomb_){
		snd_.play(Effects::Index::BLAST_);

		Renderer::getInstance().addAnimation(new VideoAnimation("anim/bomb.gif", pActive.pos_.first, pActive.pos_.second, false));

		for(size_t x = 0; x < grid_.size(); ++x) {
			auto& column = grid_[x];

			if(x == pActive.pos_.second) {
				for(size_t y = 0; y < column.size(); ++y) {
					auto& cell = column[y];
					if(pPassive.pos_.first == y && pPassive.pos_.second == x) {
						pPassive.kill();
						snd_.play(Effects::Index::EXPLOSION_);
					}

					cell = Object::BLAST_;
				}
			} else {
				if(pPassive.pos_.first == pActive.pos_.first) {
					pPassive.kill();
					snd_.play(Effects::Index::EXPLOSION_);
				}
				column[pActive.pos_.first] = Object::BLAST_;
			}
		}
	} else if(pActive.hasNuke_) {
		snd_.play(Effects::Index::BLAST_);

		Renderer::getInstance().addAnimation(new VideoAnimation("anim/nuke.gif", pActive.pos_.first, pActive.pos_.second, false));

		std::pair<off_t, off_t> ul = pActive.pos_;
		std::pair<off_t, off_t> lr = pActive.pos_;

		ul.first = std::max(ul.first - 4, (off_t)0);
		ul.second = 0;
		lr.first = std::min(lr.first + 5, (off_t)width_);
		lr.second = height_;

		for(size_t x = ul.second; x < lr.second; ++x) {
			auto& column = grid_[x];

			for(size_t y = ul.first; y < lr.first; ++y) {
				if(pPassive.pos_.first == y && pPassive.pos_.second == x) {
					snd_.play(Effects::Index::EXPLOSION_);
					pPassive.kill();
				}
				column[y] = Object::BLAST_;
			}
		}
	}
	pActive.hasBomb_ = false;
	pActive.hasNuke_ = false;
}

void Game::explode1() {
	explode(player1_, player2_);
}

void Game::explode2() {
	explode(player2_, player1_);
}

Game::grid_t& Game::grid() {
	return grid_;
}

Player& Game::player1() {
	return player1_;
}

Player& Game::player2() {
	return player2_;
}

void Game::move(Player& pActive, const Player& pPassive, const Direction& d) {
	auto e = epoch();
	auto newPos = pActive.pos_;
		switch(d) {
		case LEFT:
			if(pActive.pos_.first == 1)
				newPos.first = width_ - 1;
			else
				newPos.first = std::max((off_t)0, pActive.pos_.first - 1);
			break;
		case RIGHT:
			if(pActive.pos_.first == width_ - 1)
				newPos.first = 1;
			else
				newPos.first = std::min((off_t)width_ - 1, pActive.pos_.first + 1);
			break;
		case UP:
			newPos.second = std::max((off_t)0, pActive.pos_.second - 1);
			break;
		case DOWN:
			newPos.second = std::min((off_t)height_ - 1, pActive.pos_.second + 1);
			break;
		}

		if((newPos.first == pPassive.pos_.first && newPos.second == pPassive.pos_.second) ||
				(newPos.first == pPassive.pos_.first && newPos.second == pPassive.pos_.second))
			return; //players can't move over each other so we ignore crashing inputs

		this->checkPlayer(pActive);
		pActive.pos_ = newPos;
		pActive.lastMove_ = e;
}

void Game::reset() {
	grid_.clear();
	for (size_t r = 0; r < height_; ++r) {
		grid_.push_back(column_t(width_, Object::BLANK_));
	}
	player1_.reset();
	player2_.reset();
	Renderer::getInstance().clearAnimations();
}
