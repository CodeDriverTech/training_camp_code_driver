//测试按钮按动感知时长框架
#include<stdio.h>
#define LONG_PRESS_TIME 2000  // 长按的定义为2000毫秒

unsigned long buttonPressTime = 0;

void setup() {
  pinMode(D3, INPUT_PULLUP);  // 设置按钮引脚为输入
  pinMode(D4,OUTPUT);//定义输出引脚
}

void loop() {
  static int lastButtonState = HIGH;  // 上一次按钮的状态
  int buttonState = digitalRead(D3);  // 读取按钮的当前状态

  // 检查按钮是否被按下
  if (buttonState == LOW && lastButtonState == HIGH) {
    buttonPressTime = millis();  // 记录按钮被按下的时间
  }

  // 检查按钮是否被释放
  if (buttonState == HIGH && lastButtonState == LOW) {
    unsigned long pressDuration = millis() - buttonPressTime;  // 计算按钮被按下的持续时间
    if (pressDuration >= LONG_PRESS_TIME) {
      Serial.println("Button was long pressed");
      //被长按的处理情况
      digitalWrite(D4,HIGH);//双闪
      delay(100);
      digitalWrite(D4,LOW);
      delay(300);
      digitalWrite(D4,HIGH);
      delay(300);
      digitalWrite(D4,LOW);
      delay(300);
      digitalWrite(D4,HIGH);
    } else {
      Serial.println("Button was short pressed");
      //被短按的处理情况
      if(digitalRead(D4) == LOW){//按钮开关
        digitalWrite(D4,HIGH);
      }else{
        digitalWrite(D4,LOW);
      }
    }
  }

  lastButtonState = buttonState;  // 更新按钮的状态
}