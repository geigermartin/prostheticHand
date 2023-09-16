#include <Servo.h>
#define numOfValsRec 6
#define digitsPerValRec 3

Servo servoThumb;
Servo servoIndex;
Servo servoMiddle;
Servo servoRing;
Servo servoPinky;
Servo servoWrist;

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
  servoWrist.attach(12);
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
  servoThumb.write(valsRec[0]);
  servoIndex.write(valsRec[1]);
  servoMiddle.write(valsRec[2]);
  servoRing.write(valsRec[3]);
  servoPinky.write(valsRec[4]);
  servoWrist.write(valsRec[5]);
}
