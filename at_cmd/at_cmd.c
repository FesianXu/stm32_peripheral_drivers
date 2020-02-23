#include "at_cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include "c_interfaces.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "delay.h"
#include <string.h>

self_info local_info ;//本机信息。
u8 num_of_APs ; //AP数量。
wifi_ap APs[30] ; //ap信息

extern stdio usart1 ; //在c_interfaces.c中定义
extern char Rx1Buf[Rx1Length] ; //在USART.c中定义
extern u16 Rx1Flag ; //在USART.c中定义

/**
* @brief 初始化AT指令，主要是初始化串口接口
* @retVal 1 初始化成功
* @retVal 0 初始失败，重复初始化
*/
u8 initAT_CMD()
{
	initInterfaces(1, 115200) ;
	if(usart1.status != 0)
	{
		usart1.USART_Init(115200) ;
		return 1 ;
	}
	else
		return 0 ;
}

/**
* @brief 测试ESP8266的AT指令模式是否正常
* @retVal 1 返回正常
* @retVal 0 超时退出
* @cmd AT
*/
u8 testESP8266(void)
{
	char tmp[10] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[0].at_cmdLine, at_tail) ;
	usart1.sendStr(tmp) ;
	#if	RREERTOS_USE == 1
		vTaskDelay(20/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(20) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else 
		return 0 ;
}

/**
* @brief  搜索WiFi AP
* @retVal u8 1 搜索完毕
* @retVal u8 0 搜索超时，退出
* @cmd AT+CWLAP
* @attetion 此处的有对内容的限制，后续可以优化,
* 搜索的是以信号强度阀值为判据（暂时未实现）
* 也可以用链表储存(未实现)
* @version 0.1
* 
*/
u8 searchWiFi_AP(signed char threshold)
{
	char tmp[20] = {0} ;
	char *ok_ptr = null ;
	char *ptr_tmp = null ;
	char tmp_for_rssi[9] = {0} ;
	char tmp_mac[20] = {0} ;
	char tmp_for_channel[6] = {0} ;
	int i = -1 ;
	u8 j = 0 ;
	u8 z = 0 ;
	u8 w = 0 ;
	u8 y = 0 ;
	u8 counter_for_point = 0 ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT,at_head,at_cmd_table[5].at_cmdLine, at_tail);
	usart1.sendStr(tmp) ;
	//////////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(5000/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(3000) ;
		delay_ms(2000) ;
	#endif
	ok_ptr = strstr(Rx1Buf, ok_pattern) ;
	ptr_tmp = Rx1Buf ;
	if( ok_ptr != NULL ) // got the temp
	{
		taskENTER_CRITICAL() ; //进入临界区
		while( strstr(ptr_tmp, ok_pattern) != null )
		{
	    ptr_tmp = strstr(ptr_tmp, at_cmd_table[5].at_cmdLine) ;
			while( *ptr_tmp != '\n')
			{
				if( *ptr_tmp == '(')
					APs[i].ecn = atoi((ptr_tmp+1)) ; // got ecn
				if( *ptr_tmp == '"')
					counter_for_point++ ;
				if( counter_for_point == 1 )
				{
					APs[i].ssid[j] = *(ptr_tmp+1) ; 
					j++ ;
				}
				if( counter_for_point == 2)
				{
					APs[i].ssid[j-1] = '\0' ;
					j = 0 ;
					// printf3("ssid %d= %s\r\n", i,APs[i].ssid);
					tmp_for_rssi[z] = *(ptr_tmp+2) ;
					z++ ;
				} // got ssid
				if( counter_for_point == 3)
				{
					for(j = 8; j > 2; j--)
						tmp_for_rssi[j] = 0 ;
					APs[i].rssi = atoi(tmp_for_rssi) ; 
//					if(APs[i].rssi < threshold)
//					{
//						continue ;
//					}
					// printf3("rssi %d = %d\r\n",i, APs[i].rssi);
					tmp_mac[w] = *(ptr_tmp+1) ;
					w++ ;
				} // got rssi
				if(counter_for_point == 4)
				{
					tmp_mac[w-1] = 0 ;
					w = 0 ;
					strMAC2numMAC(tmp_mac, APs[i].MAC) ;
					// printf3("mac %d = %s \r\n",i,tmp_mac);
					if( *ptr_tmp != ')')
					{
						tmp_for_channel[y] = *(ptr_tmp+2) ;
						y++ ;
					}
					else if( *ptr_tmp == ')')
					{
						if(strlen(tmp_for_channel) == 5)
							for(j = 6; j > 2; j--)
								tmp_for_channel[j-1] = 0 ;
						else
							for(j = 6; j > 1; j--)
								tmp_for_channel[j-1] = 0 ;
						APs[i].channels = atoi(tmp_for_channel) ;
						// printf3("channel %d = %d \r\n",i, APs[i].channels) ;
					}
				} //got mac
				ptr_tmp++ ;
			};
			i++ ;
			j = 0 ;
			z = 0 ;
			w = 0 ;
			y = 0 ;
			counter_for_point = 0 ;
		}
		taskEXIT_CRITICAL() ; //退出临界区
		num_of_APs = i-1 ;
		return 1 ;
	}
	else
	{
		return 0 ;
	}
		
}

/**
* @brief 复位esp8266模块
* @retVal 1 ok_pattern收到
* @retVal 2 reset_pattern收到
* @retVal 0 复位未成功，超时退出
* @cmd AT+RST
*/
u8 resetESP8266(void)
{
	char tmp[15] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[1].at_cmdLine, at_tail) ;
	usart1.sendStr(tmp) ;
	#if	RREERTOS_USE == 1
		vTaskDelay(3000/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(3000) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if ( strstr(Rx1Buf, reset_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 加入一个AP
* @retVal 1 加入AP成功
* @retVal 0 加入AP失败，超时退出
* @retVal 2 加入AP失败，提示ERROR
* @params char *ssid AP的ssid
* @params char *passwd AP的密码
* @cmd AT+CWJAP
*/
u8 joinAP(char *ssid, char *passwd)
{
	char tmp[50] = {0} ;
	char tmp_params[20] ;
	cleanUSARTBuffer() ;
	sprintf(tmp_params, "\"%s\",\"%s\"", ssid, passwd) ;
	at_cmd_table[4].at_params = tmp_params ;
	sprintf(tmp, AT_FORMAT_PARAMS_STR, at_head, at_cmd_table[4].at_cmdLine, tmp_params,at_tail) ;
	usart1.sendStr(tmp);
	///////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(7000/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(2000) ;
		delay_ms(2000) ;
		delay_ms(2000) ;
		delay_ms(1000) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}


/**
* @brief 选择wifi的应用模式
* @retVal 1 设置成功
* @retVal 2 设置失败，ERROR提示
* @retVal 0 设置失败，超时退出
* @params u8 mode 1 Station模式
									2 AP模式
									3 AP&Station模式
* @cmd AT+CWMODE
*/
u8 setWiFiMode(u8 mode)
{
	char tmp[20] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_INT, at_head,at_cmd_table[3].at_cmdLine, mode, at_tail) ;
	usart1.sendStr(tmp) ;
	#if	RREERTOS_USE == 1
		vTaskDelay(20/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(20) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 退出与AP的连接
* @retVal 1 退出成功
* @retVal 0 退出失败，超时
* @cmd AT+CWQAP
*/
u8 quitAP(void)
{
	char tmp[15] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[6].at_cmdLine,at_tail) ;
	usart1.sendStr(tmp) ;
	#if	RREERTOS_USE == 1
		vTaskDelay(20/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(20) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else 
		return 0 ;
}

/**
* @brief 查看已经接入的设备的IP
* @retVal 1 查看成功
* @retVal 0 查看失败
* @params u8 *ips 接入设备的ip地址指针
* @cmd AT+CWLIF
*/
u8 searchDevicesIP(u8 *ips)
{
	char tmp[15] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[8].at_cmdLine, at_tail) ;
	usart1.sendStr(tmp) ;
}

/**
* @brief 获取本地的IP地址
* @retVal 1 获取成功,AP模式
* @retVal 2 获取成功，STA模式
* @retVal 3 获取成功，AP&STA模式
* @retVal 0 获取失败。
* @params self_info 传入类型
* @cmd AT+CIFSR
* @attention 在AP模式下无效
*/
u8 getLocalIP(self_info *local_info)
{
	char tmp[15] = {0} ;
	char apip[] = "APIP" ;
	char apmac[] = "APMAC" ;
	char staip[] = "STAIP" ;
	char stamac[] = "STAMAC" ;
	char iptmp[20] ;
	char mactmp[20] ;
	char *pStrTmp = NULL ;
	u8 flag = 0 ;
	u8 counter = 0 ;
	u8 loop = 0 ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[13].at_cmdLine, at_tail) ;
	usart1.sendStr(tmp) ;
	////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	
	pStrTmp = strstr(Rx1Buf, apip) ;
	if( pStrTmp != NULL) //说明有apip,apmac
	{
		while( *pStrTmp != '\n')
		{
			if( *pStrTmp == '"')
				counter++ ;
			if( counter == 1)
			{
				iptmp[loop] = *(pStrTmp+1) ;
				loop++ ;
			}
			pStrTmp++ ;
		}
		iptmp[loop-1] = '\0' ;
		loop = 0 ;
		counter = 0 ;
		// printf3("apip = %s\r\n", iptmp) ;
		strIP2numIP(iptmp, local_info->APip) ;
		
		pStrTmp = strstr(Rx1Buf, apmac) ;
		while( *pStrTmp != '\n')
		{
			if( *pStrTmp == '"')
				counter++ ;
			if( counter == 1)
			{
				mactmp[loop] = *(pStrTmp+1) ;
				loop++ ;
			}
			pStrTmp++ ;
		}
		mactmp[loop-1] = '\0' ;
		loop = 0 ;
		counter = 0 ;
		// printf3("apmac = %s\r\n", mactmp) ;
		strMAC2numMAC(mactmp, local_info->APmac) ;
		flag = 1 ; //ap ip&mac got flag 
	}
	pStrTmp = NULL ;
	pStrTmp = strstr(Rx1Buf, staip) ;
	if( pStrTmp != NULL) //说明有staip, stamac
	{
		if( flag == 1)
			flag = 3 ;
		else 
			flag = 2 ;
		
		while( *pStrTmp != '\n')
		{
			if( *pStrTmp == '"')
				counter++ ;
			if( counter == 1)
			{
				iptmp[loop] = *(pStrTmp+1) ;
				loop++ ;
			}
			pStrTmp++ ;
		}
		iptmp[loop-1] = '\0' ;
		loop = 0 ;
		counter = 0 ;
		// printf3("staip = %s\r\n", iptmp) ;
		strIP2numIP(iptmp, local_info->STAip) ;
		
		pStrTmp = strstr(Rx1Buf, stamac) ;
		while( *pStrTmp != '\n')
		{
			if( *pStrTmp == '"')
				counter++ ;
			if( counter == 1)
			{
				mactmp[loop] = *(pStrTmp+1) ;
				loop++ ;
			}
			pStrTmp++ ;
		}
		mactmp[loop-1] = '\0' ;
		loop = 0 ;
		counter = 0 ;
		// printf3("stamac = %s\r\n", mactmp) ;
		strMAC2numMAC(mactmp, local_info->STAmac) ;
	}
	return flag ;
}

/**
* @brief 建立TCP连接
* @retVal 1 连接成功
* @retVal 0 连接错误
* @retVal 2 连接已经存在
* @cmd AT+CIPSTART
* @attention 用于多路连接之时，既是CIPMUC = 1
*/
u8 createTCP_inMultiple(u8 id, char *addr, int port)
{
	char tmp[50] = {0} ;
	char type[] = "TCP" ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_TCP_IN_MULTI, at_head, at_cmd_table[10].at_cmdLine, id, type, addr, port, at_tail) ;
	usart1.sendStr(tmp) ;
}

/**
* @brief 建立TCP连接
* @retVal 1 连接成功
* @retVal 0 连接错误
* @retVal 2 连接已经存在
* @cmd AT+CIPSTART
* @attention 用于单路连接之时，既是CIPMUC = 0
*/
u8 createTCP_inSingle(char *addr, int port)
{
	char tmp[50] = {0} ;
	char type[] = "TCP" ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_TCP_IN_SINGLE, at_head, at_cmd_table[10].at_cmdLine, type, addr, port, at_tail) ;
	usart1.sendStr(tmp) ;
	////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, connected_pattern) != NULL)
		return 2 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL)
		return 0 ;
}

/**
* @brief 关闭多路连接TCP
* @retVal 1 关闭成功
* @retVal 0 关闭超时失败
* @retVal 2 在单路连接中，没有该连接，提示ERROR失败
* @retVal 3 在多路连接中，没有该id连接，提示link is not失败
* @cmd AT+CIPCLOSE
*/
u8 closeTCP_inMultiple(u8 id)
{
	char tmp[20] = {0} ;
	char link_not_pattern[] = "link is not\r\n" ; 
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_INT, at_head, at_cmd_table[12].at_cmdLine, id, at_tail) ;
	usart1.sendStr(tmp) ;
	/////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, link_not_pattern) != NULL)
		return 3 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL)
		return 2 ;
	else
		return 0 ;
}

/**
* @brief 关闭单路连接TCP
* @retVal 1 关闭成功
* @retVal 0 关闭超时失败
* @retVal 2 在单路连接中，没有该连接，提示ERROR失败
* @retVal 3 在多路连接中，没有该id连接，提示link is not失败
* @cmd AT+CIPCLOSE
*/
u8 closeTCP_inSingle(void)
{
	char tmp[20] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[12].at_cmdLine, at_tail) ;
	usart1.sendStr(tmp) ;
	/////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	////////////判断是否接收到ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL)
		return 2 ;
	else
		return 0 ;
}

/**
* @brief 启动多连接
* @retVal 1 启动多连接成功
* @retVal 2	启动单连接成功
* @retVal 3 启动多连接失败
* @retVal 4 启动单连接失败
* @retVal 5 已经处于连接状态
* @retVal 0 未知错误
* @params u8 mode 1 多路连接
									0 单路连接
* @cmd AT+CIPMUX
* @attention 只有当连接都断开后才能更改，如果开启过 server 需要重启模块
*/
u8 setMultipleConnection(u8 mode)
{
	char tmp[20] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_INT, at_head, at_cmd_table[14].at_cmdLine, mode, at_tail) ;
	usart1.sendStr(tmp) ;
	/////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, ok_pattern) != NULL && mode == 1)
		return 1 ;
	else if( strstr(Rx1Buf, ok_pattern) != NULL && mode == 0)
		return 2 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL && mode == 1)
		return 3 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL && mode == 0)
		return 4 ;
	else if( strstr(Rx1Buf, build_pattern) != NULL )
		return 5 ;
	else 
		return 0 ;
}

/**
* @brief 设置模块传输模式
* @retVal 1 设置成功
* @retVal 2 处于连接link is builded状态，设置失败
* @retVal 3 设置失败,提示ERROR
* @retVal 0 设置超时失败
* @params u8 mode 1 透传模式
									0 非透传模式
* @cmd AT+CIPMODE
* @attention 在透传连接因为+++退出之后，这个模式设置需要重新设置为
* CIPMODE=1
*/
u8 setTransmitMode(u8 mode)
{
	char tmp[20] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_INT, at_head, at_cmd_table[16].at_cmdLine, mode, at_tail) ;
	usart1.sendStr(tmp) ;
	//////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, build_pattern) != NULL )
		return 2 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 3 ;
	else 
		return 0 ;
}

/**
* @brief 设置服务器超时时间
* @retVal 1 设置成功
* @retVal 2 设置失败,提示ERROR
* @retVal 3 设置成功，没有改变，提示no change
* @retVal 0 设置超时失败
* @params unsigned int time 0-28800 服务器超时时间 单位s
* @cmd AT+CIPSTO
*/
u8 setTheOvertimeOfServer(unsigned int time)
{
	char tmp[20] = {0} ;
	char no_change_pattern[] = "no change\r\n" ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_INT, at_head, at_cmd_table[17].at_cmdLine, time, at_tail) ;
	usart1.sendStr(tmp) ;
	////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else if( strstr(Rx1Buf, no_change_pattern) != NULL )
		return 3 ;
	else 
		return 0 ;
}

/**
* @brief 配置为服务器
* @retVal 1 设置成功
* @retVal 2 设置失败,提示ERROR
* @retVal 0 设置超时失败
* @params u8 mode 0 关闭server模式
									1	开启server模式
					unsigned int port 端口号
* @cmd AT+CIPSERVER
* @attention 开启server后自动建立server监听，当有client接入时会自动按
* 顺序占用一个连接 AT+CIPMUX=1时才能开启服务器
*/
u8 setAsServer(u8 mode, unsigned int port)
{
	char tmp[20] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_SERVER, at_head, at_cmd_table[15].at_cmdLine, mode, port, at_tail) ;
	usart1.sendStr(tmp) ;
	////////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 设置AP模式下的参数
* @retVal 1 设置成功
* @retVal 2 设置失败,提示ERROR
* @retVal 0 设置超时失败
* @params char *ssid 字符串参数，接入点名称
					char *pwd	 字符串参数，密码最长为64Bytes的ASCII
					u8 	chl		通道号
					u8	ecn		加密方式
					//0 open
					//1 WEP
					//2 WPA_PSK
					//3 WPA2_PSK
					//4 WPA_WPA2_PSK
* @cmd AT+CWSAP
* @attention 指令只有在AP模式开启后有效
*/
u8 setParamsInAPMode(char *ssid, char *pwd, u8 chl, u8 ecn)
{
	char tmp[80] = {0} ;
	char params_tmp[60] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(params_tmp, "\"%s\",\"%s\",%d,%d", ssid, pwd, chl, ecn) ;
	sprintf(tmp, AT_FORMAT_PARAMS_STR, at_head, at_cmd_table[7].at_cmdLine, params_tmp, at_tail) ;
	usart1.sendStr(tmp) ;
	//////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 查看AT指令版本
* @retVal 1 查看成功
* @retVal 2 查看失败,提示ERROR
* @retVal 0 设置超时失败
* @params char *version 传入字符串参数，需要分配有效空间先,15个char
* @cmd AT+GMR
*/
u8 showATCmdVersion(char *version)
{
	char tmp[15] = {0} ;
	char version_pattern[] = "----version" ;
	char *ptr_str = null ;
	u8 flag = 0 ;
	char *version_tmp = version ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[2].at_cmdLine,at_tail) ;
	usart1.sendStr(tmp) ;
	/////////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, ok_pattern) != NULL )
	{
		ptr_str = strstr(Rx1Buf, version_pattern) ;
		while( *ptr_str != '\n')
		{
			if( *ptr_str == 'n')
				flag = 1 ;
			if(flag == 1)
			{
				*version_tmp = *(ptr_str+1) ;
				version_tmp++ ;
			}
			ptr_str++ ;
		}
		*(version_tmp-1) = 0 ;
		return 1 ;
	}
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 获得连接状态
* @retVal 1 查看成功
* @retVal 2 查看失败,提示ERROR
* @retVal 0 设置超时失败
* @params cipstatus *status
* @cmd AT+CIPSTATUS
*/
u8 getLinkStatus(cipstatus *status)
{
	char tmp[15] = {0} ;
	char *ptr_str = null ;
	char ip_tmp[20] = {0} ;
	char port_tmp[7] = {0} ;
	u8 i = 0 ;
	u8 j = 0 ;
	u8 counter = 0 ;
	u8 counter_for_comma = 0 ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[9].at_cmdLine, at_tail);
	usart1.sendStr(tmp) ;
	/////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, ok_pattern) != null)
	{
		ptr_str = strstr(Rx1Buf, "+CIPSTATUS:") ;
		while( *ptr_str != '\n' )
		{
			if( *ptr_str == ':')
			{
				status->id = atoi(ptr_str+1) ;
			}//get id
			if( *ptr_str == '"')
				counter++ ;
			if( *ptr_str == ',')
				counter_for_comma++ ;
			
			if( counter == 1)
			{
				if( *(ptr_str+1) == 'T' )
					status->connect_type = 0 ;
				else
					status->connect_type = 1 ;	
			}//get type
			if( counter == 3)
			{
				ip_tmp[i] = *(ptr_str+1) ;
				i++ ;
			} //get ip
			if( counter == 4)
			{
				ip_tmp[i-1] = 0 ;
				strIP2numIP(ip_tmp, status->IP) ;
			}
			if( counter_for_comma == 3 )
			{
				port_tmp[j] = *(ptr_str+1) ;
				j++ ;
			}
			if( counter_for_comma == 4)
			{
				port_tmp[j-1] = 0 ;
				status->port = atoi(port_tmp) ;
				status->tetype = atoi(ptr_str+1) ;
			}//get port and get tetype
			ptr_str++ ;
		}
		return 1 ;
	}
	else if( strstr(Rx1Buf, error_pattern) != null)
		return 2 ;
	else
		return 0 ;
}

/**
* @brief 在单路连接的情况下发送数据，非透传
* @retVal 1 发送成功
* @retVal 2 发送失败,提示ERROR
* @retVal 0 设置超时失败
* @params char *data 发送的数据，最大长度为2048字节
* @cmd AT+CIPSEND
* @attention 响应：收到此命令后先换行返回“>”，然后就进入了透传模式
*/
u8 SendDataInSingle_noTransparent(char *data)
{
	unsigned int len = strlen(data) ;
	char tmp[15] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT_PARAMS_INT, at_head, at_cmd_table[11].at_cmdLine, len, at_tail) ;
	usart1.sendStr(tmp);
	/////////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, rts_pattern) != null)
	{
		usart1.sendStr(data);
		return 1 ;
//		#if	RREERTOS_USE == 1
//			vTaskDelay(100/portTICK_RATE_MS) ;
//		#elif RREERTOS_USE == 0
//			delay_ms(100) ;
//		#endif
//		if( strstr(Rx1Buf, send_ok_pattern) != null) //发送完成
//		{
//			return 1 ;
//		}
//		else 
//			return 0 ;
	}
	else if( strstr(Rx1Buf, error_pattern) != null)
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 在多路连接的情况下发送数据，非透传
* @retVal 1 发送成功
* @retVal 2 发送失败,提示ERROR
* @retVal 0 设置超时失败
* @params char *data 发送的数据，最大长度为2048字节
					u8 id 需要用于传输的id号
* @cmd AT+CIPSEND
* @attention 响应：收到此命令后先换行返回“>”，然后就进入了透传模式
*/
u8 SendDataInMultiple_noTransparent(char *data, u8 id)
{
	unsigned int len = strlen(data) ;
	char tmp[25] = {0} ;
	char tmp_params[10] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp_params, "%d,%d", id, len);
	sprintf(tmp, AT_FORMAT_PARAMS_STR, at_head, at_cmd_table[11].at_cmdLine, tmp_params, at_tail) ;
	usart1.sendStr(tmp);
	/////////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, rts_pattern) != null)
	{
		usart1.sendStr(data);
		return 1 ;
//		#if	RREERTOS_USE == 1
//			vTaskDelay(100/portTICK_RATE_MS) ;
//		#elif RREERTOS_USE == 0
//			delay_ms(100) ;
//		#endif
//		if( strstr(Rx1Buf, send_ok_pattern) != null) //发送完成
//		{
//			return 1 ;
//		}
//		else 
//			return 0 ;
	}
	else if( strstr(Rx1Buf, error_pattern) != null)
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 透传发数据
* @retVal 1 发送成功
* @retVal 2 发送失败,提示ERROR
* @retVal 0 设置超时失败
* @params char *data 发送的数据，最大长度为2048字节
* @cmd AT+CIPSEND
* @attention 响应：收到此命令后先换行返回“>”，然后就进入了透传模式
* @attention 在透传连接因为+++退出之后，这个模式设置需要重新设置为
* CIPMODE=1
*/
u8 SendData_Transparent(char *data)
{
	char tmp[25] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, AT_FORMAT, at_head, at_cmd_table[11].at_cmdLine, at_tail) ;
	usart1.sendStr(tmp);
	/////////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( strstr(Rx1Buf, rts_pattern) != null)
	{
		usart1.sendStr(data);
		return 1 ;
//		#if	RREERTOS_USE == 1
//			vTaskDelay(100/portTICK_RATE_MS) ;
//		#elif RREERTOS_USE == 0
//			delay_ms(100) ;
//		#endif
//		if( strstr(Rx1Buf, send_ok_pattern) != null) //发送完成
//		{
//			return 1 ;
//		}
//		else 
//			return 0 ;
	}
	else if( strstr(Rx1Buf, error_pattern) != null)
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief 退出透传模式
* @retVal 1 退出成功
* @retVal 2 退出失败,提示ERROR
* @retVal 0 退出超时失败
* @cmd +++
*/
u8 quitFromTransparent(void)
{
	char tmp[10] = {0} ;
	cleanUSARTBuffer() ;
	sprintf(tmp, "%s", at_cmd_table[18].at_cmdLine) ;
	usart1.sendStr(tmp) ;
	////////////////////////////////////////////////////
	#if	RREERTOS_USE == 1
		vTaskDelay(50/portTICK_RATE_MS) ;
	#elif RREERTOS_USE == 0
		delay_ms(50) ;
	#endif
	if( testESP8266() == 1 )
		return 1 ;
	else 
		return 0 ;
}



//////////////////////字符串处理///////////////////////////////
static void strIP2numIP(char *strIP, u8 *numIP) //字符串IP转换成数字IP
{
	u8 counter_for_point = 0 ;
	u8 loop = 0 ;
	char tmp_num[4][4] = {0} ; // string tmp for ip
	u8 i = 0 ;
	while( *strIP != '\0')
	{
		if( *strIP == '.')
		{
			counter_for_point++ ;
			strIP++ ;
			loop = 0 ;
			continue ;
		}
		for(i = 0 ;i < 4; i++)
		{
			if(counter_for_point == i)
			{
				tmp_num[i][loop] = *strIP ;
				loop++ ;
			}
		}
		strIP++ ;
	}
	for(i = 0; i < 4 ; i++)
		numIP[i] = atoi(tmp_num[i]) ;
}

static void strMAC2numMAC(char *strMAC, u8 *numMAC) //字符串MAC转成数字MAC
{
	u8 counter_for_point = 0 ;
	u8 loop = 0 ;
	char tmp_num[6][3] = {0} ; // string tmp for mac
	char *tmp ;
	u8 i = 0 ;
	while( *strMAC != '\0')
	{
		if( *strMAC == ':')
		{
			counter_for_point++ ;
			strMAC++ ;
			loop = 0 ;
			continue ;
		}
		for(i = 0 ; i < 6; i++)
		{
			if(counter_for_point == i)
			{
				tmp_num[i][loop] = *strMAC ;
				loop++ ;
			}
		}
		strMAC++ ;
	}
	for( i = 0; i <6; i++)
		numMAC[i] = strtol(tmp_num[i], &tmp, 16) ;
}

/**
* @brief 数字IP转换成字符串IP
* @attention 需要预先给strIP分配足够的内存
*/
static void numIP2strIP(u8 *numIP, char *strIP)
{
	sprintf(strIP, "%d.%d.%d.%d",numIP[0],numIP[1],numIP[2],numIP[3]) ;
}

/**
* @brief 数字MAC转换成字符串MAC
* @attention 需要预先给strMAC分配足够的内存
*/
static void numMAC2strMAC(u8 *numMAC, char *strMAC) 
{
	sprintf(strMAC, "%x:%x:%x:%x:%x:%x", numMAC[0], numMAC[1], numMAC[2], numMAC[3], numMAC[4], numMAC[5]) ;
}

static void cleanUSARTBuffer(void)
{
	memset(Rx1Buf, 0, Rx1Length*sizeof(char)) ;
	Rx1Flag = 0 ;
}
