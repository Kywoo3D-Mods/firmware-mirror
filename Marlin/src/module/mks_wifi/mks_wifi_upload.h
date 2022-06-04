/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "../../inc/MarlinConfig.h"
#include "../../sd/cardreader.h"

#ifdef __cplusplus
  extern "C" { /* C-declarations for C++ */
#endif

#define ESP_FIRMWARE_FILE         "KWWifi.bin"
#define WIFI_SET()                WRITE(WIFI_RESET_PIN, HIGH);
#define WIFI_RESET()              WRITE(WIFI_RESET_PIN, LOW);
#define WIFI_IO1_SET()            WRITE(WIFI_IO1_PIN, HIGH);
#define WIFI_IO1_RESET()          WRITE(WIFI_IO1_PIN, LOW);

extern uint8_t wifiRxBuffer[1100];

void mksWifiOpen(void);
char find_firmware(CardReader *card, const char * const filePath);
void wifi_resetTo_uartBoot(void);
uint8_t wifi_upload_begin(CardReader *card, const char * const filePath);
uint8_t wifi_upload_ESP8266(CardReader *card, const char * const filePath);
void sendData(uint8_t *dataBlock, size_t length);
void sendCommand(uint8_t *cmd, size_t length);
void sendDataBlock(uint8_t *head, uint8_t *headData, uint8_t *buff, size_t blockSize);
size_t uartReadUpdataResponse(uint8_t *buffer, HardwareSerial *MYSERIAL, uint32_t timeOut);
#ifdef __cplusplus
  } /* C-declarations for C++ */
#endif
