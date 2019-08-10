#include "midikeyboard.hpp"

NoteEvent* MidiKeyboard::ev_ = new NoteEvent();

MidiKeyboard::MidiKeyboard(int32_t inport) {
	midiin_->openPort(inport);
	midiin_->ignoreTypes(true, true, true);
}

MidiKeyboard::~MidiKeyboard() {
	midiin_->closePort();
}

NoteEvent MidiKeyboard::receive() {
	int nBytes;

	midiin_->getMessage(&msg_);
	nBytes = msg_.size();
	std::cerr << msg_.size() << std::endl;
	if (nBytes == 3) {
		ev_->on_ = msg_[0];
		ev_->note_ = msg_[1];
		ev_->velocity_ = msg_[2];
	}
	return *ev_;
}

std::ostream& operator<<(std::ostream &out, NoteEvent& ev) {
	out << ev.on_ << '\t' << ev.note_ << '\t' << ev.velocity_ ;
	return out;
}

