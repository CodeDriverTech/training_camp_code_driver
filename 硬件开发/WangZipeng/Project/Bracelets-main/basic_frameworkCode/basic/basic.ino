#include <ESP8266WiFi.h>
#include<stdio.h>
#include <WiFiUdp.h>
#define ssid "Texthome"//目标名称
#define password "WZP8460121"//密码
/*
测试文件2024/3.23/10:27
**********************************************************************************
dreamsky.0822.wuming
服务端调试测试am
**********************************************************************************
用于测试开发板的WIFI调试程序以及基础的框架程序测试
*/
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
///////////////////////////////////////////////////////封装UDP发送接收包机制Test///////////////////////////////
//发送接收代码UDP包
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
};
////////////////////////////////////////////////////////封装UDP发送接收包机制Test/////////////////////////////
/*
//UDP协议开启以及查看测试连接类
class UDPclass{
  public: static int openUPD(){//开启UDP
    WiFiUDP Udp;//实例化UDP对象
    //设置监听端口
    unsigned int localUdpPort = 822;
    //开启UDP监听端口
    if(Udp.begin(localUdpPort)){
      Serial.println("UDP协议监听端口开启成功！");
      Serial.printf("监听端口为：%d,本机IP为：%s\n",localUdpPort,WiFi.localIP().toString().c_str());
    }else{
      Serial.printf("开启失败！");
    }
  }
  public: static bool lanchUDPpackage(String IPAdress,unsigned int port,String massage){
    WiFiUDP Udp;//实例化UDP对象
    Udp.beginPacket(IPAdress,port);//配置目的地
    Udp.write(massage);//写入udp数据包
    if(Udp.endPacket()){//发送数据包
      Serial.printf("\n发送成功");
    }else{
      Serial.printf("\n发送失败");
    }
  }
};
*/
//串口程序函数4
//目标设备丢失事件处理程序（需要通过MAC地址判断是否为目标设备）
//UDP测试收发包函数server端
int checklinkUDPserver(int Numcount){//Numconut用于记录设备是否为断连后的状态，如果为断连后的状态，则开始测试重连
  if(Numcount == 0){
    int input;
    while(input != 17){//等待数据包发送到目标设备
      input = Udp.parsePacket();
    }
    if(Udp.readString() == "84:CC:A8:9E:E4:C8"){
      Udp.beginPacket("192.168.4.2",822);
      char  replyPacket[] = "pass";
      Udp.write(replyPacket);
      Udp.endPacket();//
      Numcount++;
      Serial.printf("\nUDP连接成功！");
    }
  }
  return Numcount;
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
  //影响设备整体操控(无参)
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
      if(count == 8){//在10秒内如果没有回应，直接退出并且识别为设备断开
        Numdisconnect = 0;//标识设备已断连
        Serial.printf("\n设备连接超时！");
        return;
      }
    }
    if(Udp.readString() == "2"){
      Serial.printf("\n测试成功，目前状态为连接\n目标ip地址为");
      Serial.println(Udp.remoteIP());
      Numdisconnect = 1;
   }else{
      Serial.printf("\n与目标值不符，断连");
      Numdisconnect = 0;//标识设备已断连
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
    UDPSR::Udpsend(ipsend,(String)option);
    if(UDPSR::Udpreceive((String)option)){
      //发送成功
      Serial.printf("发送成功");
      return true;
    }
  }else{
    Serial.printf("sendToclientMoveHelper(int option)_err传参错误！\n option的传参应在[0-7]，请检查传入参数");
    return false;//发送错误
  }
}
//传输震动指令核心代码块函数
bool sendToclientMove(int option){
  //检测目标设备是否存在
  if(Check::checklinkToclientUdp(1)){
    //进行发送震动命令
    if(sendToclientMoveHelper(1)){//震动第二颗马达
      return true;
    }else{
      return false;
    }
  }else{
    Serial.printf("检测到设备未连接，发送指令错误！");
    return false;
  }
}
///////////////////////////////发送震动指令Test////////////////////////////////////////////////////////////////
void setup() {
  pinMode(D4,OUTPUT);
  digitalWrite(D4,HIGH);
  //设定通信串口为115200
  Serial.begin(115200);
  STAconnect = WiFi.onStationModeConnected(connectHelper);//connectHelper为连接到wifi后的回调函数
  STAdiconnect = WiFi.onStationModeDisconnected(disconnectHelper);//disconnectHelper为断开WIFI后的回调函数
}
//报告IP函数
void IPfunction(){
  Serial.printf("\n目前ip地址为： ");
  Serial.println(WiFi.localIP());
}
void loop() {
}
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