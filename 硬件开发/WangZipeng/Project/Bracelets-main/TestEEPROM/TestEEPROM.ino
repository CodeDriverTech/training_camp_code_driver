#include <ESP8266WiFi.h>
#include<stdlib.h>
#include<stdio.h>
#include<EEPROM.h>

void setup() {
  // put your setup code here, to run once:
  EEPROM.begin(10);//开启eeprom空间申请10个字节
  pinMode(D3, INPUT_PULLUP);
  Serial.begin(115200);
  int num1 = EEPROM.read(0);//读取索引0的空间字节
  Serial.printf("\n");
  Serial.println(num1);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(D3) == LOW){
    int num = 1;
    EEPROM.write(0, num);//把num写入到索引0的空间当中
    EEPROM.commit();
    delay(300);
  }
}
