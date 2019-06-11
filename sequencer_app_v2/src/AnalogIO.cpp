#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

#include "Variables.h"
#include "Pinout.h"
#include "AnalogIO.h"
#include "Display.h"
#include "Sequencer.h"

namespace supersixteen {

const int analog_pins[4] = { 
	ANALOG_PIN_1,
	ANALOG_PIN_2,
	ANALOG_PIN_3,
	ANALOG_PIN_4
};

#define TEMPO_PARAM 0
#define PITCH_PARAM 1
#define OCTAVE_PARAM 2
#define DURATION_PARAM 3
#define CV_PARAM 4
const int analog_params[4] = { PITCH_PARAM, OCTAVE_PARAM, DURATION_PARAM, CV_PARAM };
int display_param = PITCH_PARAM;
int display_num = 0;
bool param_changed = false;

Sequencer* sequencerVar;

void AnalogIo::init(Sequencer& sequencer){
	pinMode(ANALOG_PIN_1, INPUT);
	pinMode(ANALOG_PIN_2, INPUT);
	pinMode(ANALOG_PIN_3, INPUT);
	pinMode(ANALOG_PIN_4, INPUT);
	sequencerVar = &sequencer;
}

void AnalogIo::poll() {
	analogMultiplexor += 1;
	if (analogMultiplexor > 3) {
		analogMultiplexor = 0;
	}
	int i = analogMultiplexor;
	// if (i > 3 || i < 0) return; //sometimes we get desynced by interrupts, and analogRead on a wrong pin is fatal
	analogValues[i] = analogRead(analog_pins[i]);
	param_changed = false;
	int change_threshold = 10;
	if (display_param == analog_params[i]) {
		change_threshold = 4; //increase sensitivity when param is selected, decrease otherwise to reduce accidental "bump" changes
	}
	if (abs(analogValues[i] - lastAnalogValues[i]) > change_threshold) {
		lastAnalogValues[i] = analogValues[i];
		switch (i) {
		case 0: setPitch(analogValues[0]); break;
		case 1: setOctave(analogValues[1]); break;
		case 2: setDuration(analogValues[2]); break;
		case 3: setCV(analogValues[3]); break;
		}
	}
}

void AnalogIo::setPitch(int analogValue) {
	display_param = PITCH_PARAM;
	//calibration_value = (float(analogValues[1]) + 1024.0) / 1500.0 ; //11.60
	int newVal = analogValue / 42.1 - 12.1; //convert from 0_1024 to 0_88 to -12_0_12
	if (sequencerVar->setPitch(newVal)) setDisplayNum(newVal);
}

void AnalogIo::setOctave(int analogValue) {
	display_param = OCTAVE_PARAM;
	int newVal = analogValue / 120 - 4; //convert from 0-1024 to -4_0_4
	if (sequencerVar->setOctave(newVal)) setDisplayNum(newVal);
}

void AnalogIo::setDuration(long analogValue) { //need extra bits for exponent operation
	display_param = DURATION_PARAM;
	int newVal = analogValue * analogValue / 2615; //convert from 0-1024 to 0-400 with exponential curve
	if (sequencerVar->setDuration(newVal)) setDisplayNum(newVal);
}

void AnalogIo::setCV(int analogValue) {
	display_param = CV_PARAM;
	int newVal = analogValue / 10.23; //convert from 0-1024 to 0-100
	if (sequencerVar->setCv(newVal)) setDisplayNum(newVal);
}

void AnalogIo::displaySelectedParam() {
	//update display to show currently selected step value if applicable
	switch (display_param) {
		//case TEMPO_PARAM: break
	case PITCH_PARAM:    setDisplayNum(sequencerVar->getPitch()); break;
	case OCTAVE_PARAM:   setDisplayNum(sequencerVar->getOctave()); break;
	case DURATION_PARAM: setDisplayNum(sequencerVar->getDuration()); break;
	case CV_PARAM:       setDisplayNum(sequencerVar->getCv()); break;
	//case CALIBRATION_PARAM: num_display = calibration_values[selected_step]; break;
	}
}

void AnalogIo::setDisplayNum(int displayNum){
	display_num = displayNum;
	param_changed = true;
}

int AnalogIo::getDisplayNum(){
	return display_num;
}

bool AnalogIo::paramChanged(){
	return param_changed;
}

};
