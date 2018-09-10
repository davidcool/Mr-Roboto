// ---------------------------------------------------------------------------
// Mr. Roboto
// Robot Firmware
// v5.0
// David Cool
// http://davidcool.com
//
// Project archive: https://github.com/davidcool/Mr-Roboto
//
// Firmware to control a laser cut robot using servos, pots and a 4 position toggle switch
//
// Mr. Roboto can interface with Mr. Touch Key (https://github.com/davidcool/Mr-Touch-Key) over serial
// ---------------------------------------------------------------------------

#include <Servo.h>                                                  // include the servo library for servo related functions

const byte NUMBER_OF_SERVOS = 4;                                    // set to number of servos & pots you'll be connecting
const byte NUMBER_OF_TOGGLE_POSITIONS = 4;                          // set to number of positions (throws) for switch
                                                                    // arrays are used to store values of each servo independently, make sure arrays size matches num of servos & throws
const int togglePin[NUMBER_OF_TOGGLE_POSITIONS] = {2,3,4,5};        // the digital pin number for the toggle switch (connect center pin/pole to GND, throws to pins 2, 3, 4, 5)
const int servoPin[NUMBER_OF_SERVOS] = {6,7,8,9};                   // the digital pin numbers for the servo pulse connections
const int potPin[NUMBER_OF_SERVOS] = {0,1,2,3};                     // the analog pin number for the pot signals (10K potentiometers)

const int toggleInterval[NUMBER_OF_TOGGLE_POSITIONS] = {150,150,150,150};  // number of millisecs between toggle readings
const int servoMinDegrees[NUMBER_OF_SERVOS] = {0,0,0,0};            // the limits to servo movement (0-180 nominal values)
const int servoMaxDegrees[NUMBER_OF_SERVOS] = {180,180,180,180};    // calibrate the values to your servos

Servo myservo[NUMBER_OF_SERVOS];                                    // create servo object to control a servo 

bool toggleState[NUMBER_OF_TOGGLE_POSITIONS] = {HIGH,HIGH,HIGH,HIGH};    // set default toggle pins to high. When toggle position is selected it will go LOW b/c pole is connected to GND

int serialRead[2];                                                  // serial data from keyboard containing current pitch and note on/off value

int servoInterval[NUMBER_OF_SERVOS] = {100,100,100,100};                // initial millisecs between servo reads
int servoPosition[NUMBER_OF_SERVOS] = {90,90,90,90};                // the current angle of the servo - starting at 90 degrees
int servoDegrees[NUMBER_OF_SERVOS] = {1,1,1,1};                     // amount servo moves at each step, negative value moves opposite direction

int potInterval[NUMBER_OF_SERVOS] = {100,100,100,100};                  // initial millisecs between pot reads
int potVal[NUMBER_OF_SERVOS] = {0,0,0,0};                           // stores value of pot position, zero to initialize
int potValKnob[NUMBER_OF_SERVOS] = {0,0,0,0};                       // stores value of pot position, zero to initialize
int potValSweep[NUMBER_OF_SERVOS] = {0,0,0,0};                      // stores value of pot position, zero to initialize
int potValMIDI[NUMBER_OF_SERVOS] = {0,0,0,0};

unsigned long currentMillis = 0;                                    // stores the value of millis() in each iteration of loop(), zero to initialize
unsigned long previousToggleMillis[NUMBER_OF_TOGGLE_POSITIONS] = {0,0,0,0};  // time when toggle last checked, zero to initialize
unsigned long previousPotMillis[NUMBER_OF_SERVOS] = {0,0,0,0};      // time when pot last checked, zero to initialize

unsigned long previousServoMillis[NUMBER_OF_SERVOS] = {0,0,0,0};    // the time when the servo was last checked, zero to initialize
unsigned long servoRandomMillis[NUMBER_OF_SERVOS] = {0,0,0,0};      // the random time periods between servo direction changes, zero to initialize

int led = 13;                                                       // Use pin 13 for led feedback
bool count = 0;

int pitch[] = {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};     // array holds MIDI note pitch values starting at C3

void setup() {
  
 Serial1.begin(9600);                                               // start serial
 delay(2000);                                                       // wait 2 secs to make sure serial is up and running
 randomSeed(analogRead(5));                                         // set random seed on unconnected analog pin, noise will generate new randomized seed each time program executes
 for (int i=0; i <NUMBER_OF_SERVOS; i++) {                          // iterate through each servo in array based on total servos number
   myservo[i].write(servoPosition[i]);                              // sets the initial position
   myservo[i].attach(servoPin[i]);                                  // connects each servo to proper pin
   servoRandomMillis[i] = random(4000,20000);                       // pick a random time period between min & max number
 }
 for (int i=0; i <NUMBER_OF_TOGGLE_POSITIONS; i++) {
  pinMode(togglePin[i], INPUT_PULLUP);                              // configure toggle pins as inputs and enable the internal pull-up resistors
 }                                                                  // when toggle pin is engaged the signal goes LOW b/c the pole is connected to GND
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);                                             // led is set to output and will show serial traffic

}

void loop() {

if (Serial1) {
  count = 1;
} else if ( (!Serial1) && (count == 1) ) {
  asm volatile ("  jmp 0");
}
if (Serial1.available() >= 2){
    byte b1 = Serial1.read();
    serialRead[0] = b1;
    
    byte b2 = Serial1.read();
    serialRead[1] = b2;

    serialActive();
      
}

 currentMillis = millis();                                           // capture latest value of millis(), like loggin the time of a clock
 readToggle();                                                       // capture latest toggle values
 readPots();                                                         // capture latest pot values
 
 servoKnob(0);                                                       // run manual knob control function if proper toggle position is set to high
 servoSweep(1);                                                      // run auto sweep function if proper toggle position is set to high
 //servoRandomSweep(2);                                                // run auto random sweep function if proper toggle position is set to high
 servoRandomSweepMIDI(2);                                            // each time a random direction change happens in servo, it sends a random MIDI note to Mr. Touch Key over serial
 servoMusicDance(3);                                                 // robot reacts to touch piano via serial data

}

void serialActive() {
 digitalWrite(13, HIGH);
 delay(5);
 digitalWrite(13, LOW);
 delay(5);
}

void readToggle() {

  for (int i=0; i <NUMBER_OF_TOGGLE_POSITIONS; i++) {
   if (currentMillis - previousToggleMillis[i] >= toggleInterval[i]) {
      if (digitalRead(togglePin[i]) == LOW) {
        toggleState[i] = LOW;
      } else {
        toggleState[i] = HIGH;
      } 
      previousToggleMillis[i] += toggleInterval[i];                  // record time when toggle position changed
   }
  }

}

void readPots() {

  for (int i=0; i <NUMBER_OF_SERVOS; i++) {
     if (currentMillis - previousPotMillis[i] >= potInterval[i]) {
        potVal[i] = analogRead(potPin[i]);                           // reads the value of the potentiometer (value between 0 and 1023)
        potValKnob[i] = map(potVal[i], 0, 1023, 0, 180);             // scale it to use it with the knob (position between 0-180 deg nominal)
        potValSweep[i] = map(potVal[i], 0, 1023, 5, 50);             // scale it to use it with the servo (slow to fast millisecs speed)
        potValMIDI[i] = map(potVal[i], 0, 1023, 500, 5000);
        servoInterval[i] = potValSweep[i];                           // set millisec speed for servo interval based on pot value
        previousPotMillis[i] += potInterval[i];                      // record time when pot changed value
     }
   }

}

void servoKnob(int toggleNum) {

  if (toggleState[toggleNum] == LOW) {
    for (int i=0; i <NUMBER_OF_SERVOS; i++) {
      myservo[i].write(potValKnob[i]);                                // sets the servo position according to the scaled value
    }
  }

}

void servoSweep(int toggleNum) {

  if (toggleState[toggleNum] == LOW) {
    for (int i=0; i <NUMBER_OF_SERVOS; i++) {
      if (currentMillis - previousServoMillis[i] >= servoInterval[i]) {     
       servoPosition[i] = servoPosition[i] + servoDegrees[i];         // servoDegrees might be negative
       if ((servoPosition[i] >= servoMaxDegrees[i]) || (servoPosition[i] <= servoMinDegrees[i]))  { // if the servo is at either extreme change the sign of the degrees to make it move the other way           
         servoDegrees[i] = - servoDegrees[i];                         // reverse direction
         servoPosition[i] = servoPosition[i] + servoDegrees[i];       // and update the position to ensure it is within range
       }      
       myservo[i].write(servoPosition[i]);                            // make the servo move to the next position
       previousServoMillis[i] += servoInterval[i];                    // and record the time when the move happened
      } 
    }
  }

}

void servoRandomSweep(int toggleNum) {

  if (toggleState[toggleNum] == LOW) {
    for (int i=0; i <NUMBER_OF_SERVOS; i++) {
      if (currentMillis - previousServoMillis[i] >= servoInterval[i]) {
       if (currentMillis > servoRandomMillis[i]) {                    // is it time to trigger a random direction change of the servo?
        servoDegrees[i] = - servoDegrees[i];                          // reverse direction
        servoRandomMillis[i] = random(500,10000) + currentMillis;       // pick new random time and add to current time for future trigger     
       }
       servoPosition[i] = servoPosition[i] + servoDegrees[i];         // servoDegrees might be negative  
       if ((servoPosition[i] >= servoMaxDegrees[i]) || (servoPosition[i] <= servoMinDegrees[i]))  { // if the servo is at either extreme change the sign of the degrees to make it move the other way           
         servoDegrees[i] = - servoDegrees[i];                         // reverse direction
         servoPosition[i] = servoPosition[i] + servoDegrees[i];       // and update the position to ensure it is within range
       }      
       myservo[i].write(servoPosition[i]);                            // make the servo move to the next position
       previousServoMillis[i] += servoInterval[i];                    // and record the time when the move happened
      } 
    }
  }

}


void servoRandomSweepMIDI(int toggleNum) {

  if (toggleState[toggleNum] == LOW) {
    for (int i=0; i <NUMBER_OF_SERVOS; i++) {
      if (currentMillis - previousServoMillis[i] >= servoInterval[i]) {
       if (currentMillis > servoRandomMillis[i]) {                    // is it time to trigger a random direction change of the servo?
        servoDegrees[i] = - servoDegrees[i];                          // reverse direction
        servoRandomMillis[i] = potValMIDI[i] + currentMillis;       // pick new random time and add to current time for future trigger

        sendSerialMIDI();
             
       }
       servoPosition[i] = servoPosition[i] + servoDegrees[i];         // servoDegrees might be negative  
       if ((servoPosition[i] >= servoMaxDegrees[i]) || (servoPosition[i] <= servoMinDegrees[i]))  { // if the servo is at either extreme change the sign of the degrees to make it move the other way           
         servoDegrees[i] = - servoDegrees[i];                         // reverse direction
         servoPosition[i] = servoPosition[i] + servoDegrees[i];       // and update the position to ensure it is within range
       }      
       myservo[i].write(servoPosition[i]);                            // make the servo move to the next position
       previousServoMillis[i] += servoInterval[i];                    // and record the time when the move happened
      } 
    }
  }

}

void sendSerialMIDI() {

        // send "on" signal as "1"  to the serial port
        Serial1.write(1 % 256);

        int ranNote = pitch[random(sizeof(pitch))];
        // send note value to the serial port
        Serial1.write(ranNote % 256); 
        delay(5); //allows all serial sent to be received together

        serialActive();
        
}

void servoMusicDance(int toggleNum){

  // MIDI note mapping
  // Octave   Note Numbers
  //          C   C#  D   D#  E   F   F#  G   G#  A   A#  B
  //    0     0   1   2   3   4   5   6   7   8   9   10  11
  //    1     12  13  14  15  16  17  18  19  20  21  22  23
  //    2     24  25  26  27  28  29  30  31  32  33  34  35
  //    3     36  37  38  39  40  41  42  43  44  45  46  47
  //    4     48  49  50  51  52  53  54  55  56  57  58  59
  //    5     60  61  62  63  64  65  66  67  68  69  70  71
  //    6     72  73  74  75  76  77  78  79  80  81  82  83
  //    7     84  85  86  87  88  89  90  91  92  93  94  95
  //    8     96  97  98  99  100 101 102 103 104 105 106 107
  //    9     108 109 110 111 112 113 114 115 116 117 118 119
  //    10    120 121 122 123 124 125 126 127

  // MIDI keyboard set to C3 by default
    
  if (toggleState[toggleNum] == LOW) {
    for (int i=0; i <NUMBER_OF_SERVOS; i++) {
      if (currentMillis - previousServoMillis[i] >= servoInterval[i]) {
        if (serialRead[0] == 1) {

          //**** check for pitch 36, 37, 38 and if it's the first servo, make it move
          if ( (serialRead[1] == 36 || serialRead[1] == 37 || serialRead[1] == 38) && (i == 0) ) {
            if (currentMillis > servoRandomMillis[i]) {                    // is it time to trigger a random direction change of the servo?
            servoDegrees[i] = - servoDegrees[i];                          // reverse direction
            servoRandomMillis[i] = random(500,10000) + currentMillis;       // pick new random time and add to current time for future trigger     
           }
           
           servoPosition[i] = servoPosition[i] + servoDegrees[i];         // servoDegrees might be negative  
           if ((servoPosition[i] >= servoMaxDegrees[i]) || (servoPosition[i] <= servoMinDegrees[i]))  { // if the servo is at either extreme change the sign of the degrees to make it move the other way           
             servoDegrees[i] = - servoDegrees[i];                         // reverse direction
             servoPosition[i] = servoPosition[i] + servoDegrees[i];       // and update the position to ensure it is within range
           }
                 
           myservo[i].write(servoPosition[i]);                            // make the servo move to the next position
           previousServoMillis[i] += servoInterval[i];                    // and record the time when the move happened
          }

          //**** check for pitch 39, 40, 41 and if it's the second servo, make it move
          if ( (serialRead[1] == 39 || serialRead[1] == 40 || serialRead[1] == 41) && (i == 1) ) {
            if (currentMillis > servoRandomMillis[i]) {                    // is it time to trigger a random direction change of the servo?
            servoDegrees[i] = - servoDegrees[i];                          // reverse direction
            servoRandomMillis[i] = random(500,10000) + currentMillis;       // pick new random time and add to current time for future trigger     
           }
           
           servoPosition[i] = servoPosition[i] + servoDegrees[i];         // servoDegrees might be negative  
           if ((servoPosition[i] >= servoMaxDegrees[i]) || (servoPosition[i] <= servoMinDegrees[i]))  { // if the servo is at either extreme change the sign of the degrees to make it move the other way           
             servoDegrees[i] = - servoDegrees[i];                         // reverse direction
             servoPosition[i] = servoPosition[i] + servoDegrees[i];       // and update the position to ensure it is within range
           }
                 
           myservo[i].write(servoPosition[i]);                            // make the servo move to the next position
           previousServoMillis[i] += servoInterval[i];                    // and record the time when the move happened
          }

          //**** check for pitch 42, 43, 44 and if it's the third servo, make it move
          if ( (serialRead[1] == 42 || serialRead[1] == 43 || serialRead[1] == 44) && (i == 2) ) {
            if (currentMillis > servoRandomMillis[i]) {                    // is it time to trigger a random direction change of the servo?
            servoDegrees[i] = - servoDegrees[i];                          // reverse direction
            servoRandomMillis[i] = random(500,10000) + currentMillis;       // pick new random time and add to current time for future trigger     
           }
           
           servoPosition[i] = servoPosition[i] + servoDegrees[i];         // servoDegrees might be negative  
           if ((servoPosition[i] >= servoMaxDegrees[i]) || (servoPosition[i] <= servoMinDegrees[i]))  { // if the servo is at either extreme change the sign of the degrees to make it move the other way           
             servoDegrees[i] = - servoDegrees[i];                         // reverse direction
             servoPosition[i] = servoPosition[i] + servoDegrees[i];       // and update the position to ensure it is within range
           }
                 
           myservo[i].write(servoPosition[i]);                            // make the servo move to the next position
           previousServoMillis[i] += servoInterval[i];                    // and record the time when the move happened
          }
          
          //**** check for pitch 45, 46, 47 and if it's the fourth servo, make it move
          if ( (serialRead[1] == 45 || serialRead[1] == 46 || serialRead[1] == 47) && (i == 3) ) {
            if (currentMillis > servoRandomMillis[i]) {                    // is it time to trigger a random direction change of the servo?
            servoDegrees[i] = - servoDegrees[i];                          // reverse direction
            servoRandomMillis[i] = random(500,10000) + currentMillis;       // pick new random time and add to current time for future trigger     
           }
           
           servoPosition[i] = servoPosition[i] + servoDegrees[i];         // servoDegrees might be negative  
           if ((servoPosition[i] >= servoMaxDegrees[i]) || (servoPosition[i] <= servoMinDegrees[i]))  { // if the servo is at either extreme change the sign of the degrees to make it move the other way           
             servoDegrees[i] = - servoDegrees[i];                         // reverse direction
             servoPosition[i] = servoPosition[i] + servoDegrees[i];       // and update the position to ensure it is within range
           }
                 
           myservo[i].write(servoPosition[i]);                            // make the servo move to the next position
           previousServoMillis[i] += servoInterval[i];                    // and record the time when the move happened
          }
          
        }
      }
    }
  } 
   
}
