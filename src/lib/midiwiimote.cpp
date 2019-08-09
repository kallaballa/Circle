#include "midiwiimote.hpp"

WMEvent* MidiWiimote::ev = new WMEvent();

MidiWiimote::MidiWiimote(int32_t inport) {
	midiin->openPort(inport);
	midiin->ignoreTypes(false, false, false);
}

MidiWiimote::~MidiWiimote() {
	midiin->closePort();
}

WMEvent MidiWiimote::receive() {
	int nBytes;

	midiin->getMessage(&msg);
	nBytes = msg.size();

	if (nBytes == 10) {
		ev->ctrlNr_ = msg[1];
		ev->roll_ = msg[2];
		ev->pitch_ = msg[3];
		ev->xa_ = msg[4];
		ev->ya_ = msg[5];
		ev->za_ = msg[6];
		uint16_t buttons = (((uint16_t) msg[7])) | ((uint16_t) msg[8] << 8);
		ev->buttonMask_ = buttons;
		ev->btn_up_.update(buttons & MASK_BTN_UP);
		ev->btn_down_.update(buttons & MASK_BTN_DOWN);
		ev->btn_left_.update(buttons & MASK_BTN_LEFT);
		ev->btn_right_.update(buttons & MASK_BTN_RIGHT);
		ev->btn_a_.update(buttons & MASK_BTN_A);
		ev->btn_b_.update(buttons & MASK_BTN_B);
		ev->btn_minus_.update(buttons & MASK_BTN_MINUS);
		ev->btn_plus_.update(buttons & MASK_BTN_PLUS);
		ev->btn_home_.update(buttons & MASK_BTN_HOME);
		ev->btn_1_.update(buttons & MASK_BTN_1);
		ev->btn_2_.update(buttons & MASK_BTN_2);
	}
	return *ev;
}

std::ostream& operator<<(std::ostream &out, WMEvent& ev) {
	out << ev.ctrlNr_ << '\t' << ev.roll_ << '\t' << ev.pitch_ << '\t' << ev.xa_ << '\t' << ev.ya_ << '\t' << ev.za_ << '\t' << std::hex << ev.buttonMask_ << std::dec;
	return out;
}

