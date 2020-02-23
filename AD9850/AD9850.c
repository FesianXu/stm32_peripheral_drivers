/************************************************************************
*��д�ˣ�FesianXu
*ʱ�䣺2015/7
*����8MHz
*MCU��STM32F103ZET6
*���ܣ�����AD9850
*API�ӿں�����
(1) IO_Init()����ʼ��IO��
(2) AD9850_Init();����IO�ڣ���ʼ��ģ��
(3) Reset()����ʼ��ģ��
(4) ChangeParToSer();�ı�Ϊ����ͨ��
(5) SendAD9850(u16 FreqNeed,u8 REFCLK,u8 Power,u8 Phase);
Freq:Ƶ��
REFCLK:�Ƿ���6��Ƶ REFCLK_ENABLE(1) REFCLK_DISABLE(0)
Power:�Ƿ�Power down  POWER_UP(1)   POWER_DOWN(0)
Phase:��λ���ơ�
************************************************************************/

#include "AD9850.h"
#include "sys.h"
#include "delay.h"

/********************************************************************
ע�����
FQUP_9850�������������������ݷ���40bit�Ĵ����õģ������ݵ�ֵû��֪��֮ǰ��һ�����ܽ�FQUP_9850��Ϊ�����ء�
********************************************************************/

//PF3 FQUP_9850��PF5 REST_9850,PF7 WCLK_9850,PF9 DATA_9850
void IO_AD9850_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_6|GPIO_Pin_8; //PG2 PG4 PG6 PG8
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//�������
	GPIO_Init(GPIOG, &GPIO_InitStructure); //��ʼ��PG2 PG4 PG6 PG8
		
}

void AD9850_Init()
{
		IO_AD9850_Init();
		WCLK_9850=0;
		FQUP_9850=0;
}

/**************************************************************************
Results of Reset, Figure 14
�C Phase accumulator zeroed such that the output = 0 Hz (dc)
�C Phase offset register set to 0 such that DAC IOUT = full-scale
output and IOUTB = zero mA output
�C Internal programming address pointer reset to W0
�C Power-down bit reset to 0 (power-down disabled)
�C 40-bit DATA_9850 input register is NOT cleared
�C 6�� reference clock multiplier is disabled
�C Parallel programming mode selected by default
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

//��Ĭ�ϵĲ���ͨ�ŷ�ʽ��Ϊ����ͨ��
//��Ҫ�������ϵ����ϽǵĶ�·ñ����
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

//ǰ32λ��Ƶ�ʴ�С���Ժ���Ϊ��λ
//REFCLK,��1ʹ��6*REFCLK ��0��ʧ��6*REFCLK
//W33 always is 0
//Power ,Power-up 0 ,Power-down 1
void SendAD9850(u32 FreqNeed,u8 Power,u8 Phase)
{
	u8 i=0;
	u32 Freq=(u32)((FreqNeed+1)/0.029104);//0.04191
	FQUP_9850=LOW;
	WCLK_9850=LOW;
	//��Ƶ��
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
		
		//��Power down or not��Ϣ
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
		
		
	//����λ��Ϣ
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

//������תΪ���ں��������͸�ȫ0��40bit�����ȶ�DATA_9850��
void StableDATA_9850()
{
	u8 i=0;
	FQUP_9850=LOW;
	WCLK_9850=LOW;
		//��Ƶ��
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



