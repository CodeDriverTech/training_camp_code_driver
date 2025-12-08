#include <ESP8266WiFi.h>
#include<stdio.h>
#include<stdlib.h>
#include <WiFiUdp.h>

#define ssid "Testhome"//目标名称
#define password "WZP8460121"//密码
#define hostA "192.168.2.1"
/*
测试文件2024/3.23/10:27
**********************************************************************************
dreamsky.0822.wuming
客户端调试测试pm
**********************************************************************************
用于测试开发板的WIFI调试程序以及基础的框架程序测试
*/
int Numconnect = 0;//重新计算链接次数，用于判断是否进行UDP测试互发包
int Numcount = 0;//用于记录判断是否是重连还是正常连接的数值
int Numdisconnect = 0;//客户端设备掉线标识
IPAddress ipsend;//发送方的IP地址，用于认证和回传目的

WiFiUDP Udp;//实例化UDP对象
//定义网络TCP包传输端口
const uint16_t port = 80;//AP模式的监听端口
char* host = "192.168.4.1";//TCP服务器ip
//实例化网络客户端对象
WiFiClient client;
//注册WIFI连接成功事件处理程序
WiFiEventHandler STAconnect;
//注册WIFI连接断开事件处理程序
WiFiEventHandler STAdiconnect;
//灯光闪烁函数(闪一次)
void lighthelper(int Num,int Sce){//Num为次数,sce为间隔时间（毫秒）
  for(int i = 0 ; i< Num ; i++){
    digitalWrite(D4,LOW);
    delay(Sce);
    digitalWrite(D4,HIGH);
    delay(Sce);
  }
}
//进行TCP握手/////////////////////////////////////////Test/////////////////////
void TCPlinkfunction(){
  
  if(!client.connect(host,port)){
    Serial.println("err!,连接错误！");
    return;
  }
}
//////////////////////////////////////////////////////Test////////////////////
//提示灯函数///
void lightLEDinf(int option){//传入1为正常情况快三闪，传入0为发生错误，多次慢四闪
  if(option == 1){
    lighthelper(3,100);
  }else if(option == 0){
    lighthelper(4,800);
  }
  else
    Serial.println("\nlightLEDinf(option)_err,传参错误，1为正常三闪，0为错误四闪，请查看参数是否合规");
}
//连接wifi函数
void linkserverFunction(){
  WiFi.begin(ssid,password);
  Serial.printf("\n正在连接至目标节点%s",ssid);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.printf(".");
    //双闪表示连接
    lighthelper(2,100);
    delay(500);
  }
  Serial.printf("IP地址为：");
  Serial.println(WiFi.localIP());
  Serial.printf("MAC地址为：%s\n",WiFi.softAPmacAddress().c_str());
}
//开启WIFI功能函数3
//APWIFI模式功能
class APWIFI{//默认情况下为隐藏式WIFI
  public :static void APopen(){//默认接入4台设备
    WiFi.softAP(ssid,password,1,1);//默认隐藏WIFI
  }
  public :static void APopen(int option){//设定option为0的时候为可搜索wifi
    WiFi.softAP(ssid,password,1,option);
  }
};
//注册网络设备接入连接事件处理程序1
//AP和STA模式切换函数//默认为STA模式
class switchTrans {
  //创建默认模式切换方法
  public :static void switchMode(){
    //默认开启STA模式
    WiFi.mode(WIFI_STA);
    Serial.printf("\nSTA模式\n目标WIFI名称：%s,密码：%s",ssid,password);
    ///////////////////////////////////////////////////连接wifi///////////////////////////////////////////////////////////
    linkserverFunction();
  }
  public :static void switchMode(int option){//如果option为0则为STA。 为1则为AP模式
    if(option == 0){
      WiFi.mode(WIFI_STA);
      Serial.println("\nSTA");
    }
    else if(option == 1){
      //开启AP的wifi信号//默认隐藏式网络
      APWIFI::APopen();
      WiFi.mode(WIFI_AP);
      Serial.println("\nAP(常规工作模式)");
    }
    else
      Serial.println("switchMode(option)_err，option传参错误\n0为STA，1为AP模式，请检查设置");
  }
  public :static void switchMode(int option,int optionWIFI){//如果传参为两个参数，则后面参数决定是否为隐藏式网络 optionWIFI为0是可搜索WIFI optionWIFI为1为隐藏式WIFI
    if(option == 0){
      Serial.println("\nswitchMode(option,optionWIFI)_err,option传参错误，在此函数下，模式参数必须为为1(开启AP模式)");
    }else if(option == 1){
      if(optionWIFI == 0){//开启可搜索WIFI模式
        APWIFI::APopen(optionWIFI);
        Serial.printf("\nWIFI可搜索模式已开启(调试模式)\nWIFI名称: %s 密码: %s",ssid,password);
      }else if(optionWIFI == 1){
        APWIFI::APopen(optionWIFI);
        Serial.printf("\n隐藏式网络(工作模式)\nWIFI名称: %s 密码: %s",ssid,password);
      }else
        Serial.println("switchMode(option,optionWIFI)_err,optionWIFI参数传参错误,1为隐藏式网络，0为可搜索网络，检查传参参数是否正确");
    }else{
      Serial.println("switchMode(option,optionWIFI)_err,option参数传参错误,option参数必须传参为1(开启AP模式)");
    }
  }
};
class UDPSR{
  public:
  static void Udpsend(IPAddress ipdes,String message){
    Udp.beginPacket(ipdes,822);//测试
    Udp.write(message.c_str());//将发送的message打包
    Udp.endPacket();
  }
  static bool Udpreceive(String AttestationMessage){//AttestationMessage为接受包内容是否与AttestationMessage相同
    int input,count;//count为计时器变量
    while(input != sizeof(AttestationMessage) - 1){//接收时限10秒
      input = Udp.parsePacket();//持续检测接受包，并查看信息字符串大小
      delay(1000);
      count++;
      if(count == 10){
        Serial.printf("Udpreceive_err接收超时！");
        return false;
      }
    }
    if(Udp.readString() == AttestationMessage){
      Serial.printf("Udpreceive接收到相应数据包");
      return true;
    }
  }
  static String Udpreceive(){//直接返回读取到的包内内容，类型为String类
    delay(1000);
    Udp.parsePacket();
    String transfer = Udp.readString();
    Serial.printf("读取到的内容为 %s",transfer);
    return transfer;
  }
};
//发送检测数据包.检测函数
int checkpakeage(){/////////////////////////err////////////////////////
  Udp.beginPacket("192.168.4.1",822);
  char  replyPacket[] = "Hi, this is esp8266\n";
  Udp.write(replyPacket);
  Udp.endPacket();//
  //进行回复检测
  int input = Udp.parsePacket();//解包
  if(Udp.readString() == "pass"){
    Serial.printf("连接成功！");
  }

}
unsigned int localUdpPort = 822;
//串口程序函数4
//目标设备丢失事件处理程序（需要通过MAC地址判断是否为目标设备）
int count = 0;//记录是否断开过wifi连接
void setup() {
  pinMode(D4,OUTPUT);
  digitalWrite(D4,HIGH);
  //WiFi.setAutoReconnect(false);
  //设定通信串口为9600
  Serial.begin(9600);
  STAconnect = WiFi.onStationModeConnected(connectHelper);//connectHelper为连接到wifi后的回调函数
  STAdiconnect = WiFi.onStationModeDisconnected(disconnectHelper);//disconnectHelper为断开WIFI后的回调函数
  //test
  //STA模式开启
  switchTrans::switchMode();
  //设置监听端口
  //开启UDP监听端口
  if(Udp.begin(localUdpPort)){
    Serial.println("UDP协议监听端口开启成功！");
    Serial.printf("监听端口为：%d,本机IP为：%s\n",localUdpPort,WiFi.localIP().toString().c_str());
  }else{
    Serial.printf("开启失败！");
  }
}
//client客户端判断server服务端发起连接测试请求函数，如果收到1，则返回2(UDP)
void respondCheckToserverUdp(){
  //解包判断是否UDP包内的字符是否为“1”
  if(Udp.readString() == "1"){
    Udp.beginPacket(Udp.remoteIP(),822);//回送目标设备
    Udp.write("2");
    Udp.endPacket();//
    Serial.printf("\n已发送");
  }
}

//UDP测试收发包函数client端
void checklinkUDPclient(){
  if(Numconnect == 0){
    delay(200);
    if(Numcount == 1){//缓冲，在断链后的一瞬间系统并不能读取MAC地址，进而导致死循环
      delay(2000);
    }
    int input;
    while(input != 4){//如果一直接收不到相应字节的就进入循环
      input = Udp.parsePacket();//
      ipsend = Udp.remoteIP();
      Udp.beginPacket(Udp.remoteIP(),822);
      Udp.write(WiFi.macAddress().c_str());
      Udp.endPacket();//
    }
    if(Udp.readString() = "pass"){
      if(Numcount == 0){//针对不同情况下的输出语句
        Serial.printf("UDP协议连接成功！");
        Numcount = 1;
      }else {
        Serial.printf("UDP协议重连成功！");
      }
      Numconnect++;
    }
  }
}
//字符转换成相应的数字
int transferChar(int input){
  return input - 48;
}
//接收震动执行命令包类
class recive{
  public:
  static void reciveToserverMove(){//接收目标服务端发送的震动数据包
    String transfer = Udp.readString();
    int input = transferChar(transfer[1]);//字符串转换，用于控制相应顺序马达
    Serial.printf("\n%d\n",input);
    //UDPSR::Udpsend(Udp.remoteIP(),(String)input);
    lightLEDinf(1);
  }
};
//通过服务端的数据包的字节长度大小从而判断进入哪一个模块
void selectMode(){
  if(Numconnect == 1){//为正常连接模式，进入接收数据包指令当中
    int input = Udp.parsePacket();
    switch(input){
      case 1:
        //为接收到超时检测数据包
        respondCheckToserverUdp();
        break;
      case 2:
        //数据包字节长度为2,进入接收震动指令模式
        recive::reciveToserverMove();
        break;
    }
  }else{//Numconnect显示为目前为未认证状态，进入UDP认证模式
    checklinkUDPclient();
  }
}
void loop() {
  /////////////////////////////////////UDP测试互发包//////////////////////////
  ///////客户端发送MAC码，如果mac码服务端识别成功，服务端返回pass整体握手成功////
  //Numconnect = checklinkUDPclient(&Numcount,Numconnect);
  selectMode();
  ///////客户端发送MAC码，如果mac码服务端识别成功，服务端返回pass整体握手成功//
  /////////////////////////////////////UDP测试互发包//////////////////////////
}
 ////////////////////////////////////测试发送客户端IP地址////////////////////
  //client.printf("A");
  //if(count == 0){
  //  WiFi.begin(ssid,password);
  //  client.println(WiFi.localIP());
  //  count++;
  //}
  //读取指令
  //transfer ="";
  //client.printf(".");
//STA连接wifi回调函数
void connectHelper(const WiFiEventStationModeConnected &event){
  Serial.printf("\n\n已连接至 %s ！\n",ssid);
  digitalWrite(D4,HIGH);
}
//STA断开wifi回调函数
void disconnectHelper(const WiFiEventStationModeDisconnected &event){
  //报告状况
  Serial.printf("\nWIFI目前已断开！\n正在重新连接\n正在连接到%s",ssid);
  digitalWrite(D4,LOW);
  Numconnect = 0;//恢复设置，重新开始计数
  //linkserverFunction();
}
/*
client.connect(host,port);
  Serial.printf("\n正在TCP连接IP：");
  Serial.println(host);
  if (!client.connect(host,port)){
    Serial.println("Connection failed");
  }
  //test
  //switchTrans::switchMode(1,1);
  //test
  
  //lightLEDinf(1);
  //delay(10000);
  //lightLEDinf(0);
  //lightLEDinf(3);
  WiFi.begin(ssid,password);
  client.println(WiFi.localIP());
}
//报告IP函数
void IPfunction(){
  Serial.printf("\n目前ip地址为： ");
  Serial.println(WiFi.localIP());
  */