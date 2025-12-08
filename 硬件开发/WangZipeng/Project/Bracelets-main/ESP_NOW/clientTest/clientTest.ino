#include<stdio.h>
#include <ESP8266WiFi.h>
#include<stdlib.h>
#include<espnow.h>
#include <WiFiUdp.h>
//定义通讯通道
#define CHANNEL 1
//初始化espnow协议函数
void InitESPNOW(){
  WiFi.disconnect();
  if(esp_now_init() == 0){
    Serial.printf("\nESPNOW协议已开启\n");
  }else{
    Serial.printf("\nESPNOW协议开启失败\n");
    ESP.restart();
  }
}
//配置从端的Slave名称AP模式
void configDeviceAP(){
  const char *SSID = "Slave_dream";
  bool result = WiFi.softAP(SSID,"Slave_dream_Password",CHANNEL,0);
  if(result){
    Serial.printf("\nAP模式开启失败\n");
  }else{
    Serial.printf("\nAP模式开启成功");
  }
}
void setup() {
  // put your setup code here, to run once:
  //配置串口波率
  Serial.begin(9600);
  Serial.printf("开启espnow协议\n");
  WiFi.mode(WIFI_AP);
  //调用初始化配置函数
  configDeviceAP();
  //展示当前MAC地址
  Serial.printf("\n当前地址为:");
  Serial.println(WiFi.softAPmacAddress());
  InitESPNOW();
  //注册回调函数
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
}
//接收到数据后的回调函数
void OnDataRecv(uint8_t * mac, uint8_t *data, uint8_t len){
    Serial.println(*data);
}
void loop() {
  // put your main code here, to run repeatedly:
  
}
