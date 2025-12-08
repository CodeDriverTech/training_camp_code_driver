#include <ESP8266WiFi.h>
#include <espnow.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(D4,OUTPUT);
  //设置ESP8266模式
  WiFi.mode(WIFI_STA);

  //初始化 ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  //设置ESP8266角色：
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  //先前创建的功能 测试ESP-NOW通信
  esp_now_register_recv_cb(OnDataRecv);
}
int data;
int tdata;
void OnDataRecv(uint8_t * mac, uint8_t *Recdata, uint8_t len) {
  memcpy(&data, Recdata, sizeof(data));
  Serial.println(data);
  
  if(tdata != data){
    if(digitalRead(D4) == LOW){
      digitalWrite(D4, HIGH);
    }else{
      digitalWrite(D4, LOW);
    }
  }
  tdata = data;
}
void loop() {
  // put your main code here, to run repeatedly:
  
}
