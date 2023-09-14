// How to attach EMG: https://electropeak.com/learn/interfacing-emg-muscular-signal-sensor-with-arduino/

// Choose whether to execute via Python GUI or Arduino IDE
bool pyGUI = true;
bool arIDE = false;

// Set max time for 2 contractions
int maxT = 800;

// Libraries
#include <HardwareSerial.h>
#include <Servo.h>

// Define variables
int emgValue = 0;
int emgFilt = 0;
int threshold = map(100,0,1023,0,1023);
byte state = 1;
byte nContractions = 2;
unsigned long contractionStart;
int plotCount = 0;

// Filter settings
float EMA_a_low = 0.01;
float EMA_a_high = 0.7;
int EMA_S_low = 0;
int EMA_S_high = 0;
int highpass = 0;
int bandpass = 0;
int applyFilter = 0;

// LEDs (alternative to move servos because of currently unsolved noise problem)
int pinOpen = 39;
int pinClosed = 43;
int pinPronation = 47;
int pinSupination = 51;

// Define servos
Servo servoThumb;     
Servo servoIndex;
Servo servoMiddle;
Servo servoRing;
Servo servoPinky;
Servo servoWrist;

// Pins on Arduino
void setup() {
  servoThumb.attach(2);
  servoIndex.attach(4);
  servoMiddle.attach(6);
  servoRing.attach(8);
  servoPinky.attach(10);
  servoWrist.attach(12);
  pinMode(pinOpen,OUTPUT);
  pinMode(pinClosed,OUTPUT);
  pinMode(pinPronation,OUTPUT);
  pinMode(pinSupination,OUTPUT);
  Serial.begin(9600);
  delay(100);
  EMA_S_low = analogRead(0); //set EMA S for t=1
  EMA_S_high = analogRead(0);
}

void loop() {
  if (pyGUI){
    threshold = read(threshold);
    emgValue = analogRead(A0);
    emgFilt = filt(emgValue);
  } 
  if (arIDE){
    emgValue = analogRead(A0);
    emgFilt = filt(emgValue);
    plot();
  }
  //plot();
  if (emgFilt > threshold){
    contractionStart = millis(); // get time
    nContractions = 0;
    while (millis() < (contractionStart + maxT)){
      if (pyGUI){
        threshold = read(threshold);
        emgValue = analogRead(A0);
        emgFilt = filt(emgValue);
      } 
      if (arIDE){
        emgValue = analogRead(A0);
        emgFilt = filt(emgValue);
        plot();
      }
      if (emgFilt < threshold){
        nContractions = 1;
        switchState(state, nContractions);
      }
      if (emgFilt > threshold && nContractions == 1){
        nContractions = 2;
        switchState(state, nContractions);
        delay((contractionStart + maxT) - millis());
        break;
      }
    }
    if (nContractions == 0){
      while (emgFilt > threshold){
        if (pyGUI){
          threshold = read(threshold);
          emgValue = analogRead(A0);
          emgFilt = filt(emgValue);
        } 
        if (arIDE){
          emgValue = analogRead(A0);
          emgFilt = filt(emgValue);
          plot();
        }
        if (millis() > (contractionStart + 2000)){
          if (state == 1){
            state = 2;        
            break;
          } else if (state == 2) {
            state = 1;
            break;
          }
        }
      }
    }
  }
}

void switchState(int state, int nContractions){
  if (state == 1 && nContractions == 1){
    handClosed();
  } else if (state == 1 && nContractions == 2){
    handOpen();
  } else if (state == 2 && nContractions == 1){
    pronation();
  } else if (state == 2 && nContractions == 2){
    supination();
  }
}

int read(int threshold){
  char endMarker = '>';
  if (Serial.available() > 0){ 
    emgValue = analogRead(A0);
    emgFilt = filt(emgValue);
    char userInput = Serial.read(); // read user input
      if (userInput == 'r'){
        emgValue = analogRead(A0);
        emgFilt = filt(emgValue);
        if (pyGUI){
          Serial.println(emgFilt);     
        }
      }
      else if (userInput == 't'){
        String arduinoIn = "t";
        delay(1);
        while (Serial.available() > 0){
          String c = Serial.readString();
          if (c != endMarker){
            arduinoIn += c;
          } else {
            break;
          }
        }
        // I don't know why the f the asterisk in 't*' is required ... it doesn't make sense ... and it took me hours to figure out what's wrong -.-
        if (arduinoIn.substring(0) == 't*'){
          threshold = arduinoIn.substring(1,4).toInt();
          if (pyGUI){
            Serial.println(threshold);
          }
        }
      }
  }
  return threshold;
}

int filt(int emgValue){
  EMA_S_low = (EMA_a_low*emgValue) + ((1-EMA_a_low)*EMA_S_low);  //run the EMA
  EMA_S_high = (EMA_a_high*emgValue) + ((1-EMA_a_high)*EMA_S_high);

  highpass = emgValue - EMA_S_low; //find the high-pass as before (for comparison)
  bandpass = EMA_S_high - EMA_S_low; //find the band-pass

  applyFilter = emgValue;  // Choose which filter will be applied. Return 'emgValue' to not apply any filter.
  return applyFilter;
}

void plot(){
  plotCount++;
  if (plotCount >= 500){
    if (pyGUI){
      Serial.println(applyFilter);
      Serial.print(",");
      Serial.println(threshold);
    } else if (arIDE){
      Serial.print(emgValue);
      Serial.print(" ");
      Serial.print(EMA_S_low);
      Serial.print(" ");
      Serial.print(highpass);
      Serial.print(" ");
      Serial.print(bandpass);
      Serial.print(" ");
      Serial.println(threshold);
    }
    plotCount = 0;
  }
  return;
}

// In the following four functions, the servo's are moved according to the (via emg) chosen action.
// At the moment there's too much servo noise while they are moving, so I can't read the emg signal anymore.
// Therefore the state machine output is visualized with 4 LEDs right now, each representing one of the four possible states.

// Motion to set the servo into "open" position
void handOpen(){
  digitalWrite(pinOpen,HIGH);
  digitalWrite(pinClosed,LOW);
  digitalWrite(pinPronation,LOW);
  digitalWrite(pinSupination,LOW);
  //servoThumb.write(180);
  //servoIndex.write(180);
  //servoMiddle.write(180);
  //servoRing.write(180);
  //servoPinky.write(180);
}

// Motion to set the servo into "closed" position
void handClosed(){
  digitalWrite(pinOpen,LOW);
  digitalWrite(pinClosed,HIGH);
  digitalWrite(pinPronation,LOW);
  digitalWrite(pinSupination,LOW);
  //servoThumb.write(85);
  //servoIndex.write(100);
  //servoMiddle.write(100);
  //servoRing.write(100);
  //servoPinky.write(100);
}

void pronation(){
  digitalWrite(pinOpen,LOW);
  digitalWrite(pinClosed,LOW);
  digitalWrite(pinPronation,HIGH);
  digitalWrite(pinSupination,LOW);
  //servoWrist.write(180);
}

void supination(){
  digitalWrite(pinOpen,LOW);
  digitalWrite(pinClosed,LOW);
  digitalWrite(pinPronation,LOW);
  digitalWrite(pinSupination,HIGH);
  //servoWrist.write(120);
}
