#include <SoftwareSerial.h>
#include <MIDI.h>
#define USBserial Serial
SoftwareSerial MIDIserial(2, 4); // RX, TX

int LedInt = 13;
int PinGate = 6; // digital
int PinPitch = 9; // PWM
int PinCutoff = 10; // PWM??

// settings struct
struct MySettings : public midi::DefaultSettings {
  static const bool UseRunningStatus = true;
};

//MIDI_CREATE_DEFAULT_INSTANCE(); // binds to default hardware port
MIDI_CREATE_INSTANCE(SoftwareSerial, MIDIserial, MIDI); // port is selectable here
//MIDI_CREATE_CUSTOM_INSTANCE(SoftwareSerial, MIDIserial, MIDI, MySettings); // + settings

/*void debugMIDI () {
  USBserial.print(statusByte);
  USBserial.print(" ");
  USBserial.print(dataByte1);
  USBserial.print(" ");
  USBserial.print(dataByte2);
  USBserial.println(" ");
}*/

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  digitalWrite(LedInt, HIGH);
  digitalWrite(PinGate, HIGH);
  //analogWrite(PinGate, 169);
  //debugMIDI();
}

void handleNoteOff(byte channel, byte pitch, byte velocity) { // NoteOn messages with 0 velocity are interpreted as NoteOffs. 
  digitalWrite(LedInt, LOW);
  digitalWrite(PinGate, LOW);
  //analogWrite(PinGate, 0);
  //debugMIDI(); 
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



