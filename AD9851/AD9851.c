/************************************************************************
*��д�ˣ�FesianXu
*ʱ�䣺2015/7
*����8MHz
*MCU��STM32F103ZET6
*���ܣ�����AD9851
*API�ӿں�����
(1) IO_Init()����ʼ��IO��
(2) AD9851_Init();����IO�ڣ���ʼ��ģ��
(3) Reset()����ʼ��ģ��
(4) ChangeParToSer();�ı�Ϊ����ͨ��
(5) Send40bits(u16 FreqNeed,u8 REFCLK,u8 Power,u8 Phase);
Freq:Ƶ��
REFCLK:�Ƿ���6��Ƶ REFCLK_ENABLE(1) REFCLK_DISABLE(0)
Power:�Ƿ�Power down  POWER_UP(1)   POWER_DOWN(0)
Phase:��λ���ơ�
************************************************************************/

#include "AD9851.h"
#include "sys.h"
#include "delay.h"

/********************************************************************
ע�����
FQUP�������������������ݷ���40bit�Ĵ����õģ������ݵ�ֵû��֪��֮ǰ��һ�����ܽ�FQUP��Ϊ�����ء�
********************************************************************/

//PF3 FQUP��PF5 REST,PF7 WCLK,PF9 DATA
void IO_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //PF3
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//�������
	GPIO_Init(GPIOF, &GPIO_InitStructure); //��ʼ��PF3
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PF5
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//�������
	GPIO_Init(GPIOF, &GPIO_InitStructure); //��ʼ��PF5
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; //PF7
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//�������
	GPIO_Init(GPIOF, &GPIO_InitStructure); //��ʼ��PF7
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PF9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//�������
	GPIO_Init(GPIOF, &GPIO_InitStructure); //��ʼ��PF9
}

void AD9851_Init()
{
	IO_Init();
	WCLK=0;
	FQUP=0;
}

/**************************************************************************
Results of Reset, Figure 14
�C Phase accumulator zeroed such that the output = 0 Hz (dc)
�C Phase offset register set to 0 such that DAC IOUT = full-scale
output and IOUTB = zero mA output
�C Internal programming address pointer reset to W0
�C Power-down bit reset to 0 (power-down disabled)
�C 40-bit data input register is NOT cleared
�C 6�� reference clock multiplier is disabled
�C Parallel programming mode selected by default
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

//��Ĭ�ϵĲ���ͨ�ŷ�ʽ��Ϊ����ͨ��
//��Ҫ�������ϵ����ϽǵĶ�·ñ����
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

//ǰ32λ��Ƶ�ʴ�С���Ժ���Ϊ��λ
//REFCLK,��1ʹ��6*REFCLK ��0��ʧ��6*REFCLK
//W33 always is 0
//Power ,Power-up 0 ,Power-down 1
void Send40bits(u32 FreqNeed,u8 REFCLK,u8 Power,u8 Phase)
{
	u8 i=0;
	u32 Freq=(u32)((FreqNeed+1)/0.04191);//0.04191
	FQUP=LOW;
	WCLK=LOW;
	//��Ƶ��
	for(i=0;i<32;i++)
	{
		DATA=(u8)(Freq&0x01);
		WCLK=HIGH;
		__WAIT;
		WCLK=LOW;
		__WAIT;
		Freq=Freq>>1;
	}
		
	//��6*REFCLK
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
	
	//��Power down or not��Ϣ
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
	//����λ��Ϣ
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

//������תΪ���ں��������͸�ȫ0��40bit�����ȶ�DATA��
void StableDATA()
{
	u8 i=0;
	FQUP=LOW;
	WCLK=LOW;
	//��Ƶ��
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



