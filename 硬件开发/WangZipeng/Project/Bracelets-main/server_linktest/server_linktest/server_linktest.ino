#include <ESP8266WiFi.h>
#include<stdio.h>
#include <Pinger.h>
#include <WiFiUdp.h>
#include <espnow.h>

#define ssid "Testhome"//目标名称
#define password "WZP8460121"//密码
/*
测试文件2024/3.23/10:27
**********************************************************************************
dreamsky.0822.wuming
服务端调试测试am
**********************************************************************************
用于测试开发板的WIFI调试程序以及基础的框架程序测试
*/
WiFiUDP Udp;//实例化UDP对象
//记录设备连接状况
IPAddress ipsend;//发送方的IP地址，用于认证和回传目的
int Numdisconnect = 0;//客户端设备掉线标识 为0时表示断连，1时表示连接正常
int Numcount = 0;//记录设备连接情况 为0则表示无客户端连接

////////////////////////////////////////////Test///////////////////
//建立服务端TCP监听接口
WiFiServer server(80);//监听接口为80
//////////////////////////////////////////////Test/////////////////
//注册WIFI连接成功事件处理程序
WiFiEventHandler STAconnect;
//注册WIFI连接断开事件处理程序
WiFiEventHandler STAdiconnect;
//服务端相应防火墙/AP点配置
//IPAddress local_IP(192,168,2,1);//本身的IP地址
//IPAddress getway(192,168,2,1);
//IPAddress subnet(225,225,225,0);
//灯光闪烁函数(闪一次)
void lighthelper(int Num,int Sce){//Num为次数,sce为间隔时间（毫秒）
  for(int i = 0 ; i< Num ; i++){
    digitalWrite(D4,LOW);
    delay(Sce);
    digitalWrite(D4,HIGH);
    delay(Sce);
  }
}
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
}
//开启WIFI功能函数3
//APWIFI模式功能
class APWIFI{//默认情况下为隐藏式WIFI
  public :static void APopen(){//默认接入4台设备
    //WiFi.softAPConfig(local_IP, getway, subnet);
    WiFi.softAP(ssid,password,1,1);//默认隐藏WIFI
  }
  public :static void APopen(int option){//设定option为0的时候为可搜索wifi
    //WiFi.softAPConfig(local_IP, getway, subnet);
    WiFi.softAP(ssid,password,1,option);
  }
};
//注册网络设备接入连接事件处理程序1
WiFiEventHandler APlinkfunction;
//注册有连接点断开的处理程序
WiFiEventHandler stationDisconnectedHandler;
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
///////////////////////////////////////////////////////封装UDP发送接收包机制Test///////////////////////////////
//发送接收代码UDP包
class UDPSR{
  public:
  static void Udpsend(IPAddress ipdes,String message){
    Udp.beginPacket(ipdes,822);//测试
    Udp.write(message.c_str());//将发送的message打包
    Udp.endPacket();
  }
  //查看是否符合目标内容方法//专为传入单个数字所使用的方法//待修复
  static bool Udpreceive(String AttestationMessage){//AttestationMessage为接受包内容是否与AttestationMessage相同
    int input,count;//count为计时器变量
    while(input != 1){//接收时限10秒
      input = Udp.parsePacket();//持续检测接受包，并查看信息字符串大小
      delay(1000);
      count++;
      if(count == 6){
        Serial.printf("Udpreceive_err接收超时！");
        return false;
      }
    }
    if(Udp.readString() == AttestationMessage){
      Serial.printf("Udpreceive接收到相应数据包");
      return true;
    }
  }
};
////////////////////////////////////////////////////////封装UDP发送接收包机制Test/////////////////////////////
//串口程序函数4
//目标设备丢失事件处理程序（需要通过MAC地址判断是否为目标设备）
void setup() {
  pinMode(D4,OUTPUT);
  digitalWrite(D4,HIGH);
  //设定通信串口为115200
  Serial.begin(115200);
  STAconnect = WiFi.onStationModeConnected(connectHelper);//connectHelper为连接到wifi后的回调函数
  STAdiconnect = WiFi.onStationModeDisconnected(disconnectHelper);//disconnectHelper为断开WIFI后的回调函数
  stationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(onStationDisconnected);//客户端断开连接处理程序
  //APlinkfunction = WiFi.onSoftAPModeStationConnected(APlinkHelper);//APlinkHelper为有新的客户端接入后的调用函数
  //test
  //STA模式开启
  switchTrans::switchMode(1,1);
  //开启TCP服务器
  //server.begin();
  //Serial.println("\nserver_start");
  //检测端口
  //Serial.printf("端口为：%d\n",80);
  //Serial.println(WiFi.softAPIP());
  //test
  //switchTrans::switchMode(1,1);
  //test
  //Serial.println("等待设备加入");
  //设置监听端口
  unsigned int localUdpPort = 822;
  //开启UDP监听端口
  if(Udp.begin(localUdpPort)){
    Serial.println("UDP协议监听端口开启成功！");
    Serial.printf("监听端口为：%d,本机IP为：%s\n",localUdpPort,WiFi.softAPIP().toString().c_str());
  }else{
    Serial.printf("开启失败！");
  }
  //检测机制
  //检测是否为相应mac地址的设备加入
}
//UDP测试收发包函数server端
//记录发送方的ip地址
void checklinkUDPserver(){
  if(Numcount == 0){
    int input;
    while(input != 17){//等待数据包发送到目标设备
      input = Udp.parsePacket();
    }
    ///////////////////MAC写入内存，用于后续配对环节////////////////
    ipsend = Udp.remoteIP();//记录目标设备
    if(Udp.readString() == "84:CC:A8:9E:E4:C8"){
      Udp.beginPacket(ipsend,822);
      Udp.write("pass");
      Udp.endPacket();//
      Numcount = 1;
      Serial.printf("\nUDP连接成功！");
    }
  }
}
//UDP测试收发包函数client端
int checklinkUDPclient(int *Numcountinsde,int Numconnectinsde){
  if(Numconnectinsde == 0){
    if(*Numcountinsde == 1){//缓冲，在断链后的一瞬间系统并不能读取MAC地址，进而导致死循环
      delay(2000);
    }
    int input;
    while(input != 4){//如果一直接收不到相应字节的就进入循环
      input = Udp.parsePacket();//接收相应数据包
      Udp.beginPacket("192.168.4.1",822);
      Udp.write(WiFi.macAddress().c_str());
      Udp.endPacket();//
    }
    if(Udp.readString() = "pass"){
      if(*Numcountinsde == 0){//针对不同情况下的输出语句
        Serial.printf("UDP协议连接成功！");
        *Numcountinsde = 1;
      }else {
        Serial.printf("UDP协议重连成功！");
      }
      Numconnectinsde++;
    }
  }
  return Numconnectinsde;
}

//测试目标客户端是否在线类
class Check{
  //影响设备整体进程(无参)
  public: static void checklinkToclientUdp(){//发送1如果收到2则表示连接测试成功10秒内(UDP)
    //发送到已配对目标客户端IPsend
    Udp.beginPacket(ipsend,822);
    Udp.write("1");
    Udp.endPacket();//发送1数据包
    int input;
    int count = 0;//记录循环次数1000毫秒为单位
    while(input != 1){
      input = Udp.parsePacket();
      delay(1000);
      count++;
      if(count == 10){//在10秒内如果没有回应，直接退出并且识别为设备断开
        Numcount = 0;//标记设备丢失
        Serial.printf("\n设备连接超时！");
        return;
      }
    }
    if(Udp.readString() == "2"){
      Serial.printf("\n测试成功，目前状态为连接\n目标ip地址为");
      Serial.println(Udp.remoteIP());
   }else{
      Serial.printf("\n与目标值不符，断连");
      Numcount = 0;//标记设备丢失
   }  
  }
  ///////////////////////////////UDP超时检测包封装类Test///////////////////////////////
  public: static int checklinkToclientUdp(int Mode){//Mode用于调用带有返回类型的check方法
    //发送到已配对目标客户端IPsend
    Udp.beginPacket(ipsend,822);
    Udp.write("1");
    Udp.endPacket();//发送1数据包
    int input;
    int count = 0;//记录循环次数1000毫秒为单位
    while(input != 1){
      input = Udp.parsePacket();
      delay(1000);
      count++;
      if(count == 8){//在10秒内如果没有回应，直接退出并且识别为设备断开
        return false;//返回false代表已断开连接
      }
    }
    if(Udp.readString() == "2"){
      Serial.println(Udp.remoteIP());
      return true;//true代表正常连接
   }else{
      return false;
   } 
  }
};
/////////////////////////////////////UDP超时检测包封装类Test//////////////////////////////////////////
//发送震动指令，查看是否发送成功函数/////////////////////Test////////////////////////
bool sendToclientMoveHelper(int option){//option为从手腕正上方顺时针所指向的8个震动马达，代表所指向的8个方位
  //测试代码
  if(option <= 7 && option >= 0){//由0-7定义
    //进入发送指令环节
    UDPSR::Udpsend(ipsend,"0"+(String)option);
    //Serial.printf("1");
    /*
    if(UDPSR::Udpreceive((String)option)){
      //发送成功
      Serial.printf("发送成功");
      return true;
    }
    */
    return true;
  }else{
    Serial.printf("sendToclientMoveHelper(int option)_err传参错误！\n option的传参应在[0-7]，请检查传入参数");
    return false;//发送错误
  }
}
//传输震动指令核心代码块函数
bool sendToclientMove(int option){
  sendToclientMoveHelper(option);
  /*
  //检测目标设备是否存在
  if(Check::checklinkToclientUdp(1)){
    //进行发送震动命令
    if(){//震动第二颗马达
      return true;
    }else{
      return false;
      Serial.printf("\nsendToclientMove_err");
    }
  }else{
    Serial.printf("检测到设备未连接，发送指令错误！");
    Numcount = 0;//标记设备丢失
    return false;
  }
  */
}
///////////////////////////////发送震动指令Test////////////////////////////////////////////////////////////////
//记录设备连接状况
int Seccount = 0;
void loop() {
  /*
  if(Serial.available()){           // 当串口接收到信息后 
    newB = Serial.read();    // 将接收到的信息使用read读取
    newB = newB - 48;
  }
  */
  checklinkUDPserver();
  //nano板串口测试程序
  while (Serial.available()){           // 当串口接收到信息后 
    int serialData = Serial.read();    // 将接收到的信息使用read读取
    Serial.printf("%d",serialData - 48);
    sendToclientMove(serialData - 48);
  }
  //checklinkUDPserver();//UDP认证包
  //发送震动指令
  
  //checklinkToclient();
  //////////////////////与客户端进行TCP握手连接////////////////////////////////////////
  //WiFiClient client = server.available();//监听客户端连接
  //判断用户有没有进行输入
  //判断设备是否断开连接
    /*
    if(client.available()){
      if(client.read() != '.'){
        clientData = client.readStringUntil('\n');
        Serial.printf("\n连接成功！此设备的IP地址为：");
        Serial.println(clientData);
      }
    }
    */
  //}
  /////////////////////////////////////UDP测试互发包//////////////////////////
  
  /////////////////////////////////////UDP测试互发包//////////////////////////
}
/*
    while (WiFi.softAPgetStationNum() != 0) {
      if (client.available()) {
        // 读取数据
        if(Numcount == 0){
          String clientData;
          clientData = client.readStringUntil('\n');
          Serial.printf("\n连接成功！此设备的IP地址为：");
          Serial.println(clientData);
          Numcount++;
        }
        char option = client.read();
        if(option == 'A' || option == 'V'){
          lighthelper(2,300);
        }
        Serial.println(option);
      }
    } 
    if(1 != WiFi.softAPgetStationNum()){
      Numcount == 0;
      Serial.printf("\n设备断开连接！\n正在重连");
      while(1 != WiFi.softAPgetStationNum()){
        Serial.printf(".");
        delay(300);
      }
      Serial.println("重连成功！");
    }
*/
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
  //linkserverFunction();
}

void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt){
  //链接丢失提醒
  Serial.printf("\n目标链接丢失！");
  Numcount = 0;
}

//ap模式有新的设备加入
/*
void APlinkHelper(const WiFiEventSoftAPModeStationConnected& event){
  //报告状况
  Serial.printf("\n有设备加入！，设备的的IP地址为 ");
  Serial.println(WiFi.softAPIP());
}
*/

