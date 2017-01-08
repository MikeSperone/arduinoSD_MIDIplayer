#include <ArduinoStream.h>
#include <bufstream.h>
#include <ios.h>
#include <iostream.h>
#include <istream.h>
#include <MinimumSerial.h>
#include <ostream.h>
#include <Sd2Card.h>
#include <SdBaseFile.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <SdFatmainpage.h>
#include <SdFatStructs.h>
#include <SdFatUtil.h>
#include <SdFile.h>
#include <SdInfo.h>
#include <SdSpi.h>
#include <SdStream.h>
#include <SdVolume.h>

#include <MIDIFile.h>
#include <MIDIHelper.h>


boolean debug =0;

#define numOutputs 8

byte incomingByte;
byte note;
byte velocity;

byte switchPin[8]={10,11,12,13,A0,A1,A2,A3};


boolean switchState[8];
boolean lastSwitchState[8];   // the previous reading from the input pin

long lastDebounceTime[9];  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

int chan = 1;
int action=2; //0 =note off ; 1=note on ; 2= nada
int numOuts = numOutputs; //how many outputs
#define firstPin 2 //what's the first pin
int lowNote = 36; //what's the first note?

long noteStart[numOutputs+firstPin];
int timeout = 500;
int pwmTimeout = timeout;
boolean noteOn[numOutputs+firstPin];


//setup: declaring iputs and outputs and begin serial
void setup() {
  
   for(int i=0; i<5; i++){
     pinMode(switchPin[i], INPUT);
   }
  
  //pinMode(statusLed,OUTPUT);   // declare the LED's pin as output
  pinMode(0,INPUT);   // rx pin input
  //  /*
  for (int i=firstPin; i<firstPin+numOuts; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  
  //start serial with midi baudrate 31250 or 38400 for debugging
  
  if(debug){
    Serial.begin(38400);   
  } else {
    Serial.begin(31250);    
  }
  //digitalWrite(statusLed,HIGH);  
}

//loop: wait for serial data, and interpret the message
void loop () {

  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    switch (incomingByte) {
      case 144: //chan+143:  // note on
        action = 1;
        break;
      case 128: //chan+127:  // note off
        action = 0;
        break;
      case 208: //chan+207:  // aftertouch
        //TODO: something
        break;
      case 160: //chan+159:  // polypressure
        //TODO: something
        break;
      default:
        if (action==0 && note==0 ){ // if we received a "note off", we wait for which note (databyte)
          note=incomingByte;
          playNote(note, 0);
          note=0;
          velocity=0;
          action=2;
        } else if (action ==1) {
          if (note == 0) { // if we received a "note on", we wait for the note (databyte)
            note = incomingByte;
          } else if (note != 0) { // ...and then the velocity
            velocity = incomingByte;
            playNote(note, velocity);
            note = 0;
            velocity = 0;
            action = 0;
          }
        }
        break;

    }
    //check for timeout and pwm updates for all pins
    for(int i=firstPin; i<numOutputs+firstPin; i++) { 
      notesTimeout(i);
    }

  }
}
//end LOOP


void playNote(byte note, byte velocity){

  int value=LOW;
  value = (velocity > 10) ? HIGH : LOW;

  //since we don't want to "play" all notes we wait for a note in range
  if (note>=lowNote && note<lowNote+numOuts){
    int myPin=note-(lowNote-firstPin); // to get a pinnumber within note range
    digitalWrite(myPin, value);
   
    //check to see if the drawer is open
    if (value==HIGH && switchState[myPin-2]) {
      noteStart[myPin] = millis();
      noteOn[myPin] = true;
      if (velocity<80){
        //setPWM(myPin, velocity);
        //pwm[myPin] = true;
        //pwmStart[myPin]=millis();
        //pwmStart[myPin]=micros();
        //pwmHigh[myPin]=true;
        digitalWrite(myPin, HIGH);
      } else {
        //pwm[myPin] = false;
        digitalWrite(myPin, HIGH);
      }
    }
    if (value==LOW){
      noteOn[myPin] = false;
      digitalWrite(myPin, LOW);
//________
      //digitalWrite(myPin+2, LOW);   //debug
//________
    }
  }
}

void notesTimeout(int _pin){
  if (noteOn[_pin]) {
    
    if ((millis() - noteStart[_pin] > timeout) || (millis() - noteStart[_pin] > pwmTimeout)) {
      digitalWrite(_pin, LOW);
      noteOn[_pin] == false;
      //pwm[_pin] = false;
    }
  }

}
