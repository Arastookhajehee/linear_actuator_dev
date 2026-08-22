#include <Arduino.h>

// IBT-2 接线
const int RPWM_PIN = 5;   // 正转方向
const int LPWM_PIN = 6;   // 反转方向

int motorPwm = 160;       // 默认速度，范围 0~255

void stopMotor() {
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
}

void motorForward(int pwm) {
  pwm = constrain(pwm, 0, 255);

  // 先关掉反方向，避免两个方向同时输出
  analogWrite(LPWM_PIN, 0);
  analogWrite(RPWM_PIN, pwm);
}

void motorReverse(int pwm) {
  pwm = constrain(pwm, 0, 255);

  // 先关掉反方向，避免两个方向同时输出
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, pwm);
}

void setup() {
  Serial.begin(9600);

  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);

  stopMotor();

  Serial.println("Linear actuator ready");
  Serial.println("f = forward, r = reverse, s = stop");
  Serial.println("Send 0~255 to set PWM speed");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "f" || cmd == "F") {
      motorForward(motorPwm);
      Serial.print("FORWARD, PWM = ");
      Serial.println(motorPwm);
    }
    else if (cmd == "r" || cmd == "R") {
      motorReverse(motorPwm);
      Serial.print("REVERSE, PWM = ");
      Serial.println(motorPwm);
    }
    else if (cmd == "s" || cmd == "S") {
      stopMotor();
      Serial.println("STOP");
    }
    else {
      int value = cmd.toInt();

      // 只有纯数字且范围正确时才更新速度
      bool isNumber = true;
      for (unsigned int i = 0; i < cmd.length(); i++) {
        if (!isDigit(cmd[i])) {
          isNumber = false;
          break;
        }
      }

      if (isNumber && value >= 0 && value <= 255) {
        motorPwm = value;
        Serial.print("PWM set to ");
        Serial.println(motorPwm);
      } else {
        Serial.println("Invalid command");
      }
    }
  }
}