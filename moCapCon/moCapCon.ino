#include <Servo.h>
#define numOfValsRec 5
#define digitsPerValRec 3

Servo servoThumb;
Servo servoIndex;
Servo servoMiddle;
Servo servoRing;
Servo servoPinky;

int valsRec[numOfValsRec];
int stringLength = numOfValsRec * digitsPerValRec + 1;
int counter = 0;
bool counterStart = false;
String receivedString;

void setup() {
  Serial.begin(9600);
  servoThumb.attach(2);
  servoIndex.attach(4);
  servoMiddle.attach(6);
  servoRing.attach(8);
  servoPinky.attach(10);
}

void receiveData(){
  while(Serial.available())
  {
    char c = Serial.read();
    if (c=='$'){
      counterStart = true;
    }
    if (counterStart){
      if (counter < stringLength){
        receivedString = String(receivedString + c);
        counter++;
      }
      if (counter >= stringLength){
        for(int i = 0; i < numOfValsRec; i++){
          int num = (i*digitsPerValRec)+1;
          valsRec[i] = receivedString.substring(num, num + digitsPerValRec).toInt();
        }
        receivedString = "";
        counter = 0;
        counterStart = false;
      }
    }
  }
}

void loop() {
  receiveData();
  int thumb = valsRec[0];
  int index = valsRec[1];
  int middle = valsRec[2];
  int ring = valsRec[3];
  int pinky = valsRec[4];
  servoThumb.write(thumb);
  servoIndex.write(index);
  servoMiddle.write(middle);
  servoRing.write(ring);
  servoPinky.write(pinky);
}
