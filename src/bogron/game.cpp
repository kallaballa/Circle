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

void Game::step() {
//	grid_t old_grid = grid_;
	column_t back = grid_.back();
	column_t front;
	for(size_t c = 0; c < width_; ++c) {
		front.push_back(spawn_.roulette());
	}
	grid_.pop_back();
	grid_.push_front(front);
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
