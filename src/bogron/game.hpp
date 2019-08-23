#ifndef SRC_BOGRON_GAME_HPP_
#define SRC_BOGRON_GAME_HPP_

#include <unistd.h>
#include <cstdint>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>
#include <algorithm>
#include "spawn.hpp"
#include "../lib/sound.hpp"

using namespace std::chrono;

class Player {
private:
	size_t lifes_;
	std::pair<off_t, off_t> initial_pos_;
public:
	std::pair<off_t, off_t> pos_;
	milliseconds lastMove_;
	bool hasBomb_ = false;
	bool hasNuke_ = false;
	bool hasShield_ = false;
	bool isExploding = false;

	Player(off64_t x, off64_t y, size_t lifes) : initial_pos_(x,y), pos_(x,y), lifes_(lifes) {
	}

	bool kill() {
		isExploding = true;
		return (--lifes_ <= 0);
	}

	bool isDead() const {
		return lifes_ <= 0;
	}

	void reset() {
		pos_ = initial_pos_;
		lifes_ = 5;
		hasBomb_ = false;
		hasNuke_ = false;
	}

	size_t lifes() const {
		return lifes_;
	}
};

class Game {
public:
	typedef std::vector<uint8_t> column_t;
	typedef std::deque<column_t> grid_t;
	enum Direction {
		LEFT,
		RIGHT,
		UP,
		DOWN
	};
private:
	size_t width_;
	size_t height_;
	Player player1_;
	Player player2_;

	grid_t grid_;
	std::mutex gridMtx_;
	Spawn spawn_;
	Sound snd_;
public:
	Game(size_t width, size_t height);
	virtual ~Game();
	void checkPlayer(Player& pos);
	void lock();
	void unlock();
	void step();
	void explode(Player& pActive, Player& pPassive);
	void explode1();
	void explode2();

	grid_t& grid();
	Player& player1();
	Player& player2();


	milliseconds epoch() {
		return duration_cast<milliseconds>(
					system_clock::now().time_since_epoch());
	}

	void move(Player& p, const Player& pPassive, const Direction& d);

	void move1(Direction d) {
		move(player1_, player2_, d);
	}
	void move2(Direction d) {
		move(player2_, player1_, d);
	}

	bool isOver() {
		return player1_.isDead() || player2_.isDead();
	}

	void reset();
};

#endif /* SRC_BOGRON_GAME_HPP_ */
