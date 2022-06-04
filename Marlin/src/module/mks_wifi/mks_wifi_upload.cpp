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

#include "mks_wifi_upload.h"
#include <stdio.h>
#include <string.h>
#include "../../module/temperature.h"
#include "../../gcode/queue.h"

SdFile update_file;
SdFile *update_curDir;

void mksWifiOpen(void)
{
    SET_OUTPUT(WIFI_IO1_PIN);
    WIFI_IO1_RESET();
    SET_INPUT_PULLUP(WIFI_IO0_PIN);
    WRITE(WIFI_IO0_PIN, LOW);
}

uint8_t wifi_upload_ESP8266(CardReader *card, const char *const filePath)
{
    uint8_t state = 0;
    wifi_resetTo_uartBoot();
    state = wifi_upload_begin(card, filePath);

    return state;
}

char find_firmware(CardReader *card, const char *const filePath)
{
    if (!card->isMounted())
        card->mount();

    watchdog_refresh();
    card->openFileRead(filePath);

    if (card->isFileOpen())
    {
        card->closefile();
        return 1;
    }
    else
    {
        return 0;
    }
}

void wifi_resetTo_uartBoot(void)
{
    SET_OUTPUT(WIFI_RESET_PIN);
    WIFI_SET();
    SET_OUTPUT(WIFI_IO1_PIN);
    SET_INPUT_PULLUP(WIFI_IO0_PIN);
    WIFI_IO1_SET();

    delay(1000UL);
    SET_OUTPUT(WIFI_IO0_PIN);
    WRITE(WIFI_IO0_PIN, LOW);
    WIFI_RESET();
    delay(2000UL);
    WIFI_SET();
    delay(1000UL);
}

void sendData(uint8_t *dataBlock, size_t length)
{
    uint8_t data;

    for (uint32_t i = 0; i < length; i++)
    {
        data = dataBlock[i];

        if (data == 0xC0)
        {
            MYSERIAL2.write(0xDB);
            MYSERIAL2.write(0xDC);
        }
        else if (data == 0xDB)
        {
            MYSERIAL2.write(0xDB);
            MYSERIAL2.write(0xDD);
        }
        else
        {
            MYSERIAL2.write(data);
        }
    }
}

void sendCommand(uint8_t *cmd, size_t length)
{
    MYSERIAL2.write(0xC0);
    sendData(cmd, length);
    MYSERIAL2.write(0xC0);
}

void sendDataBlock(uint8_t *head, uint8_t *headData, uint8_t *buff, size_t blockSize)
{
    MYSERIAL2.write(0xC0);
    sendData(head, 8);
    sendData(headData, 16);
    sendData(buff, blockSize);
    MYSERIAL2.write(0xC0);
}

uint32_t espcomm_calc_checksum(unsigned char *data, uint16_t data_size)
{
    uint16_t cnt;
    uint32_t result = 0xEF;

    for (cnt = 0; cnt < data_size; cnt++)
    {
        result ^= data[cnt];
    }

    return result;
}

size_t uartReadUpdataResponse(uint8_t *buffer, HardwareSerial *MYSERIAL, uint32_t timeOut)
{
    int ch = 0;
    int state = 1;
    uint32_t timeout = millis() + timeOut;

    while (PENDING(millis(), timeout))
    {
        switch (state)
        {
        case 1: //查找开始标志
            ch = MYSERIAL->read();
            if (ch != (uint8_t)0xC0)
                break;
            state = 2;
            break;

        case 2: //查找响应标志
            ch = MYSERIAL->read();
            if (ch != (uint8_t)0x01)
                break;
            state = 3;
            break;

        case 3: //读取响应信息
            for (size_t i = 0; i < 10; i++)
            {
                ch = MYSERIAL->read();
                buffer[i] = (uint8_t)ch;
            }

            if (0x00 == buffer[7]) //判断响应类型是否为正确
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }

    return 0;
}

uint8_t wifi_upload_begin(CardReader *card, const char *const filePath)
{
    uint32_t fileSize = 0;
    uint32_t eraseSize = 0;
    uint32_t blockCount = 0;
    const uint32_t sectorsPerBlock = 16;
    const uint32_t sectorSize = 4096;
    uint32_t numSectors = 0;
    uint32_t FlashBlockSize = 0x0400;
    const uint32_t startSector = 0x00000000 / sectorSize;

    uint8_t RXBuffer[150] = {0};

    uint32_t dataBlockCount = 0;

    uint8_t syncCom[44] = {0x00, 0x08, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x12, 0x20, 0x55, 0x55,
                           0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                           0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};

    uint8_t eraseCom[24] = {0x00, 0x02, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x06, 0x00, 0xAC, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const char *const fname = card->diveToFile(true, update_curDir, filePath);

    if (!update_file.open(update_curDir, fname, O_READ))
        return 0;

    fileSize = update_file.fileSize();
    numSectors = (fileSize + sectorSize - 1) / sectorSize;

    uint32_t headSectors = sectorsPerBlock - (startSector % sectorsPerBlock);
    NOMORE(headSectors, numSectors);

    eraseSize = (numSectors < 2 * headSectors) ? (numSectors + 1) / 2 * sectorSize : (numSectors - headSectors) * sectorSize;
    blockCount = (eraseSize + FlashBlockSize - 1) / FlashBlockSize;

    delay(5000UL);
    sendCommand(syncCom, sizeof(syncCom)); //同步命令
    delay(600UL);

    sendCommand(syncCom, sizeof(syncCom)); //
    delay(600UL);

    memcpy(&eraseCom[8], &eraseSize, sizeof(eraseSize));
    memcpy(&eraseCom[12], &blockCount, sizeof(blockCount));

    sendCommand(eraseCom, sizeof(eraseCom)); //擦除FLASH命令

    delay(3000UL);

    dataBlockCount = (fileSize + FlashBlockSize - 1) / FlashBlockSize; //数据块数量

    uint8_t header[8] = {0x00, 0x03, 0x10, 0x04, 0x5F, 0x00, 0x00, 0x00};                                                      //数据包报头
    uint8_t dataHeader[16] = {0x00, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //数据包描述,
    uint8_t buffer[FlashBlockSize + 10] = {0x00};
    uint16_t cnt = 0;
    uint32_t checkSum = 0;

    memcpy(&dataHeader[0], &FlashBlockSize, sizeof(FlashBlockSize));

    for (uint32_t i = 0; i < dataBlockCount; i++)
    {
        cnt = update_file.read(buffer, FlashBlockSize);

        if (i == dataBlockCount - 1)
        {
            memset(&buffer[cnt], 0xFF, FlashBlockSize - cnt);  //最后一块数据进行0xFF填充
        }

        checkSum = espcomm_calc_checksum(buffer, FlashBlockSize);

        memcpy(&header[4], &checkSum, sizeof(checkSum)); //设置校验和
        memcpy(&dataHeader[4], &i, sizeof(i));           //设置数据块下标

        delay(40UL);

        sendDataBlock(header, dataHeader, buffer, FlashBlockSize); //发送固件数据块
        if (uartReadUpdataResponse(RXBuffer, &MYSERIAL2, 3000) != 1)
            return 0; //响应为no则返回
    }

    delay(100UL);

    update_file.close();

    card->removeFile((char*)"KWWIFI.CUR");

    SdFile file, *curDir;
    const char *const fname2 = card->diveToFile(true, curDir, filePath);

    if (file.open(curDir, fname2, O_READ))
    {
        file.rename(curDir, (char*)"KWWIFI.CUR");
        file.close();
    }

    delay(500UL);
    SET_INPUT_PULLUP(WIFI_IO0_PIN);
    WIFI_RESET();
    delay(1500UL);
    WIFI_SET();

    return 1;
}

