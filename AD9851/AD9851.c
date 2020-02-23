/************************************************************************
*编写人：FesianXu
*时间：2015/7
*晶振：8MHz
*MCU：STM32F103ZET6
*功能：驱动AD9851
*API接口函数：
(1) IO_Init()；初始化IO口
(2) AD9851_Init();配置IO口，初始化模块
(3) Reset()；初始化模块
(4) ChangeParToSer();改变为串口通信
(5) Send40bits(u16 FreqNeed,u8 REFCLK,u8 Power,u8 Phase);
Freq:频率
REFCLK:是否开启6倍频 REFCLK_ENABLE(1) REFCLK_DISABLE(0)
Power:是否Power down  POWER_UP(1)   POWER_DOWN(0)
Phase:相位控制。
************************************************************************/

#include "AD9851.h"
#include "sys.h"
#include "delay.h"

/********************************************************************
注意事项：
FQUP的上升沿是用来把数据发给40bit寄存器用的，在数据的值没有知道之前，一定不能将FQUP置为上升沿。
********************************************************************/

//PF3 FQUP，PF5 REST,PF7 WCLK,PF9 DATA
void IO_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //PF3
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//推挽输出
	GPIO_Init(GPIOF, &GPIO_InitStructure); //初始化PF3
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PF5
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//推挽输出
	GPIO_Init(GPIOF, &GPIO_InitStructure); //初始化PF5
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; //PF7
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//推挽输出
	GPIO_Init(GPIOF, &GPIO_InitStructure); //初始化PF7
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PF9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//推挽输出
	GPIO_Init(GPIOF, &GPIO_InitStructure); //初始化PF9
}

void AD9851_Init()
{
	IO_Init();
	WCLK=0;
	FQUP=0;
}

/**************************************************************************
Results of Reset, Figure 14
– Phase accumulator zeroed such that the output = 0 Hz (dc)
– Phase offset register set to 0 such that DAC IOUT = full-scale
output and IOUTB = zero mA output
– Internal programming address pointer reset to W0
– Power-down bit reset to 0 (power-down disabled)
– 40-bit data input register is NOT cleared
– 6 reference clock multiplier is disabled
– Parallel programming mode selected by default
**************************************************************************/
void Reset()
{
	REST=LOW;	
	REST=HIGH;
	__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;//need 5 sysclk,i use 7 sysclk
	REST=LOW;
	__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;//need 2 sysclk,i use 3 sysclk
	FQUP=LOW;
}

//将默认的并口通信方式变为串口通信
//需要将板子上的左上角的短路帽短上
void ChangeParToSer()
{
	WCLK=LOW;
	FQUP=LOW;
	WCLK=HIGH;
	__WAIT;
	WCLK=LOW;
	FQUP=HIGH;
	__WAIT;
	FQUP=LOW;
}

//前32位是频率大小，以赫兹为单位
//REFCLK,置1使能6*REFCLK 置0，失能6*REFCLK
//W33 always is 0
//Power ,Power-up 0 ,Power-down 1
void Send40bits(u32 FreqNeed,u8 REFCLK,u8 Power,u8 Phase)
{
	u8 i=0;
	u32 Freq=(u32)((FreqNeed+1)/0.04191);//0.04191
	FQUP=LOW;
	WCLK=LOW;
	//传频率
	for(i=0;i<32;i++)
	{
		DATA=(u8)(Freq&0x01);
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
		Freq=Freq>>1;
	}
		
	//传6*REFCLK
	if(REFCLK==REFCLK_ENABLE)
	{
		DATA=HIGH;
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
	}
	else if(REFCLK==REFCLK_DISABLE)
	{
		DATA=LOW;
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
	}
		
	//W33 always 0
	DATA=LOW;
	WCLK=HIGH;
	__WAIT;
	WCLK=LOW;
	__WAIT;
	
	//传Power down or not信息
	if(Power==POWER_UP)
	{
		DATA=LOW;
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
	}
	else if(Power==POWER_DOWN)
	{
		DATA=HIGH;
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
	}
	FQUP=LOW;
	//传相位信息
	for(i=0;i<5;i++)
	{
		DATA=(u8)(Freq&0x00000001);
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
		Freq=Freq>>1;
	}
	WCLK=LOW;
	FQUP=HIGH;
	__WAIT;
	FQUP=LOW;
}

//将并口转为串口后，立即发送个全0的40bit，以稳定DATA口
void StableDATA()
{
	u8 i=0;
	FQUP=LOW;
	WCLK=LOW;
	//传频率
	for(i=0;i<40;i++)
	{
		DATA=LOW;
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
	}
	FQUP=HIGH;
	__WAIT;
	FQUP=LOW;
}



