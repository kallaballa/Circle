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

using namespace std::chrono;

struct Player {
	std::pair<off_t, off_t> pos_;
	size_t lifes_;
	milliseconds lastMove_;

	Player(off64_t x, off64_t y, size_t lifes) : pos_(x,y), lifes_(lifes) {
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
	long speed_ = 1000000;

public:
	Game(size_t width, size_t height);
	virtual ~Game();
	void lock();
	void unlock();
	void step();
	grid_t grid();
	Player player1();
	Player player2();

	milliseconds epoch() {
		return duration_cast<milliseconds>(
					system_clock::now().time_since_epoch());
	}

	void move(Player& p, const Direction& d) {
		auto e = epoch();
//		if(e.count() - 100 > player1_.lastMove_.count()) {
			switch(d) {
			case LEFT:
				p.pos_.first = std::max((off_t)0, player1_.pos_.first - 1);
				break;
			case RIGHT:
				p.pos_.first = std::min((off_t)width_, player1_.pos_.first + 1);
				break;
			case UP:
				p.pos_.second = std::max((off_t)0, player1_.pos_.second - 1);
				break;
			case DOWN:
				p.pos_.second = std::min((off_t)height_, player1_.pos_.second + 1);
				break;
			}
			p.lastMove_ = e;
//		}
	}

	void move1(Direction d) {
		move(player1_, d);
	}
	void move2(Direction d) {
		move(player2_, d);
	}

	void setSpeed(long millis) {
		speed_ = millis * 1000;
	}

	long getSpeed() {
		return speed_ / 1000;
	}
};

#endif /* SRC_BOGRON_GAME_HPP_ */
