#include<stdio.h>
#include <ESP8266WiFi.h>
#include<stdlib.h>
#include<espnow.h>
#include <WiFiUdp.h>
uint8_t broadcasrAddress[] = {0x84, 0xCC, 0xA8, 0x9E, 0xE4, 0xC8};
//定义信道
#define CHANNEL 1
//主端初始化程序
void cofiginit(){
  WiFi.mode(WIFI_STA);
  if(esp_now_init() != 0){
    Serial.println("Erresp");
    return;
  }
}
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}
void setup() {
  //测试按钮设置
  pinMode(D3, INPUT_PULLUP);
  Serial.begin(115200);
  cofiginit();
  esp_now_register_send_cb(OnDataSent);
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  //先前创建的功能。
  esp_now_register_send_cb(OnDataSent);
  
  //与另一个ESP-NOW设备配对以发送数据
  esp_now_add_peer(broadcasrAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}
int data = 0;
void loop() {
  if(digitalRead(D3) == LOW){
    data++;
    esp_now_send(broadcasrAddress, (uint8_t *) &data, sizeof(data));
    delay(300);
  }
}
