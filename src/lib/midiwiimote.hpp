#ifndef SRC_SLIDE_MIDI_HPP_
#define SRC_SLIDE_MIDI_HPP_

#include <rtmidi/RtMidi.h>
#include "button.hpp"
#include <iostream>

struct WMEvent {
	int ctrlNr_ = 0;
	double roll_ = 0;
	double pitch_ = 0;
	double xa_ = 0;
	double ya_ = 0;
	double za_ = 0;
	Button btn_up_;
	Button btn_down_;
	Button btn_left_;
	Button btn_right_;
	Button btn_a_;
	Button btn_b_;
	Button btn_minus_;
	Button btn_plus_;
	Button btn_home_;
	Button btn_1_;
	Button btn_2_;
	uint16_t buttonMask_;
};


std::ostream& operator<<(std::ostream &out, WMEvent& ev);
class MidiWiimote {
private:
	RtMidiIn *midiin = new RtMidiIn();
	std::vector<uint8_t> msg;
	static WMEvent* ev;
public:
	MidiWiimote(int32_t inport);
	virtual ~MidiWiimote();
	WMEvent receive();
};

#endif /* SRC_SLIDE_MIDI_HPP_ */
