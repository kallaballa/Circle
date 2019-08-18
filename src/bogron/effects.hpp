#ifndef SRC_BOGRON_EFFECTS_HPP_
#define SRC_BOGRON_EFFECTS_HPP_

#include <vector>
#include <string>

struct Effects {
	enum Index {
		BLAST_ = 0, EXPLOSION_ = 1
	};

	static std::vector<std::string> filenames;

};


#endif /* SRC_BOGRON_EFFECTS_HPP_ */
