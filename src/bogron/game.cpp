#include "game.hpp"
#include "effects.hpp"

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
		p.lifes_--;
		break;
	case Object::BOMB_:
		cell = Object::BLANK_;
		p.hasNuke_ = false;
		p.hasBomb_ = true;
		break;
	case Object::NUKE_:
		cell = Object::BLANK_;
		p.hasNuke_ = true;
		p.hasBomb_ = false;
		break;
	case Object::BLAST_:
		snd_.play(Effects::Index::EXPLOSION_);
		cell = Object::BLANK_;
		p.lifes_--;
		break;
	default:
		break;
	}
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

void Game::explode(Player& p) {
	snd_.play(Effects::Index::BLAST_);
	if(p.hasBomb_){
		for(size_t i = 0; i < grid_.size(); ++i) {
			auto& column = grid_[i];

			if(i == p.pos_.second) {
				for(auto& cell : column) {
					cell = Object::BLAST_;
				}
			} else {
				column[p.pos_.first] = Object::BLAST_;
			}
		}
	} else if(p.hasNuke_) {
		std::pair<off_t, off_t> ul = p.pos_;
		std::pair<off_t, off_t> lr = p.pos_;

		ul.first = std::max(ul.first - 4, (off_t)0);
		ul.second = 0;
		lr.first = std::min(lr.first + 5, (off_t)width_);
		lr.second = height_;

		for(size_t i = ul.second; i < lr.second; ++i) {
			auto& column = grid_[i];

			for(size_t j = ul.first; j < lr.first; ++j) {
				column[j] = Object::BLAST_;
			}
		}
	}
	p.hasBomb_ = false;
	p.hasNuke_ = false;
}

void Game::explode1() {
	explode(player1_);
}

void Game::explode2() {
	explode(player2_);
}

Game::grid_t& Game::grid() {
	return grid_;
}

Player Game::player1() {
	return player1_;
}

Player Game::player2() {
	return player2_;
}

void Game::move(Player& p, const Direction& d) {
	auto e = epoch();
	auto newPos = p.pos_;
		switch(d) {
		case LEFT:
			if(player1_.pos_.first == 0)
				newPos.first = width_ - 1;
			else
				newPos.first = std::max((off_t)0, player1_.pos_.first - 1);
			break;
		case RIGHT:
			if(player1_.pos_.first == width_ - 1)
				newPos.first = 0;
			else
				newPos.first = std::min((off_t)width_ - 1, player1_.pos_.first + 1);
			break;
		case UP:
			newPos.second = std::max((off_t)0, player1_.pos_.second - 1);
			break;
		case DOWN:
			newPos.second = std::min((off_t)height_ - 1, player1_.pos_.second + 1);
			break;
		}

		if((newPos.first == player1_.pos_.first && newPos.second == player1_.pos_.second) ||
				(newPos.first == player2_.pos_.first && newPos.second == player2_.pos_.second))
			return; //players can't move over each other so we ignore crashing inputs

		this->checkPlayer(p);
		p.pos_ = newPos;
		p.lastMove_ = e;
}
