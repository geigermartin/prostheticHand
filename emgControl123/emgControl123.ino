// How to attach EMG: https://electropeak.com/learn/interfacing-emg-muscular-signal-sensor-with-arduino/

// Remember: To get the servo noise to a level this approach can handle ...
// - ground laptop (connect to power supply)
// - make sure all wires are connected properly
// - sometimes necessary: stand up from chair, take off headphones

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
int threshold = map(300,0,1023,0,1023);
byte state = 1;
byte nContractions = 2;
unsigned long contractionStart;
int plotCount = 0;
bool waitTwoContraction = false;
bool servoNoisePassed = false;
bool servoNoiseStarted = false;

// Filter
float EMA_a = 0.01;
int EMA_S = 0;

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
  EMA_S = analogRead(0); //set EMA S for t=1
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
        waitTwoContraction = true;
      }
      if (emgFilt > threshold && nContractions == 1){
        nContractions = 2;
        switchState(state, nContractions);
        //delay((contractionStart + maxT) - millis());
        waitServoNoise();
        break;
      }
      if ((millis() == (contractionStart + maxT)) && waitTwoContraction){
        switchState(state, nContractions);
        waitTwoContraction = false;
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
  EMA_S = (EMA_a*emgValue) + ((1-EMA_a)*EMA_S);  //run the EMA
  return EMA_S;
}

void waitServoNoise(){
  // With this function I wait for a 3rd suprathreshold value while being in the state of nContractions==2
  // Reason: After state detection and servo command, the servos move which results in noise in the sensor readout.
  // This noise leads to another (the 3rd) threshold crossing. As it is predictable however, I just wait for it here.
  // If I wouldn't catch this, it would get interpreted as the next 'nContractions=1' event and would lead to the according action. 
  servoNoisePassed = false;
  while (servoNoisePassed == false){
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
    if (emgFilt > threshold){
      servoNoiseStarted = true;
    }
    if (emgFilt < threshold && servoNoiseStarted){
      servoNoisePassed = true;
      servoNoiseStarted = false;
    }
    delay(1);
  }
}

void plot(){
  plotCount++;
  if (plotCount >= 200){
    if (pyGUI){
      Serial.println(EMA_S);
      Serial.print(",");
      Serial.println(threshold);
    } else if (arIDE){
      Serial.print(emgValue);
      Serial.print(" ");
      Serial.print(EMA_S);
      Serial.print(" ");
      Serial.println(threshold);
    }
    plotCount = 0;
  }
  return;
}

// While I was having issues with servo noise in sensor data, I used LEDs to visualize the state machine.
// Remove or comment out all digitalWrite commands in the following, to not use the LEDs.

// Motion to set the servo into "open" position
void handOpen(){
  digitalWrite(pinOpen,HIGH);
  digitalWrite(pinClosed,LOW);
  digitalWrite(pinPronation,LOW);
  digitalWrite(pinSupination,LOW);
  servoThumb.write(180);
  servoIndex.write(180);
  servoMiddle.write(180);
  servoRing.write(180);
  servoPinky.write(180);
}

// Motion to set the servo into "closed" position
void handClosed(){
  digitalWrite(pinOpen,LOW);
  digitalWrite(pinClosed,HIGH);
  digitalWrite(pinPronation,LOW);
  digitalWrite(pinSupination,LOW);
  servoThumb.write(85);
  servoIndex.write(100);
  servoMiddle.write(100);
  servoRing.write(100);
  servoPinky.write(100);
}

void pronation(){
  digitalWrite(pinOpen,LOW);
  digitalWrite(pinClosed,LOW);
  digitalWrite(pinPronation,HIGH);
  digitalWrite(pinSupination,LOW);
  servoWrist.write(180);
}

void supination(){
  digitalWrite(pinOpen,LOW);
  digitalWrite(pinClosed,LOW);
  digitalWrite(pinPronation,LOW);
  digitalWrite(pinSupination,HIGH);
  servoWrist.write(90);
}
