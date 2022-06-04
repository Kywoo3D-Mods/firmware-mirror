#ifndef MKS_WIFI_H
#define MKS_WIFI_H
#include "../../MarlinCore.h"
#include "../../inc/MarlinConfig.h"
#include "../../libs/Segger/log.h"
#include "mks_wifi_settings.h"
#include "../../gcode/queue.h"

void mks_wifi_out_add(uint8_t *data, uint32_t size);

#ifdef MKS_WIFI

#define MKS_OUT_BUFF_SIZE (ESP_PACKET_DATA_MAX_SIZE)
#define MKS_IN_BUFF_SIZE (ESP_PACKET_DATA_MAX_SIZE + 30)

#define MKS_TOTAL_PACKET_SIZE (ESP_PACKET_DATA_MAX_SIZE+10)
#define WIFI_MODE_STA				(uint8_t)2
#define WIFI_MODE_AP				(uint8_t)1

typedef struct
{
	uint8_t type; 
	uint16_t dataLen;
	uint8_t *data; 
} ESP_PROTOC_FRAME;

#define ESP_PROTOC_HEAD				(uint8_t)0xa5
#define ESP_PROTOC_TAIL				(uint8_t)0xfc

#define ESP_TYPE_NET				(uint8_t)0x00
#define ESP_TYPE_GCODE				(uint8_t)0x01
#define ESP_TYPE_FILE_FIRST			(uint8_t)0x02
#define ESP_TYPE_FILE_FRAGMENT		(uint8_t)0x03
#define ESP_TYPE_WIFI_LIST		    (uint8_t)0x04

#define ESP_PACKET_DATA_MAX_SIZE	1024
#define ESP_SERIAL_OUT_MAX_SIZE		1024

#define ESP_NET_WIFI_CONNECTED		(uint8_t)0x0A
#define ESP_NET_WIFI_EXCEPTION		(uint8_t)0x0E

#define NOP	__asm volatile ("nop")


typedef struct {
	bool		connected;
	uint8_t		ip[4];
	uint8_t		mode;
	char		net_name[32];
} MKS_WIFI_INFO;

extern MKS_WIFI_INFO mks_wifi_info;
extern char ip_addr[17];
extern char mksWifiNameList[21][32];
extern uint8_t wifiListPage;
extern uint8_t mks_wifi_keyValue[3];
extern char mks_wifi_password[70];
extern uint8_t mks_wifi_wordList[4][27];

void mks_wifi_init(void);

void mks_wifi_set_param(void);

uint8_t mks_wifi_input(uint8_t data);
void mks_wifi_parse_packet(ESP_PROTOC_FRAME *packet);

uint16_t mks_wifi_build_packet(uint8_t *packet, ESP_PROTOC_FRAME *esp_frame);

uint8_t mks_wifi_check_packet(uint8_t *in_data);
uint8_t check_char_allowed(char data);

void mks_wifi_send(uint8_t *packet, uint16_t size);

void mks_wifi_list(ESP_PROTOC_FRAME *packet);
void mks_wifi_list_display(void);
void mks_wifi_password_page_0(void);
void mks_wifi_password_page_1(void);
void mks_wifi_password_page_2(void);
void mks_wifi_password_page_3(void);
void mks_wifi_keyboard_touch(void);
void mks_wifi_keyboard(void);
void mks_wifi_del(void);
void mks_wifi_password_ok(void);
//void menu_info_wifi(void);

void mks_wifi_ap_mode(void);
void mks_wifi_scan(void);



void menu_info_wifi(void);

#endif
#endif