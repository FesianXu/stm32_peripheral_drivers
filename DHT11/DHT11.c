/*******************************************************************
*编写人：FesianXu
*时间：2015/7
*晶振：8MHz
*MCU：STM32F103ZET6
*功能：驱动DHT11数据采集，实现温湿度采集
*DHT11 的供电电压为 3---5.5V。传感器上电后，要等待 1s 以越过不稳定状态在此期
间无需发送任何指令。电源引脚（ VDD,GND） 之间可增加一个 100nf 的电容，用以去藕
滤波。
*DATA 用于微处理器与 DHT11 之间的通讯和同步，采用单总线数据格式，一次通讯
时间 4ms 左右，数据分小数部分和整数部分，具体格式在下面说明，当前小数部分用
于以后扩展，现读出为零。操作流程如下：
一次完整的数据传输为 40 位，高位先出。数据格式为： 8 位湿度整数数据+8 位湿
度小数数据+8 位温度整数数据+8 位温度小数数据+8 位校验和。
数据传送正确时校验和数据等于“ 8 位湿度整数数据 +8 位湿度小数数据+8 位温度整数
数据 +8 位温度小数数据” 所得结果的末 8 位。
*注意，这坑爹货只能精确到整数位，也就是说读出来的小数位都是0，卧槽

*API接口函数：
(1)int ReadData(void)
返回值：int
传入参数：void
功能：读取数据，并将数据赋给全局变量DataBuf[]
//////////////////////////技术参数//////////////////////////////////
技术参数
供电电压： 3.3~5.5V DC
输 出： 单总线数字信号
测量范围： 湿度20-90%RH， 温度0~50℃
测量精度： 湿度+-5%RH， 温度+-2℃
分 辨 率： 湿度1%RH， 温度1℃
互 换 性： 可完全互换 ，
长期稳定性： <±1%RH/年
///////////////////////////////////////////////////////////////////


*******************************************************************/
#include "DHT11.h"
#include "delay.h"

uchar DataBuf[5]={0,0,0,0,0};
/*******************************************************************
*设置为PA7输出
*******************************************************************/
void OutputInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}
/*******************************************************************
*设置为PA7输入
*******************************************************************/
void InputInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_IN_FLOATING ; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}
/*******************************************************************
*主机发出数据采集信号
*用户 MCU 发送一次开始信号后， DHT11 从低功耗模式转换到高速模式，等待主机
开始信号结束后， DHT11 发送响应信号，发出 40 位的数据，并触发一次信号采集，用
户可选择读取部分数据，从模式下， DHT11 接受到开始信号触发一次温湿度采集，如果
没有接受到主机发送开始信号， DHT11 不会主动进行温湿度采集，采集数据后转换到低
速模式。
*总线空闲状态为高电平，主机把总线拉低等待 DHT11 响应，主机把总线拉低必须
大于 18ms，保证 DHT11 能检测到起始信号。 DHT11 接受到主机的开始信号后，等待主
机开始信号结束，然后发送 80us 低电平响应的信号。主机发送开始信号结束后，延时
等待 20~40us 后，读取 DHT11 的响应信号，主机发送开始信号后，可以切换到输入模
式，或者输出高电平均可，总线由上拉电阻拉高。
*******************************************************************/
void BeginSign()
{

	COMMAND=HIGH;
	COMMAND=LOW;
	WAIT_MS(20);
	COMMAND=HIGH;
	WAIT_US(40);
}


/*******************************************************************
*读取总线的数据，以一个字节为单位。
*①、DHT11以低电平应答主机，然后拉高总线准备输出。输出0信号和1
信号都是以低电平开始高电平结束。 
②、DHT11输出0、1信号的低电平时间相同，而高电平的时间不同，输出
0信号时高电平约26-28us,而当输出1信号时高电平约为70us。 
*******************************************************************/
uchar ReadByte()
{
	uchar dat=0x00;
	u8 i=0;
	for(i=0;i<8;i++)
	{
		while(!DATA);
		WAIT_US(35);
		if(DATA==HIGH)
		{
			dat=dat|0x01;
			while(DATA);
		}
		else
		{
			dat=dat|0x00;
		}
			dat=dat<<1;
	}
	return dat;
}
/*******************************************************************
*读取数据，DataBuf[0]是湿度整数数据，DataBuf[1]是湿度小数数据，DataBuf[2]是温度整数数据，DataBuf[3]是温度小数数据。 
*读取成功返回TRUE(1)，读取失败返回FALSE(-1)
*******************************************************************/
int ReadData(void)
{
	uchar i=0;
	OutputInit();
	BeginSign();
	InputInit();
	WAIT_US(10);
	if(!DATA)//此时总线已经被从机拉低
	{
		while(!DATA);
		while(DATA);	
		for(i=0;i<5;i++)
		{
			DataBuf[i]=ReadByte();
			DATA=LOW;
		}

	}
	else
	{
		return FALSE;
	}
	while(!DATA);
	OutputInit();
	COMMAND=HIGH;
	if((DataBuf[0]+DataBuf[1]+DataBuf[2]+DataBuf[3])==DataBuf[4])
	{
		return TRUE;
	}
		
}

