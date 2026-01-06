package JavaDay1.test;

import com.sun.jdi.PathSearchingVirtualMachine;

public class HelloWorld {
    //class：表示定义一个类。类是java项目中最基本的组成单元
    //HelloWorld：类名，类名必须与文件名一致
    //{}：类的范围，所有代码都要放在{}内

    static void main() {
        //表示java项目的主入口，程序开始运行时从主入口开始向下运行
        //是一个固定格式

        System.out.println("Hello World");
        printHelloWorld();
        //文本输出Hello World
//        单行注释快捷键ctrl + /
        /*多行注释
        * ctrl + shift + /*/
    }

    public static void printHelloWorld(){
        System.out.println("hello world");
    }
}
