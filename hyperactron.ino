#include <MIDI.h>
#define USBserial Serial
#define MIDIserial Serial1

// Ableton C-2 = C0 = 00, Ableton C2 = C0 = 24; my keyb default range: 36-72
const uint8_t LOWEST_KEY = 36; // 24=C2, 36=C3
const uint8_t HIGHEST_KEY = 72; // 84=C7, 72=C6, 60=C5, 48=C4, 

int LedInt = 13;
int PinGate = 2; // digital
int PinCutoff = 3; // PWM, to 30000 in setup
int PinPitch = A14; // DAC, to 30000 in setup
int PinVeloCutoffSwitch = 5; // digital, velocity controls cutoff on/off switch
int PinLedViolet = 7;

//bool gMidiGateOn = false;
//uint8_t gMidiNoteValue = 0;
uint16_t gPitchAnalog = 0;
//uint16_t gPitchAnalog_last = 0;
//uint16_t gPitchAnalog_curr = 0;
uint8_t gNoteOnCounter = 0;
uint8_t gNoteOffCounter = 0;
bool gVelocityCutoff = false;

// MIDI settings struct
struct MySettings : public midi::DefaultSettings {
  static const bool UseRunningStatus = true;
};

MIDI_CREATE_INSTANCE(HardwareSerial, MIDIserial, MIDI); // port is selectable here
//MIDI_CREATE_CUSTOM_INSTANCE(SoftwareSerial, MIDIserial, MIDI, MySettings); // altering settings

void debugNote (byte channel, byte pitch, byte velocity, uint16_t PitchAnalog) {
  float Volts = (gPitchAnalog*3.3/4095);
  USBserial.print(channel);
  USBserial.print(" ");
  USBserial.print(pitch);
  USBserial.print(" ");
  USBserial.print(velocity);
  USBserial.print(" ");
  USBserial.print(PitchAnalog);
  USBserial.print(" ");
  USBserial.print(Volts);
  USBserial.println(" ");
}

void handleNoteOn(byte Channel, byte PitchMidi, byte Velocity) {
  if (PitchMidi >= LOWEST_KEY && PitchMidi <= HIGHEST_KEY) {
    gNoteOnCounter++;
    //gPitchAnalog_last = gPitchAnalog_curr;
    // nostromo teensy + dac mcp4822
    //gPitchAnalog = uint16_t((gMidiNoteValue-LOWEST_KEY)*835.666666666); // + gMidiPitchBend ;  // 8191/12
    // arduino uno (Konstante? 0.08333333 / 5 = 42.49999983 * 255 )
    //gPitchAnalog = uint16_t((PitchMidi-LOWEST_KEY)*0.083333333/5*255);// 0.0833333=1/12V
    // teensy 3.2
    //gPitchAnalog = uint16_t((PitchMidi-LOWEST_KEY)*0.083333333/3.3*4095); // 0.055=1st if 3.3V=5oct
    gPitchAnalog = uint16_t((PitchMidi-LOWEST_KEY)*0.02577777/3.3*4096); // 0.002666 WORKING, FIXME why?
    //gPitchAnalog = uint16_t((PitchMidi-LOWEST_KEY)0.0277777778/3.3*4095*);
    // mpasserini formula test:  unsigned int in_pitch = dac_max / notes_max * (inNote - notes_lowest);
                               //note_stack.push( in_pitch );
    //gPitchAnalog = uint16_t(4095 / HIGHEST_KEY * (PitchMidi - LOWEST_KEY));
    if (gVelocityCutoff == true) {
      analogWriteResolution(8); // set to 8bit PWM resolution
      analogWrite(PinCutoff, Velocity*2);
    }
    analogWriteResolution(12); // DAC to 12bit resolution
    analogWrite(PinPitch, gPitchAnalog);
    digitalWrite(LedInt, HIGH); // LED on
    digitalWrite(PinGate, HIGH); // GATE on
    debugNote(Channel, PitchMidi, Velocity, gPitchAnalog); // DEBUG
    USBserial.print("NoteOn - gNoteOnCounter: "); USBserial.print(gNoteOnCounter); // DEBUG
    USBserial.print(", gNoteOffCounter: "); USBserial.println(gNoteOffCounter); // DEBUG
    USBserial.print("gVelocityCutoff: "); USBserial.println(gVelocityCutoff); // DEBUG
  }
}

void handleNoteOff(byte Channel, byte PitchMidi, byte Velocity) { // NoteOn with 0 velo is NoteOff. 
  if (PitchMidi >= LOWEST_KEY && PitchMidi <= HIGHEST_KEY) {
    gNoteOffCounter++;
    if (gNoteOnCounter == gNoteOffCounter) {
      gNoteOnCounter = 0;
      gNoteOffCounter = 0;
      digitalWrite(LedInt, LOW);
      digitalWrite(PinGate, LOW);
      //analogWrite(PinPitch, 0);
    }
  }
  USBserial.print("NoteOff - gNoteOnCounter: "); USBserial.print(gNoteOnCounter); // DEBUG
  USBserial.print(", gNoteOffCounter: "); USBserial.println(gNoteOffCounter); // DEBUG
}

void setup() {
  pinMode(LedInt, OUTPUT); // BuiltIn LED
  pinMode(PinGate, OUTPUT); // Gate Pin to digital
  pinMode(PinVeloCutoffSwitch, INPUT);
  pinMode(PinLedViolet, OUTPUT);
  analogWriteResolution(8); // default to 8bit PWM resolution
  //analogWriteFrequency(PinPitch, 30000);
  analogWriteFrequency(PinCutoff, 30000);
  digitalWrite(LedInt, LOW); // LedInt off initially
  //TCCR1B = (TCCR1B & 0b11111000) | 0x01; // timer 1 (pin 9,10) to 31372.55 Hz
  USBserial.begin(115200); // debugging here
  //MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.begin(1);  // Listen to incoming messages on ch 1
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff); 
} 

void loop() {
  if (digitalRead(PinVeloCutoffSwitch) == HIGH) {
    gVelocityCutoff = false; // pink LED + R used as pullup
  }
  else {
    gVelocityCutoff = true; // violet LED, on separate output pin
    digitalWrite(PinLedViolet, HIGH);
  }
  MIDI.read(); // Read incoming messages

  //gPitchAnalog = 2047;
  //debugNote(1, 2, 3, gPitchAnalog); // DEBUG
  //digitalWrite(LedInt, HIGH); // LED on
  ///analogWrite(PinPitch, gPitchAnalog);
  //analogWrite(PinCutoff, 255);
  //delay(5000);

  //gPitchAnalog = 0;
  //debugNote(1, 2, 3, gPitchAnalog); // DEBUG
  //digitalWrite(LedInt, LOW); // LED off
  //analogWrite(PinPitch, gPitchAnalog);
  //analogWrite(PinCutoff, 0);
  //delay(5000);
}



