// How to attach EMG: https://electropeak.com/learn/interfacing-emg-muscular-signal-sensor-with-arduino/

// Choose whether to execute via Python GUI or Arduino IDE
bool pyGUI = true;
bool arIDE = false;
//Current problem with pyGUI: Threshold isn't working

// Libraries
#include <HardwareSerial.h>
#include <Servo.h>

// Define variables
int emgValue;
int threshold = map(100,0,1023,0,1023);
byte state = 1;
byte nContractions = 0;
unsigned long contractionStart;
int plotCount = 0;

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
  servoThumb.attach(4);
  servoIndex.attach(2);
  servoMiddle.attach(6);
  servoRing.attach(8);
  servoPinky.attach(10);
  servoWrist.attach(12);
  pinMode(2,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(8,OUTPUT);
}

void loop() {
  //emgValue = analogRead(A0);
  switchState(state, nContractions);
  if (pyGUI){
    threshold = read(threshold);
  } 
  if (arIDE){
    emgValue = analogRead(A0);
    plot();
  }
  //plot();
  if (emgValue > threshold){
    contractionStart = millis(); // get time
    nContractions = 0;
    while (millis() < (contractionStart + 500)){
      //emgValue = analogRead(A0);
      if (pyGUI){
        threshold = read(threshold);
      } 
      if (arIDE){
        emgValue = analogRead(A0);
        plot();
      }
      if (emgValue < threshold){
        nContractions = 1;
        switchState(state, nContractions);
      }
      if (emgValue > threshold && nContractions == 1){
        nContractions = 2;
        switchState(state, nContractions);
        delay((contractionStart + 500) - millis());
        break;
      }
    }    
    if (nContractions == 0){
      while (emgValue > threshold){
        //emgValue = analogRead(A0);
        if (pyGUI){
          threshold = read(threshold);
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
        } else {
          nContractions = 1;
          switchState(state, nContractions);
        }
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
    char userInput = Serial.read(); // read user input
      if (userInput == 'r'){
        int data = analogRead(0);
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
    Serial.println(emgValue);
    Serial.print(",");
    Serial.println(threshold);
    plotCount = 0;
  }
  return;
}

// Motion to set the servo into "open" position
void handOpen(){
  digitalWrite(2,HIGH);
  digitalWrite(4,LOW);
  digitalWrite(6,LOW);
  digitalWrite(8,LOW);
  //servoThumb.write(180);
  //servoIndex.write(180);
  //servoMiddle.write(180);
  //servoRing.write(180);
  //servoPinky.write(180);
}

// Motion to set the servo into "closed" position
void handClosed(){
  digitalWrite(2,LOW);
  digitalWrite(4,HIGH);
  digitalWrite(6,LOW);
  digitalWrite(8,LOW);
  //servoThumb.write(60);
  //servoIndex.write(0);
  //servoMiddle.write(0);
  //servoRing.write(0);
  //servoPinky.write(0);
}

void pronation(){
  digitalWrite(2,LOW);
  digitalWrite(4,LOW);
  digitalWrite(6,HIGH);
  digitalWrite(8,LOW);
  //servoWrist.write(180);
}

void supination(){
  digitalWrite(2,LOW);
  digitalWrite(4,LOW);
  digitalWrite(6,LOW);
  digitalWrite(8,HIGH);
  //servoWrist.write(0);
}
