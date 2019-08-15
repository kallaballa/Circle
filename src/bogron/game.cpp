#include "game.hpp"

Game::Game(size_t width, size_t height) : width_(width), height_(height), player1_(0, height / 2,5), player2_(width_ - 1,height / 2,5) {
	for (size_t r = 0; r < height_; ++r) {
			grid_.push_back(column_t(width_, Object::BLANK_));
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
	case Object::EXPLOSION_:
		cell = Object::BLANK_;
		p.lifes_--;
		break;
	default:
		break;
	}
}

void Game::step() {
//	grid_t old_grid = grid_;
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

Game::grid_t Game::grid() {
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
			newPos.first = std::max((off_t)0, player1_.pos_.first - 1);
			break;
		case RIGHT:
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
