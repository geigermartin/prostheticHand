// Libraries
#include <Servo.h>

int servoPos[] = {180,180,180,180,180,180};
char endMarker = '>';
bool gesture = false;
//int servoVal;

// Define servos
Servo servoThumb;     
Servo servoIndex;
Servo servoMiddle;
Servo servoRing;
Servo servoPinky;
Servo servoWrist;

void setup() {
  Serial.begin(9600);
  servoThumb.attach(2);
  servoIndex.attach(4);
  servoMiddle.attach(6);
  servoRing.attach(8);
  servoPinky.attach(10);
  servoWrist.attach(12);
}

void loop() {
  String arduinoIn = "";
  
  if (!Serial.available()){
    return;
  }
  while (Serial.available()){
    while (Serial.available() > 0){
      String c = Serial.readString();
      if (c != endMarker){
        arduinoIn += c;
      } else {
        break;
      }
    }
  }

  if (arduinoIn.substring(1,3) == "sT"){
    servoPos[0] = arduinoIn.substring(3,6).toInt();
  }
  else if (arduinoIn.substring(1,3) == "sI"){
    servoPos[1] = arduinoIn.substring(3,6).toInt();
  }
  else if (arduinoIn.substring(1,3) == "sM"){
    servoPos[2] = arduinoIn.substring(3,6).toInt();
  }
  else if (arduinoIn.substring(1,3) == "sR"){
    servoPos[3] = arduinoIn.substring(3,6).toInt();
  }
  else if (arduinoIn.substring(1,3) == "sP"){
    servoPos[4] = arduinoIn.substring(3,6).toInt();
  }
  else if (arduinoIn.substring(1,3) == "sW"){
    servoPos[5] = arduinoIn.substring(3,6).toInt();
  }
  if (arduinoIn.substring(1,2) == "g"){
    gesture = true;
    //Serial.println(arduinoIn.substring(2));
    if (arduinoIn.substring(2) == "Hand open>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 180;
      servoPos[4] = 180;
      servoPos[5] = 180;
    }
    else if (arduinoIn.substring(2) == "Hand closed>"){
      servoPos[0] = 60;
      servoPos[1] = 0;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Peace>"){
      servoPos[0] = 60;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Spiderman>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 180;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Thumbs Up>"){
      servoPos[0] = 180;
      servoPos[1] = 0;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 100;    
    }
    else if (arduinoIn.substring(2) == "Point in direction>"){
      servoPos[0] = 60;
      servoPos[1] = 180;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "One>"){
      servoPos[0] = 180;
      servoPos[1] = 0;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Two>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Three>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Four>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 180;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Five>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 180;
      servoPos[4] = 180;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Bang>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Little finger up>"){
      servoPos[0] = 60;
      servoPos[1] = 0;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 180;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Heavy metal>"){
      servoPos[0] = 60;
      servoPos[1] = 180;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 180;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Call me>"){
      servoPos[0] = 180;
      servoPos[1] = 0;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 180;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Rock>"){
      servoPos[0] = 60;
      servoPos[1] = 0;
      servoPos[2] = 0;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
    else if (arduinoIn.substring(2) == "Paper>"){
      servoPos[0] = 180;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 180;
      servoPos[4] = 180;
      servoPos[5] = 180;
    }
    else if (arduinoIn.substring(2) == "Scissor>"){
      servoPos[0] = 60;
      servoPos[1] = 180;
      servoPos[2] = 180;
      servoPos[3] = 0;
      servoPos[4] = 0;
      servoPos[5] = 180;    
    }
  }

  //servoVal = arduinoIn.substring(3,6).toInt();
  //Serial.println(servoVal);
  //if (servoVal > 100) {
  //  allToOpen();
  //} else {
  //  allToClosed();
  //}
  moveServos(servoPos, gesture);
}

bool moveServos(int servoPos[], bool gesture){
  servoThumb.write(servoPos[0]);
  servoIndex.write(servoPos[1]);
  servoMiddle.write(servoPos[2]);
  servoRing.write(servoPos[3]);
  servoPinky.write(servoPos[4]);
  servoWrist.write(servoPos[5]);
  if (gesture){
    delay(5000);
    servoThumb.write(180);
    servoIndex.write(180);
    servoMiddle.write(180);
    servoRing.write(180);
    servoPinky.write(180);
    servoWrist.write(180);
    servoPos[0] = 180;
    servoPos[1] = 180;
    servoPos[2] = 180;
    servoPos[3] = 180;
    servoPos[4] = 180;
    servoPos[5] = 180;
    gesture = false;
  }
  
  Serial.flush();
  while(Serial.available()){Serial.read();}
  return gesture;
}
