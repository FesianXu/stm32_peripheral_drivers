#ifndef __DHT11_H
#define __DHT11_H

#define uchar unsigned char
#define uint unsigned int
#define HIGH 1
#define LOW 0
#define COMMAND PAout(7)
#define DATA PAin(7)
//这里使用宏定义去定义延时是为了在实时系统中用阻塞态去描述，而不是用普通的延时函数
#define WAIT_US(x) (delay_us(x))
#define WAIT_MS(x) (delay_ms(x))
#define TRUE 1;
#define FALSE -1;


int ReadData(void);



#endif
