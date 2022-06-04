#include "mks_wifi.h"

#ifdef MKS_WIFI

#include "../../lcd/marlinui.h"
#include "mks_wifi_sd.h"
#include "mks_wifi_gcodes.h"
#include "../../lcd/tft/tft.h"
#include "../../lcd/tft/touch.h"
#include "../../lcd/menu/menu_item.h"
#include "../../core/multi_language.h"

volatile uint8_t mks_in_buffer[MKS_IN_BUFF_SIZE];
uint8_t mks_out_buffer[MKS_OUT_BUFF_SIZE];

char mksWifiNameList[21][32] = {0};
char ip_addr[17] = {0};
uint8_t wifiListPage = 0;
uint8_t mks_wifi_keyValue[3] = {0};
char mks_wifi_password[70] = {0};

uint8_t mks_wifi_wordList[4][27] = {  {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' '},
                                      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' '},
									  {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '|', ':', ';', '(', ')', '$', '&', '@', '\"', '.', '-', '-', '-', '-', '-', ' '},
									  {',', '?', '!', '\'', '[', ']', '#', '{', '}', '%', '^', '*', '+', '=', '_', '\\', '/', '~', '<', '>', '-', '-', '-', '-', '-', '-', ' '}  };

volatile uint8_t esp_packet[MKS_TOTAL_PACKET_SIZE];

MKS_WIFI_INFO mks_wifi_info; // __attribute__ ((section (".ccmram")));

void mks_wifi_init(void)
{
	SERIAL_ECHO_MSG("Init MKS WIFI");
	DEBUG("Init MKS WIFI");

	memset(&mks_wifi_info, 0, sizeof(mks_wifi_info));

	SET_OUTPUT(MKS_WIFI_IO0);
	WRITE(MKS_WIFI_IO0, HIGH);

	SET_OUTPUT(MKS_WIFI_IO4);
	WRITE(MKS_WIFI_IO4, HIGH);

	SET_OUTPUT(MKS_WIFI_IO_RST);
	WRITE(MKS_WIFI_IO_RST, LOW);

	safe_delay(200);
	WRITE(MKS_WIFI_IO_RST, HIGH);

	safe_delay(200);
	WRITE(MKS_WIFI_IO4, LOW);
}

void mks_wifi_set_param(void)
{
	uint32_t packet_size;
	ESP_PROTOC_FRAME esp_frame;

	uint32_t ap_len = strlen((const char *)MKS_WIFI_SSID);
	uint32_t key_len = strlen((const char *)MKS_WIFI_KEY);

	memset(mks_out_buffer, 0, MKS_OUT_BUFF_SIZE);

	mks_out_buffer[0] = WIFI_MODE_STA;

	mks_out_buffer[1] = ap_len;
	memcpy((char *)&mks_out_buffer[2], (const char *)MKS_WIFI_SSID, ap_len);

	mks_out_buffer[2 + ap_len] = key_len;
	memcpy((char *)&mks_out_buffer[2 + ap_len + 1], (const char *)MKS_WIFI_KEY, key_len);

	esp_frame.type = ESP_TYPE_NET;
	esp_frame.dataLen = 2 + ap_len + key_len + 1;
	esp_frame.data = mks_out_buffer;
	packet_size = mks_wifi_build_packet((uint8_t *)esp_packet, &esp_frame);

	if (packet_size > 8)
	{ // 4 байта заголовка + 2 байта длины + хвост + название сети и пароль
		//выпихнуть в uart
		mks_wifi_send((uint8_t *)esp_packet, packet_size);
	};
}

/*
Получает данные из всех функций, как только
есть перевод строки 0x0A, формирует пакет для
ESP и отправляет
*/
void mks_wifi_out_add(uint8_t *data, uint32_t size)
{
	MYSERIAL2.write(0xA5);
	MYSERIAL2.write(0x02);

	uint8_t *dataLen = (uint8_t *)&size;

	for (size_t i = 0; i < 2; i++)
		MYSERIAL2.write(dataLen[i]);

	for (size_t i = 0; i < size; i++)
		MYSERIAL2.write(data[i]);

	MYSERIAL2.write(0xFC);
}

uint8_t mks_wifi_input(uint8_t data)
{
	ESP_PROTOC_FRAME esp_frame;
#ifdef MKS_WIFI_ENABLED_WIFI_CONFIG
	static uint8_t get_packet_from_esp = 0;
#endif
	static uint8_t packet_start_flag = 0;
	static uint8_t packet_type = 0;
	static uint16_t packet_index = 0;
	static uint16_t payload_size = ESP_PACKET_DATA_MAX_SIZE;
	uint8_t ret_val = 1;

	//Не отдавать данные в очередь команд, если идет печать
	// if (CardReader::isPrinting()){
	// 	DEBUG("No input while printing");
	// 	return 1;
	// }

	if (data == ESP_PROTOC_HEAD)
	{
		payload_size = ESP_PACKET_DATA_MAX_SIZE;
		packet_start_flag = 1;
		packet_index = 0;
		memset((uint8_t *)mks_in_buffer, 0, MKS_IN_BUFF_SIZE);
	}
	else if (!packet_start_flag)
	{
		DEBUG("Byte not in packet %0X", data);
		return 1;
	}

	if (packet_start_flag)
	{
		mks_in_buffer[packet_index] = data;
	}

	if (packet_index == 1)
	{
		packet_type = mks_in_buffer[1];
	}

	if (packet_index == 3)
	{
		payload_size = uint16_t(mks_in_buffer[3] << 8) | mks_in_buffer[2];

		if (payload_size > ESP_PACKET_DATA_MAX_SIZE)
		{
			ERROR("Payload size too big");
			packet_start_flag = 0;
			packet_index = 0;
			memset((uint8_t *)mks_in_buffer, 0, MKS_IN_BUFF_SIZE);
			return 1;
		}
	}

	if ((packet_index >= (payload_size + 4)) || (packet_index >= ESP_PACKET_DATA_MAX_SIZE))
	{

		if (mks_wifi_check_packet((uint8_t *)mks_in_buffer))
		{
			ERROR("Packet check failed");
			packet_start_flag = 0;
			packet_index = 0;
			return 1;
		}

		esp_frame.type = packet_type;
		esp_frame.dataLen = payload_size;
		esp_frame.data = (uint8_t *)&mks_in_buffer[4];

		mks_wifi_parse_packet(&esp_frame);

#ifdef MKS_WIFI_ENABLED_WIFI_CONFIG
		if (!get_packet_from_esp)
		{
			DEBUG("Fisrt packet from ESP, send config");

			mks_wifi_set_param();
			get_packet_from_esp = 1;
		}
#endif
		packet_start_flag = 0;
		packet_index = 0;
	}

	/* Если в пакете G-Сode, отдаем payload дальше в обработчик марлина */
	// if((packet_type == ESP_TYPE_GCODE) &&
	//    (packet_index >= 4) &&
	//    (packet_index < payload_size+5)
	//   ){

	// 	if(!check_char_allowed(data)){
	// 		ret_val=0;
	// 	}else{
	// 		ERROR("Char not allowed: %0X %c",data,data);
	// 		packet_start_flag=0;
	// 		packet_index=0;
	// 		return 1;
	// 	}

	// }

	if (packet_start_flag)
	{
		packet_index++;
	}

	return ret_val;
}

/*
Проверяет, что символы из текстового диапазона
для G-code команд
*/
uint8_t check_char_allowed(char data)
{

	if (data == 0x0a || data == 0x0d)
	{
		return 0;
	}

	if ((data >= 0x20) && (data <= 0x7E))
	{
		return 0;
	}

	return 1;
}

/*
Проверяет пакет на корректность:
наличие заголовка
наличие "хвоста"
длина пакета
*/
uint8_t mks_wifi_check_packet(uint8_t *in_data)
{
	uint16_t payload_size;
	uint16_t tail_index;

	if (in_data[0] != ESP_PROTOC_HEAD)
	{
		ERROR("Packet head mismatch");
		return 1;
	}

	payload_size = uint16_t(in_data[3] << 8) | in_data[2];

	if (payload_size > ESP_PACKET_DATA_MAX_SIZE)
	{
		ERROR("Payload size mismatch");
		return 1;
	}

	tail_index = payload_size + 4;

	if (in_data[tail_index] != ESP_PROTOC_TAIL)
	{
		ERROR("Packet tail mismatch");
		return 1;
	}

	return 0;
}

void mks_wifi_parse_packet(ESP_PROTOC_FRAME *packet)
{
	static uint8_t show_ip_once = 0;
	char str[100];
	char ip_addr_old[25] = {0};
	int cmd_value = 0;

	switch (packet->type)
	{
	case ESP_TYPE_NET:
		memset(str, 0, 100);
		if (packet->data[6] == ESP_NET_WIFI_CONNECTED)
		{
			if (show_ip_once == 0)
			{
				show_ip_once = 1;
				sprintf(str, "IP %d.%d.%d.%d", packet->data[0], packet->data[1], packet->data[2], packet->data[3]);
				ui.set_status((const char *)str, true);
				SERIAL_ECHO_START();
				SERIAL_ECHOLN((char *)str);

				//Вывод имени сети

				if (packet->data[8] < 100)
				{
					memcpy(str, &packet->data[9], packet->data[8]);
					str[packet->data[8]] = 0;
					SERIAL_ECHO_START();
					SERIAL_ECHO("WIFI: ");
					SERIAL_ECHOLN((char *)str);
				}
				else
				{
					DEBUG("Network NAME too long");
				}
			}
			//DEBUG("[Net] connected, IP: %d.%d.%d.%d",packet->data[0],packet->data[1],packet->data[2],packet->data[3]);
		}
		else if (packet->data[6] == ESP_NET_WIFI_EXCEPTION)
		{
			DEBUG("[Net] wifi exeption");
		}
		else
		{
			DEBUG("[Net] wifi not config");
		}
		mks_wifi_info.connected = (packet->data[6] == 0x0A);
		memcpy(&(mks_wifi_info.ip), &(packet->data[0]), 4);
		mks_wifi_info.mode = packet->data[7];
		if (packet->data[8] < 32)
		{
			strncpy(mks_wifi_info.net_name, (char *)&(packet->data[9]), packet->data[8]);
			memset(&mks_wifi_info.net_name[packet->data[8]], 0x20, 32-packet->data[8]-1);
		}
		else
		{
			strncpy(mks_wifi_info.net_name, (char *)&(packet->data[9]), 31);
		}
        
		if (mks_wifi_info.net_name[0] == 'M' && mks_wifi_info.net_name[1] == 'K' && mks_wifi_info.net_name[2] == 'S' && mks_wifi_info.net_name[3] == 'W')
		{
			mks_wifi_ap_mode();
		}
        
        memcpy(ip_addr_old, ip_addr, sizeof(ip_addr_old));
        sprintf(ip_addr,"%d.%d.%d.%d", mks_wifi_info.ip[0], mks_wifi_info.ip[1], mks_wifi_info.ip[2], mks_wifi_info.ip[3]);

		if(strcmp(ip_addr_old, ip_addr) != 0)
		{
			memset(ip_addr_old, 0, sizeof(ip_addr_old));
			memcpy(ip_addr_old, "IP ", 3);
			strcat(ip_addr_old, ip_addr);
			ui.set_status(ip_addr_old, false);
		}     
		
	case ESP_TYPE_GCODE:
		char gcode_cmd[50];
		uint32_t cmd_index;
		// packet->data[packet->dataLen] = 0;
		// DEBUG("Gcode packet: %s",packet->data);

		cmd_index = 0;
		memset(gcode_cmd, 0, 50);
		for (uint32_t i = 0; i < packet->dataLen; i++)
		{

			if (packet->data[i] != 0x0A)
			{
				gcode_cmd[cmd_index++] = packet->data[i];
			}
			else
			{
				if (0x47 == gcode_cmd[0])
				{
					GCodeQueue::ring_buffer.enqueue((const char *)gcode_cmd, false, MKS_WIFI_SERIAL_NUM);
					cmd_index = 0;
					memset(gcode_cmd, 0, 50);
				}
				else
				{
					cmd_value = strtol((const char *)&gcode_cmd[1], NULL, 10);

					switch (cmd_value)
					{
					case 105:
						mks_m105();
						break;

					case 115:
						mks_m115();
						break;

					case 991:
						mks_m991();
						break;

					case 992:
						mks_m992();
						break;

					case 994:
						mks_m994();
						break;

					case 997:
						mks_m997();
						break;

					case 25:
						mks_m25();
						break;

					case 26:
						mks_m26();
						break;

					case 27:
						mks_m27();
						break;

					case 20:
						mks_m20();
						break;

					default:
						GCodeQueue::ring_buffer.enqueue((const char *)gcode_cmd, false, MKS_WIFI_SERIAL_NUM);
						break;
					}

					cmd_index = 0;
					memset(gcode_cmd, 0, 50);
				}
			}
		}

		break;
	case ESP_TYPE_FILE_FIRST:
		DEBUG("[FILE_FIRST]");
		//Передача файла останавливает все процессы,
		//поэтому печать в этот момент не возможна.
		if (!CardReader::isPrinting())
		{
			mks_wifi_start_file_upload(packet);
		}
		break;
	case ESP_TYPE_FILE_FRAGMENT:
		DEBUG("[FILE_FRAGMENT]");
		break;
	case ESP_TYPE_WIFI_LIST:
	    memset(mksWifiNameList, 0, sizeof(mksWifiNameList));
		mks_wifi_list(packet);
		wifiListPage = 0;
		memset(mks_wifi_keyValue, 0, 3);
		mks_wifi_list_display();
		break;
	default:
		DEBUG("[Unkn]");
		break;
	}
}

uint16_t mks_wifi_build_packet(uint8_t *packet, ESP_PROTOC_FRAME *esp_frame)
{
	uint16_t packet_size = 0;

	memset(packet, 0, MKS_TOTAL_PACKET_SIZE);
	packet[0] = ESP_PROTOC_HEAD;
	packet[1] = esp_frame->type;

	for (uint32_t i = 0; i < esp_frame->dataLen; i++)
	{
		packet[i + 4] = esp_frame->data[i]; // 4 байта заголовка отступить
	}

	packet_size = esp_frame->dataLen + 4;

	if (packet_size > MKS_TOTAL_PACKET_SIZE)
	{
		ERROR("ESP packet too big");
		return 0;
	}

	if (esp_frame->type != ESP_TYPE_NET)
	{
		packet[packet_size++] = 0x0d;
		packet[packet_size++] = 0x0a;
		esp_frame->dataLen = esp_frame->dataLen + 2; //Два байта на 0x0d 0x0a
	}

	*((uint16_t *)&packet[2]) = esp_frame->dataLen;

	packet[packet_size] = ESP_PROTOC_TAIL;
	return packet_size;
}

void mks_wifi_ap_mode(void)
{
	uint8_t disconnect[5] = {0xA5, 0x05, 0x00, 0x00, 0xFC};
	uint8_t ap_mode[23] = {0xA5, 0x00, 0x12, 0x00, 0x01, 0x07, 'K', 'y', 'w', 'o', 'o', '3', 'D', 0x8, '0', '1', '2', '3', '4', '5', '6', '7', 0xFC};

	for (size_t i = 0; i < 5; i++)  MYSERIAL2.write(disconnect[i]);
	delay(100);
	for (size_t i = 0; i < 23; i++)  MYSERIAL2.write(ap_mode[i]);
}

void mks_wifi_scan(void)
{
	uint8_t scan[5] = {0xA5, 0x07, 0x00, 0x00, 0xFC};
    
	for (size_t i = 0; i < 5; i++)  MYSERIAL2.write(scan[i]);
	delay(300);
	for (size_t i = 0; i < 5; i++)  MYSERIAL2.write(scan[i]);
	
	tft.canvas(0, 0, 300, 260);
    tft.set_background(COLOR_BLACK);
    tft_string.set(GET_TEXT(MSG_WIFI_SEARCH));
    tft.add_text(50, 80, COLOR_YELLOW, tft_string);
	tft.queue.sync();
}

void mks_wifi_list(ESP_PROTOC_FRAME *packet)
{
	uint8_t *dataNew = packet->data;
	uint16_t index = 0;
	uint16_t length = 0;

	for (size_t i = 0; i < dataNew[0]; i++)
	{
		index++ ;
		memcpy(mksWifiNameList[i], &dataNew[index + 1], dataNew[index]);
        index += dataNew[index];
		index++;
	}  
}
	

void mks_wifi_list_display(void)
{
	if (ui.currentScreen == menu_info_wifi)
	{
	    ui.clear_lcd(); 

		for (size_t i = 0; i < 5; i++)
		{
			tft.canvas(0, i*64, 400, 64);
			tft.set_background(COLOR_BLACK);
			tft_string.set(mksWifiNameList[i+5*wifiListPage]);
			tft.add_text(0, 15, COLOR_YELLOW, tft_string);
		}
		tft.queue.sync();

		add_control(410, 40, WIFI_PAGE_UP, imgUp, true);
		add_control(410, 247, WIFI_PAGE_DOWN, imgDown, true);
		add_control(410, 143, BACK, imgBack);

		touch.add_control(WIFI_NAME_0, 0, 0, 400, 64, 0);
		touch.add_control(WIFI_NAME_1, 0, 64, 400, 64, 0);
		touch.add_control(WIFI_NAME_2, 0, 128, 400, 64, 0);
		touch.add_control(WIFI_NAME_3, 0, 192, 400, 64, 0);
		touch.add_control(WIFI_NAME_4, 0, 256, 400, 64, 0);
	}
}

void mks_wifi_password_page_1(void)
{
	uint16_t color_back = COLOR_GREY;
	uint16_t color_word = COLOR_CYAN;

	tft.canvas(0, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("a");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("b");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("c");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("d");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("e");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("f");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 104, 71, 53);
    tft.set_background(color_back);
    tft_string.set("g");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("h");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("i");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("j");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("k");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("l");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("m");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 158, 71, 53);
    tft.set_background(color_back);
    tft_string.set("n");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("o");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("p");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("q");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("r");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("s");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("t");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 212, 71, 53);
    tft.set_background(color_back);
    tft_string.set("u");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("v");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("w");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("x");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("y");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("z");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("space");
    tft.add_text(0, 11, color_word, tft_string);
	tft.canvas(408, 266, 71, 53);
    tft.set_background(color_back);
    tft_string.set("  <---");
    tft.add_text(0, 11, color_word, tft_string);
	tft.queue.sync();
    
}

void mks_wifi_password_page_0(void)
{
	uint16_t color_back = COLOR_GREY;
	uint16_t color_word = COLOR_CYAN;

    tft.canvas(409, 0, 70, 49);
    tft.set_background(COLOR_BLACK);
    tft_string.set("OK>>");
    tft.add_text(0, 11, COLOR_GREEN, tft_string);
	tft.canvas(0, 50, 95, 53);
    tft.set_background(color_back);
    tft_string.set("ABC");
    tft.add_text(20, 11, color_word, tft_string);
	tft.canvas(96, 50, 95, 53);
    tft.set_background(color_back);
    tft_string.set("abc");
    tft.add_text(20, 11, color_word, tft_string);
	tft.canvas(192, 50, 95, 53);
    tft.set_background(color_back);
    tft_string.set("123");
    tft.add_text(20, 11, color_word, tft_string);
	tft.canvas(288, 50, 95, 53);
    tft.set_background(color_back);
    tft_string.set("#+=");
    tft.add_text(20, 11, color_word, tft_string);
	tft.canvas(384, 50, 95, 53);
    tft.set_background(color_back);
    tft_string.set("  <<del");
    tft.add_text(5, 11, color_word, tft_string);
//-----------------------------------------------------------------

	tft.canvas(0, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("A");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("B");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("C");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("D");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("E");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("F");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 104, 71, 53);
    tft.set_background(color_back);
    tft_string.set("G");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("H");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("I");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("J");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("K");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("L");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("M");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 158, 71, 53);
    tft.set_background(color_back);
    tft_string.set("N");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("O");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("P");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("Q");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("R");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("S");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("T");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 212, 71, 53);
    tft.set_background(color_back);
    tft_string.set("U");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("V");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("W");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("X");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("Y");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("Z");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("space");
    tft.add_text(0, 11, color_word, tft_string);
	tft.canvas(408, 266, 71, 53);
    tft.set_background(color_back);
    tft_string.set("  <---");
    tft.add_text(0, 11, color_word, tft_string);
    tft.queue.sync();

	mks_wifi_keyboard_touch();
}

void mks_wifi_password_page_2(void)
{
	uint16_t color_back = COLOR_GREY;
	uint16_t color_word = COLOR_CYAN;

	tft.canvas(0, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("1");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("2");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("3");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("4");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("5");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("6");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 104, 71, 53);
    tft.set_background(color_back);
    tft_string.set("7");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("8");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("9");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("0");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("-");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("|");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set(":");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 158, 71, 53);
    tft.set_background(color_back);
    tft_string.set(";");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("(");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set(")");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("$");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("&");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("@");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("\"");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 212, 71, 53);
    tft.set_background(color_back);
    tft_string.set(".");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("space");
    tft.add_text(0, 11, color_word, tft_string);
	tft.canvas(408, 266, 71, 53);
    tft.set_background(color_back);
    tft_string.set("  <---");
    tft.add_text(0, 11, color_word, tft_string);

	tft.queue.sync();
}

void mks_wifi_password_page_3(void)
{
	uint16_t color_back = COLOR_GREY;
	uint16_t color_word = COLOR_CYAN;

	tft.canvas(0, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set(",");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("?");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("!");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("'");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("[");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 104, 67, 53);
    tft.set_background(color_back);
    tft_string.set("]");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 104, 71, 53);
    tft.set_background(color_back);
    tft_string.set("#");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("{");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("}");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("%");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("^");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("*");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 158, 67, 53);
    tft.set_background(color_back);
    tft_string.set("+");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 158, 71, 53);
    tft.set_background(color_back);
    tft_string.set("=");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("_");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("\\");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("/");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("~");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set("<");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 212, 67, 53);
    tft.set_background(color_back);
    tft_string.set(">");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(408, 212, 71, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
//-----------------------------------------------------------------
	tft.canvas(0, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(68, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(136, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(204, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(272, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set(" ");
    tft.add_text(25, 11, color_word, tft_string);
	tft.canvas(340, 266, 67, 53);
    tft.set_background(color_back);
    tft_string.set("space");
    tft.add_text(0, 11, color_word, tft_string);
	tft.canvas(408, 266, 71, 53);
    tft.set_background(color_back);
    tft_string.set("  <---");
    tft.add_text(0, 11, color_word, tft_string);

	tft.queue.sync();
}

void mks_wifi_keyboard_touch(void)
{
	touch.add_control(WIFI_PASSWORD_OK, 370, 0, 110, 49,0);
	touch.add_control(KEYBOARD_1_1, 0, 50, 96, 54,0);
	touch.add_control(KEYBOARD_1_2, 96, 50, 96, 54,0);
	touch.add_control(KEYBOARD_1_3, 192, 50, 96, 54,0);
	touch.add_control(KEYBOARD_1_4, 288, 50, 96, 54,0);
	touch.add_control(KEYBOARD_1_5, 384, 50, 96, 54,0);
	touch.add_control(KEYBOARD_2_1, 0, 104, 68, 54,0);
	touch.add_control(KEYBOARD_2_2, 68, 104, 68, 54,0);
	touch.add_control(KEYBOARD_2_3, 136, 104, 68, 54,0);
	touch.add_control(KEYBOARD_2_4, 204, 104, 68, 54,0);
	touch.add_control(KEYBOARD_2_5, 272, 104, 68, 54,0);
	touch.add_control(KEYBOARD_2_6, 340, 104, 68, 54,0);
	touch.add_control(KEYBOARD_2_7, 408, 104, 71, 54,0);
	touch.add_control(KEYBOARD_3_1, 0, 158, 68, 54,0);
	touch.add_control(KEYBOARD_3_2, 68, 158, 68, 54,0);
	touch.add_control(KEYBOARD_3_3, 136, 158, 68, 54,0);
	touch.add_control(KEYBOARD_3_4, 204, 158, 68, 54,0);
	touch.add_control(KEYBOARD_3_5, 272, 158, 68, 54,0);
	touch.add_control(KEYBOARD_3_6, 340, 158, 68, 54,0);
	touch.add_control(KEYBOARD_3_7, 408, 158, 71, 54,0);
	touch.add_control(KEYBOARD_4_1, 0, 212, 68, 54,0);
	touch.add_control(KEYBOARD_4_2, 68, 212, 68, 54,0);
	touch.add_control(KEYBOARD_4_3, 136, 212, 68, 54,0);
	touch.add_control(KEYBOARD_4_4, 204, 212, 68, 54,0);
	touch.add_control(KEYBOARD_4_5, 272, 212, 68, 54,0);
	touch.add_control(KEYBOARD_4_6, 340, 212, 68, 54,0);
	touch.add_control(KEYBOARD_4_7, 408, 212, 71, 54,0);
	touch.add_control(KEYBOARD_5_1, 0, 266, 68, 54,0);
	touch.add_control(KEYBOARD_5_2, 68, 266, 68, 54,0);
	touch.add_control(KEYBOARD_5_3, 136, 266, 68, 54,0);
	touch.add_control(KEYBOARD_5_4, 204, 266, 68, 54,0);
	touch.add_control(KEYBOARD_5_5, 272, 266, 68, 54,0);
	touch.add_control(KEYBOARD_5_6, 340, 266, 68, 54,0);
	touch.add_control(BACK, 408, 266, 71, 54,0);
}

void mks_wifi_keyboard(void)
{
    char password[70] = {0};
	uint8_t len = 0;

	mks_wifi_password[mks_wifi_keyValue[2]] = mks_wifi_wordList[mks_wifi_keyValue[0]][mks_wifi_keyValue[1]];

	memcpy(password, mks_wifi_password, mks_wifi_keyValue[2]+1);

	memcpy(mks_wifi_password, password, 70);

	len = mks_wifi_keyValue[2]+1;

	if ( len > 27 )	 memcpy(password, &password[len-27], len);
	
	tft.canvas(0, 0, 408, 49);
    tft.set_background(COLOR_BLACK);
    tft_string.set(password);
    tft.add_text(0, 10, COLOR_CYAN, tft_string);
    tft.queue.sync();

	if (mks_wifi_keyValue[2] < 68)  mks_wifi_keyValue[2]++;
}

void mks_wifi_del(void)
{
	char password[70] = {0};
	uint8_t len = 0;

	if (mks_wifi_keyValue[2] > 0)   mks_wifi_keyValue[2]--;
    memcpy(password, mks_wifi_password, mks_wifi_keyValue[2]);
	memcpy(mks_wifi_password, password, 70);
 
    len = mks_wifi_keyValue[2];
	if ( len > 27 )	 memcpy(password, &password[len-27], len);

	tft.canvas(0, 0, 408, 49);
    tft.set_background(COLOR_BLACK);
    tft_string.set(password);
    tft.add_text(0, 10, COLOR_CYAN, tft_string);
    tft.queue.sync();
}

void mks_wifi_password_ok(void)
{
	uint8_t disconnect[5] = {0xA5, 0x05, 0x00, 0x00, 0xFC};
	uint8_t wificfg[100] = {0xA5, 0x00, 0xCC, 0x00, 0x02, };
    uint32_t nameLen = 0;
	uint32_t passwordLen = 0;

	for (size_t i = 0; i < 5; i++)  MYSERIAL2.write(disconnect[i]);
	delay(100);

    nameLen = strlen(&mksWifiNameList[wifiListPage][0]);
	passwordLen = strlen(mks_wifi_password);

    wificfg[5] = nameLen;
	memcpy(&wificfg[6], &mksWifiNameList[wifiListPage][0], nameLen);

	wificfg[6 + nameLen] = passwordLen;

	memcpy(&wificfg[6 + nameLen + 1], mks_wifi_password, passwordLen);

    wificfg[2] = nameLen + passwordLen + 3;

	wificfg[6 + nameLen + 1 + passwordLen] = 0xFC;

	for (size_t i = 0; i < wificfg[2] + 5; i++)  MYSERIAL2.write(wificfg[i]);
}


void mks_wifi_send(uint8_t *packet, uint16_t size)
{
	for (uint32_t i = 0; i < (uint32_t)(size + 1); i++)
	{
		while (MYSERIAL2.availableForWrite() == 0)
		{
			safe_delay(10);
		}
		MYSERIAL2.write(packet[i]);
	}
}
                                            // WiFi info
void menu_info_wifi(void) {
  
      START_SCREEN();
      if (mks_wifi_info.connected)
      {
        PSTRING_ITEM(MSG_WIFI_CONNECTED, GET_TEXT(MSG_YES), SS_CENTER);
      
        if (mks_wifi_info.mode == 0x01)
          PSTRING_ITEM(MSG_WIFI_MODE, "AP", SS_CENTER);
        else
          PSTRING_ITEM(MSG_WIFI_MODE, "STA", SS_CENTER);

        PSTRING_ITEM(MSG_WIFI_ADDRESS, ip_addr, SS_CENTER);
        PSTRING_ITEM(MSG_WIFI_NETWORK, mks_wifi_info.net_name, SS_CENTER);

        tft.canvas(0, 260, 180, 60);
        tft.set_background(COLOR_GREEN);
        tft_string.set(GET_TEXT(MSG_WIFI_AP_MODE));
        tft.add_text(40, 15, COLOR_YELLOW, tft_string);
        tft.canvas(200, 260, 180, 60);
        tft.set_background(COLOR_GREEN);
        tft_string.set(GET_TEXT(MSG_WIFI_CONNECT));
        tft.add_text(10, 15, COLOR_YELLOW, tft_string);
        tft.queue.sync();

        touch.add_control(WIFI_AP, 0, 260, 180, 60, 0);
        touch.add_control(WIFI_CONNECT, 200, 260, 180, 60, 0);
      }
      else
      {
        PSTRING_ITEM(MSG_WIFI_IN_CONNECTED, "...", SS_CENTER);
      }

      END_SCREEN();
    }


#else
void mks_wifi_out_add(uint8_t *data, uint32_t size)
{
	while (size--)
	{
		MYSERIAL2.write(*data++);
	}
	return;
};

#endif