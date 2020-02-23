/************************************************************************
*编写人：FesianXu
*时间：2015/7
*晶振：8MHz
*MCU：STM32F103ZET6
*功能：驱动AD9850
*API接口函数：
(1) IO_Init()；初始化IO口
(2) AD9850_Init();配置IO口，初始化模块
(3) Reset()；初始化模块
(4) ChangeParToSer();改变为串口通信
(5) SendAD9850(u16 FreqNeed,u8 REFCLK,u8 Power,u8 Phase);
Freq:频率
REFCLK:是否开启6倍频 REFCLK_ENABLE(1) REFCLK_DISABLE(0)
Power:是否Power down  POWER_UP(1)   POWER_DOWN(0)
Phase:相位控制。
************************************************************************/

#include "AD9850.h"
#include "sys.h"
#include "delay.h"

/********************************************************************
注意事项：
FQUP_9850的上升沿是用来把数据发给40bit寄存器用的，在数据的值没有知道之前，一定不能将FQUP_9850置为上升沿。
********************************************************************/

//PF3 FQUP_9850，PF5 REST_9850,PF7 WCLK_9850,PF9 DATA_9850
void IO_AD9850_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_6|GPIO_Pin_8; //PG2 PG4 PG6 PG8
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//推挽输出
	GPIO_Init(GPIOG, &GPIO_InitStructure); //初始化PG2 PG4 PG6 PG8
		
}

void AD9850_Init()
{
		IO_AD9850_Init();
		WCLK_9850=0;
		FQUP_9850=0;
}

/**************************************************************************
Results of Reset, Figure 14
– Phase accumulator zeroed such that the output = 0 Hz (dc)
– Phase offset register set to 0 such that DAC IOUT = full-scale
output and IOUTB = zero mA output
– Internal programming address pointer reset to W0
– Power-down bit reset to 0 (power-down disabled)
– 40-bit DATA_9850 input register is NOT cleared
– 6 reference clock multiplier is disabled
– Parallel programming mode selected by default
**************************************************************************/
void Reset_9850()
{
		REST_9850=LOW;
		REST_9850=HIGH;
		__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;//need 5 sysclk,i use 7 sysclk
		REST_9850=LOW;
		__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;//need 2 sysclk,i use 3 sysclk
		FQUP_9850=LOW;
}

//将默认的并口通信方式变为串口通信
//需要将板子上的左上角的短路帽短上
void ChangeParToSer_9850()
{
	WCLK_9850=LOW;
	FQUP_9850=LOW;
	WCLK_9850=HIGH;
	__WAIT;
	WCLK_9850=LOW;
	FQUP_9850=HIGH;
	__WAIT;
	FQUP_9850=LOW;
}

//前32位是频率大小，以赫兹为单位
//REFCLK,置1使能6*REFCLK 置0，失能6*REFCLK
//W33 always is 0
//Power ,Power-up 0 ,Power-down 1
void SendAD9850(u32 FreqNeed,u8 Power,u8 Phase)
{
	u8 i=0;
	u32 Freq=(u32)((FreqNeed+1)/0.029104);//0.04191
	FQUP_9850=LOW;
	WCLK_9850=LOW;
	//传频率
	for(i=0;i<32;i++)
	{
		DATA_9850=(u8)(Freq&0x01);
		WCLK_9850=HIGH;
		__WAIT;
		WCLK_9850=LOW;
		__WAIT;
		Freq=Freq>>1;
	}
		
	//W32 always 0
	DATA_9850=LOW;
	WCLK_9850=HIGH;
	__WAIT;
	WCLK_9850=LOW;
	__WAIT;
	
	//W33 always 0
	DATA_9850=LOW;
	WCLK_9850=HIGH;
	__WAIT;
	WCLK_9850=LOW;
	__WAIT;
		
		//传Power down or not信息
	if(Power==POWER_UP)
	{
		DATA_9850=LOW;
		WCLK_9850=HIGH;
		__WAIT;
		WCLK_9850=LOW;
		__WAIT;
	}
	else if(Power==POWER_DOWN)
	{
		DATA_9850=HIGH;
		WCLK_9850=HIGH;
		__WAIT;
		WCLK_9850=LOW;
		__WAIT;
	}
	FQUP_9850=LOW;
		
		
	//传相位信息
	for(i=0;i<5;i++)
	{
		DATA_9850=(u8)(Freq&0x00000001);
		WCLK_9850=HIGH;
		__WAIT;
		WCLK_9850=LOW;
		__WAIT;
		Freq=Freq>>1;
	}
	WCLK_9850=LOW;
	FQUP_9850=HIGH;
	__WAIT;
	FQUP_9850=LOW;
		
		
}

//将并口转为串口后，立即发送个全0的40bit，以稳定DATA_9850口
void StableDATA_9850()
{
	u8 i=0;
	FQUP_9850=LOW;
	WCLK_9850=LOW;
		//传频率
	for(i=0;i<40;i++)
	{
		DATA_9850=LOW;
		WCLK_9850=HIGH;
		__WAIT;
		WCLK_9850=LOW;
		__WAIT;
	}
	FQUP_9850=HIGH;
	__WAIT;
	FQUP_9850=LOW;
}



