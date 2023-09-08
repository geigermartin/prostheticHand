// How to attach EMG: https://electropeak.com/learn/interfacing-emg-muscular-signal-sensor-with-arduino/

// Thresholding is not working atm

// Choose whether to execute via Python GUI or Arduino IDE
bool pyGUI = true;
bool arIDE = false;
// Set max time for 2 contractions
int maxT = 800;
// Use filter or not
//bool filter = 0;

// Libraries
#include <HardwareSerial.h>
#include <Servo.h>

// Define variables
int emgValue;
int threshold = map(100,0,1023,0,1023);
byte state = 1;
byte nContractions = 2;
unsigned long contractionStart;
int plotCount = 0;
float EMA_a = 0.5;    //initialization of EMA alpha
int EMA_S = 0;        //initialization of EMA S

// Define servos
Servo servoThumb;     
Servo servoIndex;
Servo servoMiddle;
Servo servoRing;
Servo servoPinky;
Servo servoWrist;

// Pins on Arduino
void setup() {
  Serial.begin(9600);
  servoThumb.attach(2);
  servoIndex.attach(4);
  servoMiddle.attach(6);
  servoRing.attach(8);
  servoPinky.attach(10);
  servoWrist.attach(12);
  EMA_S = analogRead(A0);      //set EMA S for t=1
}

void loop() {
  //emgValue = analogRead(A0);
  //switchState(state, nContractions);
  if (pyGUI){
    threshold = read(threshold);
    emgValue = analogRead(A0);
    EMA_S = (EMA_a*emgValue) + ((1-EMA_a)*EMA_S);  //run the EMA
  } 
  if (arIDE){
    emgValue = analogRead(A0);
    EMA_S = (EMA_a*emgValue) + ((1-EMA_a)*EMA_S);  //run the EMA
    plot();
  }
  //plot();
  if (EMA_S > threshold){
    contractionStart = millis(); // get time
    nContractions = 0;
    while (millis() < (contractionStart + maxT)){
      //emgValue = analogRead(A0);
      if (pyGUI){
        threshold = read(threshold);
        emgValue = analogRead(A0);
        EMA_S = (EMA_a*emgValue) + ((1-EMA_a)*EMA_S);  //run the EMA
      } 
      if (arIDE){
        emgValue = analogRead(A0);
        plot();
      }
      if (EMA_S < threshold){
        nContractions = 1;
        switchState(state, nContractions);
      }
      if (EMA_S > threshold && nContractions == 1){
        nContractions = 2;
        switchState(state, nContractions);
        delay((contractionStart + maxT) - millis());
        break;
      }
    }
    //switchState(state, nContractions);
    if (nContractions == 0){
      while (EMA_S > threshold){
        //emgValue = analogRead(A0);
        if (pyGUI){
          threshold = read(threshold);
          emgValue = analogRead(A0);
          EMA_S = (EMA_a*emgValue) + ((1-EMA_a)*EMA_S);  //run the EMA
        } 
        if (arIDE){
          emgValue = analogRead(A0);
          plot();
        }
        if (millis() > (contractionStart + 2000)){
          //pronation();
          if (state == 1){
            state = 2;        
            //switchState(state, nContractions);
            break;
          } 
          else if (state == 2) {
            state = 1;
            //switchState(state, nContractions);
            break;
          }
        } //else {
          //nContractions = 1;
          //switchState(state, nContractions);
        //}
      }
    }
  }
}

void switchState(int state, int nContractions){
  if (state == 1 && nContractions == 1){
    handClosed();
  }
  else if (state == 1 && nContractions == 2){
    handOpen();
  }
  else if (state == 2 && nContractions == 1){
    pronation();
  }
  else if (state == 2 && nContractions == 2){
    supination();
  }
}

int read(int threshold){
  char endMarker = '>';
  if (Serial.available() > 0){ 
    emgValue = analogRead(A0);
    EMA_S = (EMA_a*emgValue) + ((1-EMA_a)*EMA_S);  //run the EMA
    char userInput = Serial.read(); // read user input
      if (userInput == 'r'){
        int data = analogRead(A0);
        Serial.println(data);     
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
          Serial.println(threshold);
          //threshold = map(threshold,0,1023,0,1023);
        }
      }
  }
  return threshold;
}

void plot(){
  plotCount++;
  if (plotCount >= 500){
    Serial.println(EMA_S);
    Serial.print(",");
    Serial.println(threshold);
    plotCount = 0;
  }
  return;
}

// Motion to set the servo into "open" position
void handOpen(){
  //digitalWrite(2,HIGH);
  //digitalWrite(4,LOW);
  //digitalWrite(6,LOW);
  //digitalWrite(8,LOW);
  servoThumb.write(180);
  servoIndex.write(180);
  servoMiddle.write(180);
  servoRing.write(180);
  servoPinky.write(180);
}

// Motion to set the servo into "closed" position
void handClosed(){
  //digitalWrite(2,LOW);
  //digitalWrite(4,HIGH);
  //digitalWrite(6,LOW);
  //digitalWrite(8,LOW);
  servoThumb.write(85);
  servoIndex.write(100);
  servoMiddle.write(100);
  servoRing.write(100);
  servoPinky.write(100);
}

void pronation(){
  //digitalWrite(2,LOW);
  //digitalWrite(4,LOW);
  //digitalWrite(6,HIGH);
  //digitalWrite(8,LOW);
  servoWrist.write(180);
}

void supination(){
  //digitalWrite(2,LOW);
  //digitalWrite(4,LOW);
  //digitalWrite(6,LOW);
  //digitalWrite(8,HIGH);
  servoWrist.write(120);
}
