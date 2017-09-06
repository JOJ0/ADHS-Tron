#include <MIDI.h>
#define USBserial Serial
#define MIDIserial Serial1

const uint8_t MIDI_CH = 1;
// Ableton C-2 = C0 = 00, Ableton C2 = C0 = 24; my keyb default range: 36-72
const uint8_t LOWEST_KEY = 36; // 24=C2, 36=C3
const uint8_t HIGHEST_KEY = 72; // 84=C7, 72=C6, 60=C5, 48=C4, 
const uint8_t CC_CUTOFF = 74; // 74 usually is filter cutoff 
const uint8_t CC_LFO_RATE = 75; // 75 is not a standard CC
const uint8_t CC_PANIC = 123; // 123 is "All Notes Off" message

const uint8_t PIN_LED_INT = 13;
const uint8_t PIN_GATE = 2; // digital
const uint8_t PIN_CUTOFF = 3; // PWM, to 30000 in setup
const uint8_t PIN_PITCH = A14; // DAC, to 30000 in setup
const uint8_t PIN_SWITCH_CUTOFF_MODE = 5; // digital, velocity controls cutoff on/off switch
const uint8_t PIN_SWITCH_CLOCK = 8; // digital, metronome on/off switch
const uint8_t PIN_MIDI_SERIAL = 0; // Serial Port 1 (Pin 0) is called "Serial1" in software
const uint8_t PIN_LED_PINK = 9; // cutoff controlled by CC
const uint8_t PIN_LED_VIOLET = 7;  // cutoff controlled by velocity
const uint8_t PIN_LFO_RATE = 4; // PWM, to 30000 in setup
const uint8_t PIN_LED_READY1 = 7;
const uint8_t PIN_LED_READY2 = 9;
const uint8_t PIN_LED_CLOCK = 6; // metronome

//bool gMidiGateOn = false;
//uint8_t gMidiNoteValue = 0;
uint16_t gPitchAnalog = 0;
//uint16_t gPitchAnalog_last = 0;
//uint16_t gPitchAnalog_curr = 0;
uint8_t gNoteOnCounter = 0;
uint8_t gNoteOffCounter = 0;
bool gVelocityCutoff = false;
uint8_t gClockCount = 0;
uint8_t gBeatCount = 1;
unsigned long gMicrosOnLastBeat = 0;
float gBPM = 0;
bool gMetronome = false;

// MIDI settings struct
struct MySettings : public midi::DefaultSettings {
  static const bool UseRunningStatus = true;
};

//MIDI_CREATE_INSTANCE(HardwareSerial, MIDIserial, MIDI); // port is selectable here
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, MIDIserial, MIDI, MySettings); // altering settings

void debugNote (byte channel, byte pitch, byte velocity, uint16_t PitchAnalog) {
  float Volts = (gPitchAnalog*3.3/4096);
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
      analogWrite(PIN_CUTOFF, Velocity*0.6);
    }
    analogWriteResolution(12); // DAC to 12bit resolution
    analogWrite(PIN_PITCH, gPitchAnalog);
    digitalWrite(PIN_GATE, HIGH); // GATE on
    //
    digitalWrite(PIN_LED_INT, HIGH); // DEBUG LED on
    //debugNote(Channel, PitchMidi, Velocity, gPitchAnalog); // DEBUG
    USBserial.print("NoteOn - gNoteOnCounter: "); USBserial.print(gNoteOnCounter); // DEBUG
    USBserial.print(", gNoteOffCounter: "); USBserial.println(gNoteOffCounter); // DEBUG
    USBserial.print("gVelocityCutoff: "); USBserial.println(gVelocityCutoff); // DEBUG
    USBserial.print("gMetronome: "); USBserial.println(gMetronome); // DEBUG
  }
}

void handleNoteOff(byte Channel, byte PitchMidi, byte Velocity) { // NoteOn with 0 velo is NoteOff. 
  if (PitchMidi >= LOWEST_KEY && PitchMidi <= HIGHEST_KEY) {
    gNoteOffCounter++;
    if (gNoteOnCounter == gNoteOffCounter) {
      gNoteOnCounter = 0;
      gNoteOffCounter = 0;
      digitalWrite(PIN_LED_INT, LOW);
      digitalWrite(PIN_GATE, LOW);
      //analogWrite(PIN_PITCH, 0);
    }
  }
  //USBserial.print("NoteOff - gNoteOnCounter: "); USBserial.print(gNoteOnCounter); // DEBUG
  //USBserial.print(", gNoteOffCounter: "); USBserial.println(gNoteOffCounter); // DEBUG
}

void handleControlChange(byte inChannel, byte inNumber, byte inValue) {
  if (inNumber == CC_CUTOFF && gVelocityCutoff == false) {
    analogWriteResolution(8); // set to 8bit PWM resolution
    analogWrite(PIN_CUTOFF, inValue*0.9);
    //USBserial.print("CC_CUTOFF: "); USBserial.println(inValue); // DEBUG
  }
  if (inNumber == CC_LFO_RATE) {
    analogWriteResolution(8); // set to 8bit PWM resolution
    analogWrite(PIN_LFO_RATE, inValue*2);
    //float Volts = (inValue*2*3.3/256); // DEBUG
    //USBserial.print("CC_LFO: "); USBserial.println(inValue); // DEBUG
    //USBserial.print("Volts_LFO: "); USBserial.println(Volts); // DEBUG
  }
  if (inNumber == CC_PANIC) {
    gNoteOnCounter = 0;
    gNoteOffCounter = 0;
    digitalWrite(PIN_GATE, LOW);
    digitalWrite(PIN_LED_INT, LOW);
    USBserial.print("Panic! - All Notes Off: "); USBserial.println(CC_PANIC); // DEBUG
  }
}

void handleStart() {
  //gMicrosOnLastBeat=0;
}

void handleStop() {
  gClockCount=0;
  gBeatCount=1;
  gBPM=0;
  gNoteOnCounter = 0;
  gNoteOffCounter = 0;
  digitalWrite(PIN_GATE, LOW);
}

void handleContinue() {
}

void handleClock() {
  switch(gClockCount) {
    case 0:
      // BPM calc start
      float microsSinceLastBeat;
      microsSinceLastBeat = micros()-gMicrosOnLastBeat;
      gBPM = 60000000/microsSinceLastBeat;
      //USBserial.print("micros ON last beat: "); USBserial.println(gMicrosOnLastBeat); // DEBUG
      //USBserial.print("micros since last beat: "); USBserial.println(microsSinceLastBeat); // DEBUG
      USBserial.print("BPM: "); USBserial.println(gBPM, 4); // DEBUG
      // BPM calc end
      // metronome start
      if (gMetronome == true) {
        digitalWrite(PIN_LED_INT, HIGH); // DEBUG blink on full beat
        USBserial.print("gBeatCount: "); USBserial.println(gBeatCount); // DEBUG
        if (gBeatCount == 1) { 
          //analogWrite(PIN_LED_CLOCK, 255); // blink fully lit on first beat
          digitalWrite(PIN_LED_CLOCK, HIGH);
        }
        else {
          analogWrite(PIN_LED_CLOCK, 255); // blink damped on beats 2,3,4
        }
      }
      else {
        analogWrite(PIN_LED_CLOCK, 0); // metronome LED off
      }
      gMicrosOnLastBeat=micros();
      if (gBeatCount < 4) { gBeatCount++; }
      else { gBeatCount=1; } // beat calc
      break;
    case 2:
      digitalWrite(PIN_LED_INT, 0);
      //analogWrite(PIN_LED_CLOCK, 0);
      break;
    case 5:
      analogWrite(PIN_LED_CLOCK, 0); // metronome lit duration
      //USBserial.println("e"); // DEBUG
      break;
    case 11:
      //USBserial.println("+"); // DEBUG
      break;
    case 17:
      //USBserial.println("a"); // DEBUG
      break;
  }

  if (gClockCount < 23) {
    gClockCount++;
  }
  else {
    gClockCount=0;
  }
}

void  blinkLedWhenReady(uint8_t PIN_LED1, uint8_t PIN_LED2, uint8_t PIN_LED3){
  for (int i = 0; i<10; i++) {
    digitalWrite(PIN_LED1, HIGH);
    digitalWrite(PIN_LED2, HIGH);
    digitalWrite(PIN_LED3, LOW);
    delay(200);
    digitalWrite(PIN_LED1, LOW);
    digitalWrite(PIN_LED2, LOW);
    digitalWrite(PIN_LED3, HIGH);
    delay(200);
  }
}

void setup() {
  pinMode(PIN_LED_INT, OUTPUT); // BuiltIn LED
  pinMode(PIN_GATE, OUTPUT); // Gate Pin to digital
  pinMode(PIN_SWITCH_CUTOFF_MODE, INPUT);
  pinMode(PIN_LED_PINK, OUTPUT);
  pinMode(PIN_LED_VIOLET, OUTPUT);
  pinMode(PIN_MIDI_SERIAL, INPUT_PULLUP);
  pinMode(PIN_LED_CLOCK, OUTPUT); // metronome
  pinMode(PIN_SWITCH_CLOCK, INPUT); // metronome
  analogWriteResolution(8); // default to 8bit PWM resolution
  //analogWriteFrequency(PIN_PITCH, 30000);
  analogWriteFrequency(PIN_CUTOFF, 30000);
  analogWriteFrequency(PIN_LFO_RATE, 30000);
  digitalWrite(PIN_LED_INT, LOW); // PIN_LED_INT off initially
  USBserial.begin(115200); // debugging here
  //MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.begin(MIDI_CH);  // Listen to incoming messages on given channel
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff); 
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleContinue(handleContinue);
  MIDI.setHandleClock(handleClock);
  blinkLedWhenReady(PIN_LED_INT, PIN_LED_READY1, PIN_LED_READY2);
  //while(!USBserial); // wait until USBserial is accessible
  USBserial.println("Hyperactive Teensy ready, waiting for MIDI input...");
  //delay(1000);
} 

void loop() {
  if (digitalRead(PIN_SWITCH_CUTOFF_MODE) == HIGH) {
    gVelocityCutoff = true; // violet LED
    digitalWrite(PIN_LED_VIOLET, HIGH);
    digitalWrite(PIN_LED_PINK, LOW);
  }
  else {
    gVelocityCutoff = false; // pink LED
    digitalWrite(PIN_LED_PINK, HIGH);
    digitalWrite(PIN_LED_VIOLET, LOW);
  }
  if (digitalRead(PIN_SWITCH_CLOCK) == HIGH) {
    gMetronome = true; 
    //USBserial.println("metronome on");
  }
  else {
    gMetronome = true; // BROKEN SWITCH WORKAROUND 
    //digitalWrite(PIN_LED_CLOCK, LOW);
    //USBserial.println("metronome off");
  }
  MIDI.read(); // Read incoming messages


  //gPitchAnalog = 2047;
  //debugNote(1, 2, 3, gPitchAnalog); // DEBUG
  //digitalWrite(PIN_LED_INT, HIGH); // LED on
  ///analogWrite(PIN_PITCH, gPitchAnalog);
  //analogWrite(PIN_CUTOFF, 255);
  //delay(5000);

  //gPitchAnalog = 0;
  //debugNote(1, 2, 3, gPitchAnalog); // DEBUG
  //digitalWrite(PIN_LED_INT, LOW); // LED off
  //analogWrite(PIN_PITCH, gPitchAnalog);
  //analogWrite(PIN_CUTOFF, 0);
  //delay(5000);
}



