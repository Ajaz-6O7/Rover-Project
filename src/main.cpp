#include <Arduino.h>
#include <IRremote.hpp>

// left motors
const int L_in1 = 7;
const int L_in2 = 8;
const int L_spd_pin = 6;

// right motors
const int R_in1 = 10;
const int R_in2 = 11;
const int R_spd_pin = 9;

const int STBY = 4;  // Standby pin on driver
const int Vpin = A0; // analog pin used to measure voltage of battery

float batteryV;                  // measured Voltage of battery
const float LOW_battery_V = 6.6; // Lowest total V level of battery when motors need to be shutdown for protecting  lithium cells from danger zone.

unsigned long Lastdecode = 0;
const int threshold = 250;

float measure_Voltage()
{
  batteryV = analogRead(Vpin) * (5.0 / 1024.0) * 2.0; // multiplying by 2 two get full voltage reading of battery (V is divided by 2 in two identical Resistors in series.)

  Serial.print(batteryV);
  Serial.print("V  || ");

  return batteryV;
}

void rover_moveforwad(int speed)
{
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

void rover_movebackward(int speed)
{
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

void rover_turnleft(int L_spd, int R_spd)
{
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

void rover_turnright(int L_spd, int R_spd)
{
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

void rover_stop()
{
  digitalWrite(L_in1, LOW);
  digitalWrite(L_in2, LOW);
  digitalWrite(R_in1, LOW);
  digitalWrite(R_in2, LOW);
}

void setup()
{
  pinMode(L_spd_pin, OUTPUT);
  pinMode(R_spd_pin, OUTPUT);
  pinMode(L_in1, OUTPUT);
  pinMode(L_in2, OUTPUT);
  pinMode(R_in1, OUTPUT);
  pinMode(R_in2, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);

  Serial.begin(9600);

  IrReceiver.begin(2, ENABLE_LED_FEEDBACK);

  rover_stop();
}

void loop()
{
  measure_Voltage();
  if (batteryV > LOW_battery_V)
  {
    digitalWrite(STBY, HIGH);

    if (IrReceiver.decode())
    {
      Lastdecode = millis();

      switch (IrReceiver.decodedIRData.command)
      {
      case 0x46:
      case 0x58:
        Serial.println("forwad, ");
        rover_moveforwad(255);
        break;
      case 0x15:
      case 0x59:
        Serial.println("backward, ");
        rover_movebackward(255);
        break;
      case 0x44:
      case 0x5A:
        Serial.println("left, ");
        rover_turnleft(180, 180);
        break;
      case 0x43:
      case 0x5B:
        Serial.println("right, ");
        rover_turnright(180, 180);
        break;
      }
      IrReceiver.resume();
    } else {
      Serial.println();
    }
    if (Lastdecode != 0 && millis() - Lastdecode > threshold)
    {
      rover_stop();
    }
  }
  else
  {
    digitalWrite(STBY, LOW);
    rover_stop();
    Serial.println("LOW BATTERY!!!");
  }
  delay(200);
}