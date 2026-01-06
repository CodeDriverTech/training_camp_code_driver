# **Day 1**

---
## 1.helloworld程序案例


```java{.line-numbers}
public class helloworld{
    //class：表示定义一个类。类是java项目中最基本的组成单元
    //HelloWorld：类名，类名必须与文件名一致
    //{}：类的范围，所有代码都要放在{}内

    static void main(){
        //表示java项目的主入口，程序开始运行时从主入口开始向下运行
        //是一个固定格式

        System.out.println("Hello World");
        //文本输出Hello World
//        单行注释快捷键ctrl + /
        /*多行注释
        * ctrl + shift + /*/
        printHelloWorld();
    }
    public static void printHelloWorld(){System.out.println("helloworld 2");}
}
```


## 2.字面量

### 1.字面量的分类

|字面量类型|说明|举例|
|:-:|:-:|:-:|
|整数|不带小数点|66,-88|
|小数|带小数点|3.14,5.20|
|字符串|双引号括起来的内容|"hello"|
|字符|单引号括起来的内容，只有一个|'a','0'|
|布尔|布尔值，表示真假|只有两个值：true,false|
|空|一个特殊的值，空值|null|   




**示例**
```java{.line-numbers}
public class ValueDemo1{
    public static void main (String [] args){
        //整数
        System.out.println(66);
        System.out.println(-88);
        //小数
        System.out.println(3.14);
        //字符串
        System.out.println("Hello");
        //字符
        System.out.println('a');
        System.out.println('我');//支持中文字符
        //布尔
        System.out.println(true);
        System.out.println(false);
        //空值,不能直接打印出来，只能以字符串形式打印出null
        System.out.println("null");
    }
}
```
*拓展：*
- 特殊字符：\n,\t,\r ...
  - \t(制表符) : 在打印的时候，把前面字符串的长度补齐到8，或者8的整数倍。最少补1个空格，最多补8个空格  // *可在打印表格数据时用于对齐*
```java
public class ValueDemo2{
    public static void main (String [] args){
        System.out.println("name" + '\t' + "age");
        System.out.println("tom" + '\t' + "23");
    }
}
```

## 3.变量

### 1.变量的定义格式
    数据类型 变量名 = 值
### 2.用法
    变量可重新赋值，但不可重复定义
    运算
### 3.使用场景
    某个值经常改变
    经常重复使用某个值

```java{.line-numbers}
// 语法：数据类型 变量名 = 值;
int age = 25;           // 整数柜子，存年龄
double price = 19.99;   // 小数柜子，存价格
String name = "张三";   // 文字柜子，存姓名
boolean isStudent = true; // 开关柜子，真假值

int a = 10;
System.out.println(a);//10
a = 50;
System.out.println(a);//50

int b = 7;
int c = 3;
System.out.println(b + c);//10
```

## 4.基本数据类型
|基本数据类型|关键字|取值范围|内存占用|
|:-:|:-:|:-:|:-:|
|整数|byte|-128~127|1|
|^|short|-32768~327672|2|
|^|int(默认)|-2147483648~2147483647（10位数）|4|
|^|long|-9223372036854775808~9223372036854775807（19位数）|8|
|浮点数|float|-3.401298e-38到3.402823e+38|4|
|^|double(默认)|-4.9000000e-324到1.797693e+308|8|
|字符|char|0-65535|2|
|布尔|boolean|true, false|1|

```java{.line-numbers}
//long
//如果要定义long类型的变量
//在数据值的后面需要加一个L作为后缀
//L可以是大写的，也可以是小写的。
//建议：使用大写。
long n = 99999999999L;
System.out.println(n);
//float
//注意点：定义float类型变量的时候
//数据值也需要加一个F作为后缀
float f = 10.1F;
System. out. println(f);
//double
double d = 20.2;
System. out. println(d);
```
## 5.标识符
    1.标识符：就是给类，方法，变量等起的名字
    2.标识符命名规则---硬性要求
       · 由数字、字母、下划线(_)和美元符($)组成
       · 不能以数字开头
       · 不能是关键字
       · 区分大小写
    3.标识符命名规则-软性建议
       · 小驼峰命名法：方法、变量
            规范1：标识符是一个单词的时候，全部小写
                范例1: name
            规范2：标识符由多个单词组成的时候，第一个单词首字母小写，其他单词首字母大写
                范例2: firstName
    4.大驼峰命名法：类名
            规范1：标识符是一个单词的时候，首字母大写
                范例1: Student
            规范2：标识符由多个单词组成的时候，每个单词的首字母大写
                范例2: GoodStudent 

## 6.键盘录入
- Java帮我们写好一个类叫Scanner，这个类就可以接收键盘输入的数字。

    - 步骤一：导包---Scanner这个类在哪
    - 步骤二：创建对象---表示我要开始用Scanner这个类了
    - 步骤三：接收数据---真正开始干活了
```java{.ine-numbers}
import java.util.Scanner;//导包的动作必须出现在类定义的上边。   
Scanner sc = new Scanner(System.in);//上面这个格式里面，只有sc是变量名，可以变，其他的都不允许变
int i = sc.nextInt();//左面这个格式里面，只有i是变量名，可以变，其他的都不允许变。
```
**示例**
```java{.line-numbers}
import java.util.Scanner;
//1.导包，找到scanner这个类在哪
//书写注意：要写在类定义的上面

public class ScannerDemo1{//类定义
    public static void main (String [] args){
        //2.创建对象，表示我现在准备用scanner这个类
        Scanner sc = new Scanner(System.in);

        System.out.println("请输入整数：");
        //3.接收数据
        //变量i此时记录了键盘输入的数据
        int i = sc.nextInt();
        System.out.println(i);
    }
}
```
## 7.I-IDEA项目结构
    project (项目)
        module (模块)
            package (包)
                class (类)

## 8.运算符
    1.运算符
        对字面量或者变量进行操作的符号
    2.表达式
        用运算符把字面量或者变量连接起来，符合java语法的式子就可以称为表达式。不同运算符连接的表达式体现的是不同类型的表达式。
### 1.算术运算符
|符号|作用|
|:-:|:-:|
|+|加|
|-|减|
|*|乘|
|/|除|
|%|取余，取模|
```java{.line-numbers}
package JavaDay1.test;

public class ArithmeticoperatorDemo1 {
    public static void main(String[] args) {
        //+
        System. out. println(3+2);//5
        //-
        System. out. println(5-1);//4
        //*
        System. out. println(7*9);//63
        //如果在计算的时候有小数参与
        //结论：
        //在代码中，如果有小数参与计算，结果有可能不精确的。
        //为什么呢？
        //暂时只要知道这个结论就可以了。
        //具体的原因，我们到了JavaSE的时候，会再详细的讲解。
        System. out. println(1.1+1.1);//2.2
        System. out. println(1.1+1.01);//2.110000000000003
        System. out. println(1.1-1.01);
        System. out. println(1.1*1.01);
}
```
**注意事项：** */和%的区别：两个数据做除法，/取结果的商，%取结果的余数。整数操作只能得到整数，要想得到小数，必须有浮点数参与运算。*
#### 1>数据类型转换
    数字进行运算时，数据类型不一样不能运算，需要转成一样的，才能运算
##### I.隐式转换(自动类型提升)
    ·取值范围小的-->取值范围大的
    ·隐式转换的两种提升规则
        1.取值范围小的，和取值范围大的进行运算，小的会先提升为大的，再进行运算
        2.byte short char三种类型的数据在运算的时候，都会直接先提升为int，然后再进行运算    
##### II.强制转换
    ·取值范围大的-->取值范围小的
    ·如果把一个取值范围大的数值，赋值给取值范围小的变量，是不允许直接赋值的。如果一定要这么做就需要加入强制转换
**格式：** 目标数据类型	变量名 =(目标数据类型)被强转的数据
```java
double a = 12.3
int b = (int) a;
```
**示例**
```java{.line-numbers}
package JavaDay1.test;

public class ArithmeticoperatorDemo3{
    public static void main(String[] args){
        byte b1 = 10;
        byte b2 = 20;
        //现在我们要强转的是谁？
        //b1 + b2计算之后的结果。
        // (byte)b1 + b2 强转的是b1，并不是最终的结果
        byte result = (byte)(b1 + b2);
        System. out. println(result); //30
    }
}
```
#### 2>字符串相加
    ·当“+”操作中出现字符串时，这个“+”是字符串连接符，而不是算术运算符了。
     会将前后的数据进行拼接并产生一个新的字符串。
    ·连续进行"+"操作时，从左到右逐个执行。
`System. out. println(3.7 + " abc");//"3.7abc"`   
`System. out. println(" abc" + true); //" abctrue"`

#### 3>字符相加 （**使用ASCII码**）
    byte short char三种类型的数据在运算的时候，都会直接先提升为int，然后再进行运算

### 2.自增自减运算符
|符号|作用|说明|
|:-:|:-:|:-:|
|++|加|变量的值加1|
|-|减|变量的值减1|

*++和--无论是放在变量的前边还是后边，单独写一行结果是一样的。*
### 3.赋值运算符
|符号|作用|说明|
|:-:|:-:|:-:|
|=|赋值|inta=10，，将10赋值给变量a|
|+=|加后赋值|a+=b，将a+b的值给a|
|-=|减后赋值|a-=b，将a-b的值给a|
|*=|乘后赋值|a ^ { * } = b ，将a×b的值给a|
|/=|除后赋值|a/=b，将a÷b的商给a|
|%=|取余后赋值|a%=b，将a÷b的余数给a|
### 4.关系运算符
|符号|说明|
|:-:|:-:|
|==|a==b，判断a和b的值是否相等，成立为true，不成立为false|
|！=|a!=b，判	断a和b的值是否不相等，成立为true，不成立为false|
|a>b|判断a是否大于b,成立为true,不成立为false|
|>=|a>=b，判断a是否大于等于b，成立为true，不成立为false|
|<|a<b,判断a是否小于b,成立为true,不成立为false|
|<=|a<=b,判断a是否小于等于b，成立为true，不成立为false|

**注意事项：** *关系运算符的结果都是boolean类型，要么是true,要么是false.千万不要把“==”误写成“=”。*
### 5.逻辑运算符
符号|作用|说明
:-:|:-:|:-:
&|逻辑与（且）|并且，两边都为真，结果才是真
\||逻辑或|或者，两边都为假，结果才是假
`^|逻辑异或|相同为false, 不同为true
！|逻辑非|取反
```java{.line-numbers}
package JavaDay1.test;

public class LogicoperatorDemo1{
    public static void main(String[] args){
        //1. & 并且
        //两边都为真，结果才是真
        System. out. println(true & true);//true
        System. out. println(false & false);//false
        System. out. println(true & false);//false
        System. out. println(false & true);//false
        //2. | 或者
        //两边都为假，结果才是假
        System. out. println(true | true);//true
        System. out. println(false | false);//false
        System. out. println(true | false);//true
        System. out. println(false | true);//true
        //^ 异或
        //相同为false, 不同为true
        //了解一下即可
        System. out. println(true ^ true);//false
        System. out. println(false ^ false);//false
        System. out. println(true ^ false);//true
        System. out. println(false ^ true);//true
        //! 逻辑非 取反
        //提示：
        //取反的感叹号不要写多次，要么不写，要么只写一次
        System. out. println(!false);//true
        System. out. println(!true);//false
    }
}
```
符号|作用|说明
:-:|:-:|:-:
&&|短路与|结果和&相同，但是有短路效果
\|\||短路或|结果和\|相同，但是有短路效果

**注意事项：**
- &|：无论左边true false，右边都要执行。
- &&|：如果左边能确定整个表达式的结果，右边不执行。
- &&：左边为false，右边不管是真是假，整个表达式的结果一定是false。
- ||：左边为true，右边不管是真是假，整个表达式的结果一定是true。
### 6.三元运算符
    格式：关系表达式？表达式1：表达式2;
范例：求两个数的较大值。
```java
int max = a > b ? a : b;// 把三元运算符的结果赋值给一个变量
System. out. println(a > b ? a : b); //把三元运算符的结果直接打印
```
    计算规则
    首先计算关系表达式的值
    如果值为true，表达式1的值就是运算结果
    如果值为false，表达式2的值就是运算结果
### 7.运算符优先级
    ()优先于所有