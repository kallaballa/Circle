#include "button.hpp"

std::ostream& operator<<(std::ostream& out, const Button& btn) {
	out << btn.press_ << '\t' << btn.release_;
	return out;
}
