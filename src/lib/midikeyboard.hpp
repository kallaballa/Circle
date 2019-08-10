#ifndef SRC_SLIDE_MIDI_HPP_
#define SRC_SLIDE_MIDI_HPP_

#include <rtmidi/RtMidi.h>
#include <iostream>

struct NoteEvent {
	bool on_ = false;
	int note_ = 0;
	int velocity_ = 0;
};

std::ostream& operator<<(std::ostream &out, NoteEvent& ev);
class MidiKeyboard {
private:
	RtMidiIn *midiin_ = new RtMidiIn();
	std::vector<uint8_t> msg_;
	static NoteEvent* ev_;
public:
	MidiKeyboard(int32_t inport);
	virtual ~MidiKeyboard();
	NoteEvent receive();
};

#endif /* SRC_SLIDE_MIDI_HPP_ */
