/**
* @brief �����˹���ATָ���һЩ����
* @author FesianXu
* @version V0.1
* @date 2016.7.4
*/
#ifndef __ATCMD_H_
#define __ATCMD_H_

#include "../../SYSTEM/sys/sys.h"
#include "c_common.h"

#define __AT_CMD
#define AT_CMD_NUM 10
#define AT_QUERY "?"  //ѯ��ģʽ��һЩATָ����Ҫ
#define AT_FORMAT "%s%s%s" //atָ��͸�ʽ
#define AT_FORMAT_PARAMS_STR "%s%s=%s%s" //���в�����atָ��͸�ʽ
#define AT_FORMAT_PARAMS_INT "%s%s=%d%s" //���в�����atָ��͸�ʽ
#define AT_FORMAT_PARAMS_TCP_IN_MULTI "%s%s=%d,\"%s\",\"%s\",%d%s" //TCP
#define AT_FORMAT_PARAMS_TCP_IN_SINGLE "%s%s=\"%s\",\"%s\",%d%s" //TCP
#define AT_FORMAT_PARAMS_SERVER "%s%s=%d,%d%s" // server����

#ifdef AT_MODE
const static char *at_head = "AT" ; //ATָ��ͷ��
const static char *at_tail = "\r\n" ; //ATָ��β��
const static char *reset_pattern = "----RST_Finish\r\n" ; //��λ��־
const static char *ok_pattern = "----OK\r\n" ; //����ok��־
const static char *error_pattern = "ERROR\r\n" ; //�����־
const static char *connected_pattern = "ALREAY CONNECT\r\n" ; //�Ѿ�����TCP/UDP��־
const static char *build_pattern = "link is builded\r\n" ; //�ı�ģʽʱ�������Ӵ��� ��
const static char *rts_pattern = ">" ;//׼���������ݱ�־
const static char *send_ok_pattern = "SEND OK\r\n" ;//�������

typedef struct at_cmd
{
	char *at_cmdLine ;
	u8   len ;
	char *at_params ;
}AT_cmd ;
// at����ṹ��

typedef struct wifi_ap
{
	//0 open
	//1 WEP
	//2 WPA_PSK
	//3 WPA2_PSK
	//4 WPA_WPA2_PSK
	u8 ecn ; //wifi�ļ���ģʽ
	char ssid[20] ; //SSID
	signed char rssi ;  //�ź�ǿ��
	u8 MAC[6] ; //mac��ַ 48bits
	u8 channels ; //�ŵ���
}wifi_ap ; //WiFi ap����Ϣ,AT+CWLAP����

typedef struct cipstatus
{
	u8 id ; //���ӵ�id�ţ�0-4
	u8 connect_type ; // ����ΪTCPʱ0��ΪUDPʱ1
	u8 IP[4] ; //IP��ַ
	int port ;//�˿ں�
	u8 tetype ; //0����ģ����Ϊclient����
							//1: ��ģ����server������
}cipstatus ; // AT+CIPSTATUS����

typedef struct self_info
{
	u8 APip[4] ; //ap��ip��ַ
	u8 APmac[6] ; //ap��max��ַ
	u8 STAip[4] ; //STA��ip��ַ
	u8 STAmac[6] ; //STA��max��ַ
}self_info ; //������Ϣ


static __AT_CMD
AT_cmd at_cmd_table[] = {
	{"",0,NULL} , //test the AT mode is working or not ��
	{"+RST",4,NULL} , //reset the esp8266 ��
	{"+GMR",4,NULL}, //show the AT cmd version ��
	{"+CWMODE",7,NULL} , //which mode, STA , AP ,STA&AP ��
	{"+CWJAP",6,NULL} ,  //join an AP ��
	{"+CWLAP",6,NULL} , // show the AP list ��
	{"+CWQAP",6,NULL} , //quit the connection with AP ��
	{"+CWSAP",6,NULL} , //set the parameters in the AP mode ��
	{"+CWLIF",6,NULL} , //show the IP that already connected this device ��
	{"+CIPSTATUS",10,NULL} , //show the status of the connection
	{"+CIPSTART",9,NULL} , //establish a TCP connection or set up a UDP port ��
	{"+CIPSEND",8,NULL} , //send data ��
	{"+CIPCLOSE",9,NULL} , //close the TCP or UDP connection ��
	{"+CIFSR",6,NULL} , //get the local ip address ��
	{"+CIPMUX",7,NULL} , //set the multiple connections or not ��
	{"+CIPSERVER",10,NULL} , //set as a server ��
	{"+CIPMODE",8,NULL} , //set the transmit mode	��
	{"+CIPSTO",7,NULL} , // set the over-time ��
	{"+++",3,NULL} //quit from transparent transmission 
	} ;
///////////////////////////////////////////////////////////////////////////
u8 initAT_CMD(void) ;
u8 searchWiFi_AP(signed char threshold) ; //search the surrounding wifi ap
u8 resetESP8266(void) ;  //reset the esp8266
u8 joinAP(char *ssid, char *passwd) ; //join an AP
u8 quitAP(void) ; //quit the joined ap 
u8 setWiFiMode(u8 mode) ; //set the mode of the wifi
u8 testESP8266(void) ; //test the esp8266 whether right working
u8 searchDevicesIP(u8 *ips) ; // search the connected devices' IP
u8 getLocalIP(self_info *local_info) ; //got the local ip and mac Address
u8 createTCP_inMultiple(u8 id, char *addr, int port) ; //create a TCP connection in multiple cipmux = 1 mode ;
u8 createTCP_inSingle(char *addr, int port) ; //create a TCP connection in single cipmux = 0 mode
u8 closeTCP_inMultiple(u8 id) ; //close multiple TCP connection
u8 closeTCP_inSingle(void) ; // close single TCP connection
u8 setMultipleConnection(u8 mode) ; //set up the multiple connections
u8 setTransmitMode(u8 mode) ; //set the transmit mode
u8 setAsServer(u8 mode, unsigned int port) ; //set as a server
u8 setParamsInAPMode(char *ssid, char *pwd, u8 chl, u8 ecn) ; //set the parameters to the esp8266 in the AP mode
u8 showATCmdVersion(char *version) ; //show AT cmd version
u8 getLinkStatus(cipstatus *status) ; //get the status of the TCP or UDP link
u8 SendDataInSingle_noTransparent(char *data) ;//send data in single links with no transparent mode
u8 SendDataInMultiple_noTransparent(char *data, u8 id) ; //send data in multiple links with no transparent mode
u8 SendData_Transparent(char *data) ; //send data in transparent mode
u8 quitFromTransparent(void) ; // quit from transparent mode

static void strIP2numIP(char *strIP, u8 *numIP) ;
static void strMAC2numMAC(char *strMAC, u8 *numMAC) ;
static void numIP2strIP(u8 *numIP, char *strIP) ;
static void numMAC2strMAC(u8 *numMAC, char *strMAC) ;
static void cleanUSARTBuffer(void) ;
#endif
	
	
#endif
	
	
	
	
	
	
	
	
	