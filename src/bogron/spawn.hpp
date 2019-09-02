#ifndef SRC_BOGRON_SPAWN_HPP_
#define SRC_BOGRON_SPAWN_HPP_

#include <random>
#include "object.hpp"

class Spawn {
private:
  std::random_device rd_;
  std::mt19937 mt_;
  std::uniform_real_distribution<float> dist_;
public:
	static float MINE_;
	static float BOMB_;
	static float NUKE_;
	static float SHIELD_;
	static float BLANK_;

	Spawn(): rd_(), mt_(rd_()), dist_(0, MINE_ + BOMB_ + SHIELD_ + NUKE_ + BLANK_) {
	}

	Object roulette() {
		float r = dist_(mt_);
		if(r < NUKE_) {
			return Object::NUKE_;
		} else if(r < BOMB_) {
			return Object::BOMB_;
		} else if(r < SHIELD_) {
			return Object::SHIELD_;
		} else if(r < MINE_) {
			return Object::MINE_;
		} if(r < BLANK_) {
			return Object::BLANK_;
		}
		return Object::BLANK_;
	}
};

#endif /* SRC_BOGRON_SPAWN_HPP_ */
