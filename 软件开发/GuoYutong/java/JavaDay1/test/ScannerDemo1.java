package JavaDay1.test;

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
