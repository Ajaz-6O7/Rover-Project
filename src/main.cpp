#include <Arduino.h>
#include <Servo.h>
#include <IRremote.hpp>

Servo Scanner;

const int Buzzpin = 12;

// Ultrasonic sensor pins
const int TrigPin = A2;
const int EchoPin = A1;

// right motors
const int R_in1 = 4;
const int R_in2 = 3;
const int R_spd_pin = 5;  // PWM A pin

// left motors
const int L_in1 = 7;
const int L_in2 = 8;
const int L_spd_pin = 6;  // PWM B pin

const int STBY = A5;  // Standby pin on driver
const int MAX_spd = 180;
const int trn_spd = 100;
const int spd_rate = 15;  // acceleration/deceleration rate when moving(adjust
                          // to change speed, min = 1, max = MAX_spd val)
const int trn_rate = 15;
const int Vpin = A0;  // analog pin used to measure voltage of battery
const float LOW_battery_V =
    6.6;  // Lowest total V level of battery when motors need to be shutdown for
          // protecting  lithium cells from danger zone.
const int threshold = 250;
const int BeepGap = 3000;
const float safe_dist = 50.0;  // obstacle avoidance distance in auto mode(cm).

// no need to change the values below, these are for commands
const int Move_Forwad = 1;
const int Move_Backward = 2;
const int Turn_Left = 3;
const int Turn_Right = 4;
const int servo_left = 1;
const int servo_right = 2;
const int servo_front = 3;
unsigned long Lastdecode = 0;
unsigned long LastBeep = 0;
bool motor_stopped;
bool AutoMode_ON;
bool new_ang;
float batteryV;
float temperature;
float distance;
String LastCall;

float find_distance(int direction) {
  switch (direction) {
    case servo_left:
      Scanner.write(150);
      new_ang = true;
      break;
    case servo_right:
      Scanner.write(30);
      new_ang = true;
      break;
    case servo_front:
      Scanner.write(90);
      new_ang = false;
      break;
  }
  if (new_ang) {
    delay(500);
  }
  digitalWrite(TrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW);

  return pulseIn(EchoPin, HIGH) * 0.034 / 2;
}

float measure_Voltage() {
  batteryV = analogRead(Vpin) * (5.2 / 1024.0) *
             2.0;  // multiplying by 2 two get full voltage reading of battery(V
                   // is divided by 2 in two identical Resistors in series.)
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

bool rover_stop() {
  if (LastCall == "Left" || LastCall == "Right") {
    for (int s = trn_spd; s >= 0; s -= trn_rate) {
      analogWrite(L_spd_pin, s);
      analogWrite(R_spd_pin, s);
      delay(10);
    }
  } else {
    for (int s = MAX_spd; s >= 0; s -= spd_rate) {
      analogWrite(L_spd_pin, s);
      analogWrite(R_spd_pin, s);
      delay(10);
    }
  }

  digitalWrite(L_in1, LOW);
  digitalWrite(L_in2, LOW);
  digitalWrite(R_in1, LOW);
  digitalWrite(R_in2, LOW);

  motor_stopped = true;
  return motor_stopped;
}

bool Rover(int command) {
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
        for (int s = 0; s <= trn_spd; s += trn_rate) {
          rover_turnleft(s, s);
          delay(10);
        }
      }
      rover_turnleft(trn_spd, trn_spd);
      break;

    case Turn_Right:
      if (motor_stopped == true) {
        for (int s = 0; s <= trn_spd; s += trn_rate) {
          rover_turnright(s, s);
          delay(10);
        }
      }
      rover_turnright(trn_spd, trn_spd);
      break;
  }
  motor_stopped = false;
  return motor_stopped;
}

bool Auto_Mode() {
  while (AutoMode_ON) {
    distance = find_distance(servo_front);
    Serial.print("dist:");
    Serial.println(distance);

    if (distance >= safe_dist || distance == 0.00) {
      Rover(Move_Forwad);
      LastCall = "Forwad";
    } else if (distance < safe_dist && distance != 0.00) {
      rover_stop();
      delay(500);

      float R_dist = find_distance(servo_right);
      float L_dist = find_distance(servo_left);

      if (R_dist >= L_dist) {
        Rover(Turn_Right);
        LastCall = "Right";
      } else {
        Rover(Turn_Left);
        LastCall = "Left";
      }
      delay(300);
      rover_stop();

      Scanner.write(90);
      delay(500);
    }
    measure_Voltage();
    if (batteryV < LOW_battery_V) {
      AutoMode_ON = false;
    } else if (IrReceiver.decode() && IrReceiver.decodedIRData.command != 0x0) {
      if (IrReceiver.decodedIRData.command == 0x30) {
        AutoMode_ON = false;
      }
      delay(10);
    }
    IrReceiver.resume();
  }
  return AutoMode_ON;
  return motor_stopped;
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
  pinMode(TrigPin, OUTPUT);
  pinMode(EchoPin, INPUT);

  digitalWrite(STBY, HIGH);

  Scanner.attach(10);
  Serial.begin(9600);

  IrReceiver.begin(2, ENABLE_LED_FEEDBACK);

  rover_stop();
  AutoMode_ON = false;
  LastCall = "Stop";
}

void loop() {
  measure_Voltage();
  if (batteryV > LOW_battery_V) {
    digitalWrite(STBY, HIGH);

    if (IrReceiver.decode() && IrReceiver.decodedIRData.command != 0x0) {
      Serial.print("Code:");
      Serial.println(IrReceiver.decodedIRData.command, HEX);
      switch (IrReceiver.decodedIRData.command) {
        // im using 2 cases for each code beacuse I plan to use 2 remotes, just
        // 1 case for each is enough.
        case 0x46:
        case 0x58:
          Rover(Move_Forwad);
          LastCall = "Forwad";
          break;
        case 0x15:
        case 0x59:
          Rover(Move_Backward);
          LastCall = "Backward";
          break;
        case 0x44:
        case 0x5A:
          Rover(Turn_Left);
          LastCall = "Left";
          break;
        case 0x43:
        case 0x5B:
          Rover(Turn_Right);
          LastCall = "Right";
          break;
        case 0x2C:
          AutoMode_ON = true;
          Auto_Mode();
          break;
      }
      Lastdecode = millis();
    }
    IrReceiver.resume();
    if (Lastdecode != 0 && millis() - Lastdecode > threshold) {
      rover_stop();
    }
    Serial.println(motor_stopped);
  } else {
    rover_stop();
    digitalWrite(STBY, LOW);

    if (batteryV >= 0.10 && batteryV <= LOW_battery_V) {
      Alarm();
    }
  }
  delay(10);
}