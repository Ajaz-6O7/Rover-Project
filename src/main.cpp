#include <Arduino.h>

#include <IRremote.hpp>

const int Buzzpin = 12;

// left motors
const int L_in1 = 7;
const int L_in2 = 8;
const int L_spd_pin = 6;  // PWM A pin

// right motors
const int R_in1 = 10;
const int R_in2 = 11;
const int R_spd_pin = 9;  // PWM B pin

bool motor_stopped;
const int STBY = 4;  // Standby pin on driver
const int MAX_spd = 180;
const int spd_rate = 5;  // acceleration/deceleration rate when moving(adjust
                         // to change speed, min = 1, max = MAX_spd val)
const int Vpin = A0;     // analog pin used to measure voltage of battery

float batteryV;  // measured Voltage of battery
const float LOW_battery_V =
    6.6;  // Lowest total V level of battery when motors need to be shutdown for
          // protecting  lithium cells from danger zone.

unsigned long Lastdecode = 0;
unsigned long LastBeep = 0;
const int threshold = 250;
const int BeepGap = 3000;
// no need to change the values below, these are for commands
const int Move_Forwad = 1;
const int Move_Backward = 2;
const int Turn_Left = 3;
const int Turn_Right = 4;

float measure_Voltage() {
  analogRead(Vpin);
  delay(2);
  batteryV = analogRead(Vpin) * (5.0 / 1024.0) *
             2.0;  // multiplying by 2 two get full voltage reading of battery
                   // (V is divided by 2 in two identical Resistors in series.)

  Serial.print(batteryV);
  Serial.print("V  || ");

  return batteryV;
}

unsigned long Alarm() {
  if ((millis() - LastBeep) >= BeepGap) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(Buzzpin, HIGH);
      delay(100);
      digitalWrite(Buzzpin, LOW);
      delay(100);
    }
    LastBeep = millis();
  }
}

void rover_moveforwad(int speed) {
  // speed
  analogWrite(L_spd_pin, speed);
  analogWrite(R_spd_pin, speed);

  // leftside forwad
  digitalWrite(L_in1, HIGH);
  digitalWrite(L_in2, LOW);

  // rightside forwad
  digitalWrite(R_in1, HIGH);
  digitalWrite(R_in2, LOW);
}

void rover_movebackward(int speed) {
  // speed
  analogWrite(L_spd_pin, speed);
  analogWrite(R_spd_pin, speed);

  // leftside backwards
  digitalWrite(L_in1, LOW);
  digitalWrite(L_in2, HIGH);

  // rightside backwards
  digitalWrite(R_in1, LOW);
  digitalWrite(R_in2, HIGH);
}

void rover_turnleft(int L_spd, int R_spd) {
  // speed
  analogWrite(L_spd_pin, L_spd);
  analogWrite(R_spd_pin, R_spd);

  // leftside backwards
  digitalWrite(L_in1, LOW);
  digitalWrite(L_in2, HIGH);

  // rightside forwad
  digitalWrite(R_in1, HIGH);
  digitalWrite(R_in2, LOW);
}

void rover_turnright(int L_spd, int R_spd) {
  // speed
  analogWrite(L_spd_pin, L_spd);
  analogWrite(R_spd_pin, R_spd);

  // leftside forwads
  digitalWrite(L_in1, HIGH);
  digitalWrite(L_in2, LOW);

  // rightside backwards
  digitalWrite(R_in1, LOW);
  digitalWrite(R_in2, HIGH);
}

void rover_stop() {
  for (int s = MAX_spd; s >= 0; s -= spd_rate) {
    analogWrite(L_spd_pin, s);
    analogWrite(R_spd_pin, s);
    delay(10);
  }

  digitalWrite(L_in1, LOW);
  digitalWrite(L_in2, LOW);
  digitalWrite(R_in1, LOW);
  digitalWrite(R_in2, LOW);
}

void Rover(int command) {
  switch (command) {
    case Move_Forwad:
      if (motor_stopped == true) {
        for (int s = 0; s <= MAX_spd; s += spd_rate) {
          rover_moveforwad(s);
          delay(10);
        }
      } else {
        rover_moveforwad(MAX_spd);
      }
      break;

    case Move_Backward:
      if (motor_stopped == true) {
        for (int s = 0; s <= MAX_spd; s += spd_rate) {
          rover_movebackward(s);
          delay(10);
        }
      } else {
        rover_movebackward(MAX_spd);
      }
      break;

    case Turn_Left:
      if (motor_stopped == true) {
        for (int s = 0; s <= MAX_spd; s += spd_rate) {
          rover_turnleft(s, s);
          delay(10);
        }
      } else {
        rover_turnleft(MAX_spd, MAX_spd);
      }
      break;

    case Turn_Right:
      if (motor_stopped == true) {
        for (int s = 0; s <= MAX_spd; s += spd_rate) {
          rover_turnright(s, s);
          delay(10);
        }
      } else {
        rover_turnright(MAX_spd, MAX_spd);
      }
      break;
  }
}

void setup() {
  pinMode(L_spd_pin, OUTPUT);
  pinMode(R_spd_pin, OUTPUT);
  pinMode(L_in1, OUTPUT);
  pinMode(L_in2, OUTPUT);
  pinMode(R_in1, OUTPUT);
  pinMode(R_in2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(Buzzpin, OUTPUT);

  digitalWrite(STBY, HIGH);

  Serial.begin(9600);

  IrReceiver.begin(2, ENABLE_LED_FEEDBACK);

  rover_stop();
}

void loop() {
  measure_Voltage();
  if (batteryV > LOW_battery_V) {
    digitalWrite(STBY, HIGH);

    if (IrReceiver.decode()) {
      switch (IrReceiver.decodedIRData.command) {
        // im using 2 cases for each code beacuse I plan to use 2 remotes, just
        // 1 case for each is enough.
        case 0x46:
        case 0x58:
          Serial.println("forwad, ");
          Rover(Move_Forwad);
          break;
        case 0x15:
        case 0x59:
          Serial.println("backward, ");
          Rover(Move_Backward);
          break;
        case 0x44:
        case 0x5A:
          Serial.println("left, ");
          Rover(Turn_Left);
          break;
        case 0x43:
        case 0x5B:
          Serial.println("right, ");
          Rover(Turn_Right);
          break;
      }
      Lastdecode = millis();
      IrReceiver.resume();
    } else {  // this else statement is completely optional.its for making
              // serial monitor output looks good.
      Serial.println();
    }

    if (Lastdecode != 0 && millis() - Lastdecode > threshold) {
      rover_stop();
      motor_stopped = true;
    } else {
      motor_stopped = false;
    }

  } else {
    digitalWrite(STBY, LOW);
    rover_stop();
    if (batteryV >= 0.10 && batteryV <= LOW_battery_V) {
      Serial.println("LOW BATTERY!!!");
      Alarm();
    } else {
      Serial.println("NO BATTERY FOUND!!!");
    }
  }
  delay(10);
}