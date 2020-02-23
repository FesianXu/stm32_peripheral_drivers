#include "at_cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include "c_interfaces.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "delay.h"
#include <string.h>

self_info local_info ;//������Ϣ��
u8 num_of_APs ; //AP������
wifi_ap APs[30] ; //ap��Ϣ

extern stdio usart1 ; //��c_interfaces.c�ж���
extern char Rx1Buf[Rx1Length] ; //��USART.c�ж���
extern u16 Rx1Flag ; //��USART.c�ж���

/**
* @brief ��ʼ��ATָ���Ҫ�ǳ�ʼ�����ڽӿ�
* @retVal 1 ��ʼ���ɹ�
* @retVal 0 ��ʼʧ�ܣ��ظ���ʼ��
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
* @brief ����ESP8266��ATָ��ģʽ�Ƿ�����
* @retVal 1 ��������
* @retVal 0 ��ʱ�˳�
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else 
		return 0 ;
}

/**
* @brief  ����WiFi AP
* @retVal u8 1 �������
* @retVal u8 0 ������ʱ���˳�
* @cmd AT+CWLAP
* @attetion �˴����ж����ݵ����ƣ����������Ż�,
* �����������ź�ǿ�ȷ�ֵΪ�оݣ���ʱδʵ�֣�
* Ҳ������������(δʵ��)
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
		taskENTER_CRITICAL() ; //�����ٽ���
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
		taskEXIT_CRITICAL() ; //�˳��ٽ���
		num_of_APs = i-1 ;
		return 1 ;
	}
	else
	{
		return 0 ;
	}
		
}

/**
* @brief ��λesp8266ģ��
* @retVal 1 ok_pattern�յ�
* @retVal 2 reset_pattern�յ�
* @retVal 0 ��λδ�ɹ�����ʱ�˳�
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if ( strstr(Rx1Buf, reset_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief ����һ��AP
* @retVal 1 ����AP�ɹ�
* @retVal 0 ����APʧ�ܣ���ʱ�˳�
* @retVal 2 ����APʧ�ܣ���ʾERROR
* @params char *ssid AP��ssid
* @params char *passwd AP������
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}


/**
* @brief ѡ��wifi��Ӧ��ģʽ
* @retVal 1 ���óɹ�
* @retVal 2 ����ʧ�ܣ�ERROR��ʾ
* @retVal 0 ����ʧ�ܣ���ʱ�˳�
* @params u8 mode 1 Stationģʽ
									2 APģʽ
									3 AP&Stationģʽ
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL )
		return 2 ;
	else 
		return 0 ;
}

/**
* @brief �˳���AP������
* @retVal 1 �˳��ɹ�
* @retVal 0 �˳�ʧ�ܣ���ʱ
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else 
		return 0 ;
}

/**
* @brief �鿴�Ѿ�������豸��IP
* @retVal 1 �鿴�ɹ�
* @retVal 0 �鿴ʧ��
* @params u8 *ips �����豸��ip��ַָ��
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
* @brief ��ȡ���ص�IP��ַ
* @retVal 1 ��ȡ�ɹ�,APģʽ
* @retVal 2 ��ȡ�ɹ���STAģʽ
* @retVal 3 ��ȡ�ɹ���AP&STAģʽ
* @retVal 0 ��ȡʧ�ܡ�
* @params self_info ��������
* @cmd AT+CIFSR
* @attention ��APģʽ����Ч
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
	if( pStrTmp != NULL) //˵����apip,apmac
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
	if( pStrTmp != NULL) //˵����staip, stamac
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
* @brief ����TCP����
* @retVal 1 ���ӳɹ�
* @retVal 0 ���Ӵ���
* @retVal 2 �����Ѿ�����
* @cmd AT+CIPSTART
* @attention ���ڶ�·����֮ʱ������CIPMUC = 1
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
* @brief ����TCP����
* @retVal 1 ���ӳɹ�
* @retVal 0 ���Ӵ���
* @retVal 2 �����Ѿ�����
* @cmd AT+CIPSTART
* @attention ���ڵ�·����֮ʱ������CIPMUC = 0
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, connected_pattern) != NULL)
		return 2 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL)
		return 0 ;
}

/**
* @brief �رն�·����TCP
* @retVal 1 �رճɹ�
* @retVal 0 �رճ�ʱʧ��
* @retVal 2 �ڵ�·�����У�û�и����ӣ���ʾERRORʧ��
* @retVal 3 �ڶ�·�����У�û�и�id���ӣ���ʾlink is notʧ��
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
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
* @brief �رյ�·����TCP
* @retVal 1 �رճɹ�
* @retVal 0 �رճ�ʱʧ��
* @retVal 2 �ڵ�·�����У�û�и����ӣ���ʾERRORʧ��
* @retVal 3 �ڶ�·�����У�û�и�id���ӣ���ʾlink is notʧ��
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
	////////////�ж��Ƿ���յ�ok_pattern/////////////////////////
	if( strstr(Rx1Buf, ok_pattern) != NULL )
		return 1 ;
	else if( strstr(Rx1Buf, error_pattern) != NULL)
		return 2 ;
	else
		return 0 ;
}

/**
* @brief ����������
* @retVal 1 ���������ӳɹ�
* @retVal 2	���������ӳɹ�
* @retVal 3 ����������ʧ��
* @retVal 4 ����������ʧ��
* @retVal 5 �Ѿ���������״̬
* @retVal 0 δ֪����
* @params u8 mode 1 ��·����
									0 ��·����
* @cmd AT+CIPMUX
* @attention ֻ�е����Ӷ��Ͽ�����ܸ��ģ���������� server ��Ҫ����ģ��
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
* @brief ����ģ�鴫��ģʽ
* @retVal 1 ���óɹ�
* @retVal 2 ��������link is builded״̬������ʧ��
* @retVal 3 ����ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
* @params u8 mode 1 ͸��ģʽ
									0 ��͸��ģʽ
* @cmd AT+CIPMODE
* @attention ��͸��������Ϊ+++�˳�֮�����ģʽ������Ҫ��������Ϊ
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
* @brief ���÷�������ʱʱ��
* @retVal 1 ���óɹ�
* @retVal 2 ����ʧ��,��ʾERROR
* @retVal 3 ���óɹ���û�иı䣬��ʾno change
* @retVal 0 ���ó�ʱʧ��
* @params unsigned int time 0-28800 ��������ʱʱ�� ��λs
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
* @brief ����Ϊ������
* @retVal 1 ���óɹ�
* @retVal 2 ����ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
* @params u8 mode 0 �ر�serverģʽ
									1	����serverģʽ
					unsigned int port �˿ں�
* @cmd AT+CIPSERVER
* @attention ����server���Զ�����server����������client����ʱ���Զ���
* ˳��ռ��һ������ AT+CIPMUX=1ʱ���ܿ���������
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
* @brief ����APģʽ�µĲ���
* @retVal 1 ���óɹ�
* @retVal 2 ����ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
* @params char *ssid �ַ������������������
					char *pwd	 �ַ��������������Ϊ64Bytes��ASCII
					u8 	chl		ͨ����
					u8	ecn		���ܷ�ʽ
					//0 open
					//1 WEP
					//2 WPA_PSK
					//3 WPA2_PSK
					//4 WPA_WPA2_PSK
* @cmd AT+CWSAP
* @attention ָ��ֻ����APģʽ��������Ч
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
* @brief �鿴ATָ��汾
* @retVal 1 �鿴�ɹ�
* @retVal 2 �鿴ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
* @params char *version �����ַ�����������Ҫ������Ч�ռ���,15��char
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
* @brief �������״̬
* @retVal 1 �鿴�ɹ�
* @retVal 2 �鿴ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
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
* @brief �ڵ�·���ӵ�����·������ݣ���͸��
* @retVal 1 ���ͳɹ�
* @retVal 2 ����ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
* @params char *data ���͵����ݣ���󳤶�Ϊ2048�ֽ�
* @cmd AT+CIPSEND
* @attention ��Ӧ���յ���������Ȼ��з��ء�>����Ȼ��ͽ�����͸��ģʽ
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
//		if( strstr(Rx1Buf, send_ok_pattern) != null) //�������
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
* @brief �ڶ�·���ӵ�����·������ݣ���͸��
* @retVal 1 ���ͳɹ�
* @retVal 2 ����ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
* @params char *data ���͵����ݣ���󳤶�Ϊ2048�ֽ�
					u8 id ��Ҫ���ڴ����id��
* @cmd AT+CIPSEND
* @attention ��Ӧ���յ���������Ȼ��з��ء�>����Ȼ��ͽ�����͸��ģʽ
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
//		if( strstr(Rx1Buf, send_ok_pattern) != null) //�������
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
* @brief ͸��������
* @retVal 1 ���ͳɹ�
* @retVal 2 ����ʧ��,��ʾERROR
* @retVal 0 ���ó�ʱʧ��
* @params char *data ���͵����ݣ���󳤶�Ϊ2048�ֽ�
* @cmd AT+CIPSEND
* @attention ��Ӧ���յ���������Ȼ��з��ء�>����Ȼ��ͽ�����͸��ģʽ
* @attention ��͸��������Ϊ+++�˳�֮�����ģʽ������Ҫ��������Ϊ
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
//		if( strstr(Rx1Buf, send_ok_pattern) != null) //�������
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
* @brief �˳�͸��ģʽ
* @retVal 1 �˳��ɹ�
* @retVal 2 �˳�ʧ��,��ʾERROR
* @retVal 0 �˳���ʱʧ��
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



//////////////////////�ַ�������///////////////////////////////
static void strIP2numIP(char *strIP, u8 *numIP) //�ַ���IPת��������IP
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

static void strMAC2numMAC(char *strMAC, u8 *numMAC) //�ַ���MACת������MAC
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
* @brief ����IPת�����ַ���IP
* @attention ��ҪԤ�ȸ�strIP�����㹻���ڴ�
*/
static void numIP2strIP(u8 *numIP, char *strIP)
{
	sprintf(strIP, "%d.%d.%d.%d",numIP[0],numIP[1],numIP[2],numIP[3]) ;
}

/**
* @brief ����MACת�����ַ���MAC
* @attention ��ҪԤ�ȸ�strMAC�����㹻���ڴ�
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
