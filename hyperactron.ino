#include <SoftwareSerial.h>
#include <MIDI.h>
#define USBserial Serial
SoftwareSerial MIDIserial(2, 4); // RX, TX

int LedInt = 13;
int PinGate = 6; // digital
int PinPitch = 9; // PWM
int PinCutoff = 10; // PWM??
bool gMidiGateOn = false;
uint8_t gMidiNoteValue = 0;

const uint8_t LOWEST_KEY = 24; // C2

// settings struct
struct MySettings : public midi::DefaultSettings {
  static const bool UseRunningStatus = true;
};

//MIDI_CREATE_DEFAULT_INSTANCE(); // binds to default hardware port
MIDI_CREATE_INSTANCE(SoftwareSerial, MIDIserial, MIDI); // port is selectable here
//MIDI_CREATE_CUSTOM_INSTANCE(SoftwareSerial, MIDIserial, MIDI, MySettings); // altering settings

void debugNote (byte channel, byte pitch, byte velocity) {
  USBserial.print(channel);
  USBserial.print(" ");
  USBserial.print(pitch);
  USBserial.print(" ");
  USBserial.print(velocity);
  USBserial.println(" ");
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (note >= LOWEST_KEY) {
    gMidiGateOn = true;
    gMidiNoteValue = note;
  }
  digitalWrite(LedInt, HIGH);
  digitalWrite(PinGate, HIGH);
  //analogWrite(PinGate, 169);
  debugNote(channel, pitch, velocity);
}

void handleNoteOff(byte channel, byte pitch, byte velocity) { // NoteOn messages with 0 velocity are interpreted as NoteOffs. 
  digitalWrite(LedInt, LOW);
  digitalWrite(PinGate, LOW);
  //analogWrite(PinGate, 0);
  //debugNote(channel, pitch, velocity);
}

void setup() {
  pinMode(LedInt, OUTPUT); // BuiltIn LED
  digitalWrite(LedInt, LOW); // LedInt off
  USBserial.begin(115200); // debugging here
  //MIDIserial.begin(31250);
  //MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.begin(1);  // Listen to incoming messages on ch 1
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
}

void loop() {
  MIDI.read(); // Read incoming messages
}



