# 基于STM32微控制器的一些外设设备驱动程序

从以前的硬盘里面找到了大二时候初学stm32单片机时候写的一些硬件的驱动代码，写得不好，但是能work，就挂在这里看对别人有没有帮助吧。哈哈哈，以前的代码写得好差呀。文件介绍：



1. `AD9850`和`AD9851`都是DDS芯片，用于打波的。
2. `at_cmd`里面的是和`esp8266`芯片的AT指令模式有关的代码。
3. `DHT11`里面是温度芯片`DHT11`的驱动
4. `SYSTEM`里面提供了延时，串口，系统配置等一些库函数
5. `datasheets`里面是一些datasheet的芯片文档