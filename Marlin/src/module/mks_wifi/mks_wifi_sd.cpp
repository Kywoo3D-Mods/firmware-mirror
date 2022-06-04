#include "mks_wifi_sd.h"

#include "../../MarlinCore.h"
#include "../../lcd/marlinui.h"
#include "../../libs/fatfs/ff.h"
#include "../../libs/buzzer.h"
#include "../temperature.h"
#include "../../libs/fatfs/fatfs_shared.h"
#include "uart.h"
#include "../../libs/numtostr.h"
#include "HardwareSerial.h"
#include "mks_wifi_upload.h"

#ifdef MKS_WIFI

#if ENABLED(TFT_480x320) || ENABLED(TFT_480x320_SPI)
#include "mks_wifi_ui.h"
#endif

volatile uint8_t dma_buff[1100] = {0};

volatile uint8_t dma_state = 0;

FIL upload_file;
unsigned int writeLen = 0;


void mks_wifi_sd_ls(void)
{
    res = f_opendir((DIR *)&dir, "0:"); /* Open the directory */
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir((DIR *)&dir, (FILINFO *)&fno); /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)
                break; /* Break on error or end of dir */
            DEBUG("%s\n", fno.fname);
        }
    }
    else
    {
        ERROR("Opendir error %d", res);
    }
    f_closedir((DIR *)&dir);
}

uint8_t mks_wifi_sd_init(void)
{
    card.release();
    DEBUG("Card release");
    res = f_mount((FATFS *)&FATFS_Obj, "0", 1);
    DEBUG("SD init result:%d", res);
    return (uint8_t)res;
}

void mks_wifi_sd_deinit(void)
{
    DEBUG("Unmount SD");
    f_mount(0, "", 1);
    DEBUG("Marlin mount");
    card.mount();
};

void sd_delete_file(char *filename)
{
    mks_wifi_sd_init();
    DEBUG("Remove %s", filename);
    f_unlink(filename);
    mks_wifi_sd_deinit();
}

/*
Ищет файл filename и возвращает 8.3 имя в dosfilename
Возвращаемое значение 1 если нашлось, 0 если нет
*/

uint8_t get_dos_filename(char *filename, char *dosfilename)
{
    uint8_t ret_val = 0;

    mks_wifi_sd_init();

    res = f_opendir((DIR *)&dir, "0:"); /* Open the directory */

    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir((DIR *)&dir, (FILINFO *)&fno); /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)
                break; /* Break on error or end of dir */

            if (!strcmp((char *)fno.fname, filename))
            {
                DEBUG("Found %s %s\n", fno.fname, fno.altname);
                strncpy(dosfilename, (char *)fno.altname, 13);
                ret_val = 1;
            }
        }
    }
    else
    {
        ERROR("Opendir error %d", res);
    }
    f_closedir((DIR *)&dir);

    mks_wifi_sd_deinit();

    return ret_val;
}

uint32_t file_size_writen = 0;
uint8_t file_failed_count = 0;

void mks_wifi_start_file_upload(ESP_PROTOC_FRAME *packet)
{
    char str[100];
    uint32_t file_size;
#if ENABLED(SHOW_PROGRESS)
    uint32_t old_file_size_writen;
#endif

    uint16_t in_sector;
    char file_failed_name[100] ={0};
    uint32_t dma_timeout = DMA_TIMEOUT;
    uint16_t data_size;
    int16_t save_bed, save_e0;

    uint32_t data_to_write = 0;
    uint8_t *data_packet;
    char file_name[100];

    save_bed = thermalManager.degTargetBed();
    save_e0 = thermalManager.degTargetHotend(0);

    DEBUG("Saved target temp E0 %d Bed %d", save_e0, save_bed);

    thermalManager.setTargetBed(0);
    thermalManager.setTargetHotend(0, 0);
    thermalManager.manage_heater();
    thermalManager.disable_all_heaters();
    //Установить имя файла. Смещение на 3 байта, чтобы добавить путь к диску
    file_name[0] = '0';
    file_name[1] = ':';
    file_name[2] = '/';

    memcpy((uint8_t *)file_name + 3, (uint8_t *)&packet->data[5], (packet->dataLen - 5));
    file_name[packet->dataLen - 5 + 3] = 0;

    file_size = (packet->data[4] << 24) | (packet->data[3] << 16) | (packet->data[2] << 8) | packet->data[1];
    DEBUG("Start file %s size %d", file_name, file_size);

    //Отмонтировать SD от Marlin, Монтировать FATFs
    if (mks_wifi_sd_init())
    {
        ERROR("Error SD mount");
        ui.set_status((const char *)"Error SD mount", true);
        ui.update();
        mks_wifi_sd_deinit();

        WIFI_IO1_SET();
        delay(2500);
        WIFI_IO1_RESET();
        return;
    }

    DEBUG("Open file");
    //открыть файл для записи
    res = f_open((FIL *)&upload_file, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (res)
    {
        ERROR("File open error %d", res);
        ui.set_status((const char *)"File open error", true);
        ui.update();
        mks_wifi_sd_deinit();

        WIFI_IO1_SET();
        delay(2500);
        WIFI_IO1_RESET();
        return;
    }

#if ENABLED(TFT_480x320) || ENABLED(TFT_480x320_SPI)
    mks_upload_screen(file_name + 3, file_size);
#if ENABLED(SHOW_PROGRESS)
    mks_update_status(file_name + 3, 0, file_size);
#endif
#endif

    file_size_writen = 0; //Счетчик записанных в файл данных
#if ENABLED(SHOW_PROGRESS)
    old_file_size_writen = 0;
#endif

    WIFI_IO1_SET();
    MYSERIAL2.begin(1958400);
    USART1->CR1 &= ~USART_CR1_RXNEIE; //禁用UART RXNE中断
    USART1->CR3 = USART_CR3_DMAR;     //开启RXDMA

    RCC->AHBENR |= RCC_AHBENR_DMA1EN; //打开DMA1时钟

    DMA1_Channel5->CCR = DMA_CONF;
    DMA1_Channel5->CCR |= DMA_CCR_HTIE; //开启传输过半中断
    DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;
    DMA1_Channel5->CMAR = (uint32_t)dma_buff;
    DMA1_Channel5->CNDTR = ESP_PACKET_SIZE;
    DMA1->IFCR = DMA_IFCR_CGIF5 | DMA_IFCR_CTEIF5 | DMA_IFCR_CHTIF5 | DMA_IFCR_CTCIF5;

    NVIC_SetPriority(DMA1_Channel5_IRQn, 1); //设置DMA1中断优先级
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);      //开启DMA1中断
    DMA1_Channel5->CCR |= DMA_CCR_EN;        //开启DMA1
    WIFI_IO1_RESET();

    while (dma_timeout > 0)
    {
        TERN_(USE_WATCHDOG, HAL_watchdog_refresh());
        dma_timeout--;

        if (1 == dma_state)
        {
            dma_state = 0;
         
            if (0x80 == dma_buff[7])
            {
                f_sync((FIL *)&upload_file);
                f_close((FIL *)&upload_file);

                DMA1_Channel5->CCR &= ~DMA_CCR_EN;
                MYSERIAL2.begin(115200);
                WIFI_IO1_RESET();
                break;
            }

            DMA1_Channel5->CCR &= ~DMA_CCR_EN; //关闭通道
            DMA1_Channel5->CNDTR = 0;          //清零通道计数器
            DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;
            DMA1_Channel5->CMAR = (uint32_t)dma_buff;
            DMA1_Channel5->CNDTR = ESP_PACKET_SIZE;
            DMA1->IFCR = DMA_CLEAR;
            DMA1_Channel5->CCR |= DMA_CCR_EN; //使能通道

            WIFI_IO1_RESET();

            dma_timeout = DMA_TIMEOUT;
        }           
    }

    if (file_size_writen != 0 && dma_timeout > 0 )
    {
        TERN_(USE_WATCHDOG, HAL_watchdog_refresh());
        mks_wifi_sd_deinit();
        TERN_(USE_WATCHDOG, HAL_watchdog_refresh());
#if ENABLED(TFT_480x320) || ENABLED(TFT_480x320_SPI)
        mks_end_transmit();
#endif
        ui.set_status((const char *)"Upload done", true);
        BUZZ(1000, 260);
    }
    else
    {
        DMA1_Channel5->CCR &= ~DMA_CCR_EN;
        MYSERIAL2.begin(115200);
        WIFI_IO1_RESET();

#if ENABLED(TFT_480x320) || ENABLED(TFT_480x320_SPI)
        mks_end_transmit();
#endif
        ui.set_status((const char *)"Upload failed", true);

        file_failed_count++;
        sprintf(file_failed_name, "file_failed_%d.gcode", file_failed_count);
        f_rename(file_name, file_failed_name);

        TERN_(USE_WATCHDOG, HAL_watchdog_refresh());
        mks_wifi_sd_deinit();

        BUZZ(436, 392);
        BUZZ(109, 0);
        BUZZ(436, 392);
        BUZZ(109, 0);
        BUZZ(436, 392);
    }

    TERN_(USE_WATCHDOG, HAL_watchdog_refresh());
    thermalManager.setTargetBed(0);
    thermalManager.setTargetHotend(0, 0);
}

#ifdef STM32F1
extern "C" void DMA1_Channel5_IRQHandler(void)
{
    if (DMA1->ISR & DMA_ISR_TEIF5)
    {
        dma_buff[7] = 0x80;
        dma_state = 1;
        file_size_writen = 0;
    }

    if (DMA1->ISR & DMA_ISR_HTIF5)
    {
        WIFI_IO1_SET();
    }

    if (DMA1->ISR & DMA_ISR_TCIF5)
    {
        dma_state = 1;
        res = f_write((FIL *)&upload_file, (uint8_t *)&dma_buff[8], 1015, &writeLen);

        if (FR_OK == res && dma_buff[0] == 0xA5 && dma_buff[1] == 0x03)
        {
            file_size_writen++;
            DMA1_Channel5->CCR &= ~DMA_CCR_EN; //关闭通道
        }
        else
        {
            file_size_writen = 0;
            dma_buff[7] = 0x80;
        }
    }

    DMA1->IFCR = DMA_CLEAR;
}
#endif

#endif