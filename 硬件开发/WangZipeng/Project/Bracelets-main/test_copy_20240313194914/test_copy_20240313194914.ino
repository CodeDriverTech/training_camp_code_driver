#include <ESP8266WiFi.h>
#include <Arduino.h>
#define ssid "dreamsky"
#define password "lina.8460121"
WiFiEventHandler STAGotIP;//创捷一个WiFiEventHandler类型的STAGotIP变量用于承接事件处理函数
int Aswitch = D3 , LEDIN = D4;
int juage;
//开关LED函数
void switchLEDFounction()
{dd
  int LEDARR[7] = {D0,D1,D2,D5,D6,D7,D8};
  for(int i = 0 ; i < 7; i++ ){
    digitalWrite(LEDARR[i],HIGH);
    delay(60);
    digitalWrite(LEDARR[i],LOW);
    if(i<6)
      delay(60);
  }
  for(int i = 7 ; i > 0 ;i--)
  {
    digitalWrite(LEDARR[i],HIGH);
    delay(60);
    digitalWrite(LEDARR[i],LOW);
    delay(60);
  }
}
//起爆器函数
void BOOM()
{
  int BOOMNum = 1000;
  for(int i = 0 ;  i < 1000 ; i++){
    digitalWrite(D8,HIGH);
    delay(1000);
    digitalWrite(D8,LOW);
    delay(BOOMNum);
    
  }
}
void setup() {
  
  Serial.begin(115200);
  pinMode(D0,OUTPUT);
  pinMode(D1,OUTPUT);
  pinMode(D2,OUTPUT);
  //pinMode(D3,OUTPUT);
  //pinMode(D4,OUTPUT);
  pinMode(D6,OUTPUT);
  pinMode(D5,OUTPUT);d
  pinMode(D7,OUTPUT);d
  pinMode(D8,OUTPUT);d
  
  pinMode(Aswitch,INPUT_PULLUP);
  /*
  WiFi.mode(WIFI_STA); 
  pinMode(LEDIN,OUTPUT);//设定D4为输出引脚
  //STAGotIP = WiFi.onStationModeGotIP(GotIPHandler);
  WiFi.begin(ssid, password); 
  Serial.print("正在开始连接");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    digitalWrite(LEDIN,HIGH);
    delay(1000);
    digitalWrite(LEDIN,LOW);  
    Serial.print(".");
  }
  for(int i = 0 ; i < 3 ; i++)
  {
    digitalWrite(LEDIN,0);
    delay(500);
    digitalWrite(LEDIN,1);
    delay(2000);

  }
  
  digitalWrite(LEDIN,0);
  digitalWrite(LEDIN,0);
  digitalWrite(LEDIN,0);
  digitalWrite(LEDIN,0);
  Serial.print("目前wifi状态为；");
  Serial.println(WiFi.getMode());
  Serial.println("\n模块获得IP：");
  Serial.print(WiFi.localIP());
  */
}

void loop() {
  // put your main code here, to run repeatedly:

  if(digitalRead(Aswitch) == LOW)
    BOOM();

  //digitalWrite(D4,HIGH);
  //delay(1000);
  //digitalWrite(D4,LOW);  
}

/*
 void GotIPHandler(const WiFiEventStationModeGotIP &event)//当无线终端连接到WiFi后，设备都会自动调用
{
  Serial.println("模块获得IP：");
  Serial.print(WiFi.localIP());//输出NodeMCU的IP地址。这一功能是通过 WiFi.localIP()函数来实现的。该函数的返回值即NodeMCU的IP地址。
}
*/


