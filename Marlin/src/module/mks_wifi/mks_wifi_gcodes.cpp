#include "mks_wifi_gcodes.h"
#include "../../module/printcounter.h"
#include "../../libs/duration_t.h"
#include "../../libs/buzzer.h"
#include "../../feature/pause.h"
#include "../../module/motion.h"
#include "../../feature/powerloss.h"
#ifdef MKS_WIFI

const uint8_t pak[5] = {0xA5, 0x07, 0x00, 0x00, 0xFC};
char responseCmd[] = "ok\r\n";

const char m997_idle[] = "M997 IDLE\r\n";
const char m997_printing[] = "M997 PRINTING\r\n";
const char m997_pause[] = "M997 PAUSE\r\n";

const char m115_firmware[] = "FIRMWARE_NAME:TFT24\r\n";

char filenameOpen[50] = {0};

void mks_m991(void)
{
    char tempBuf[100];
    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    memset(tempBuf, 0, 100);

    sprintf((char *)tempBuf, "T:%d /%d B:%d /%d T0:%d /%d T1:0 /0 @:0 B@:0\r\n",
            (int)Temperature::degHotend(target_extruder), Temperature::degTargetHotend(target_extruder),
            (int)Temperature::degBed(), Temperature::degTargetBed(),
            (int)Temperature::degHotend(target_extruder), Temperature::degTargetHotend(target_extruder));

    mks_wifi_out_add((uint8_t *)tempBuf, strlen(tempBuf));

}

void mks_m992(void)
{
    char buffer[30] = {0};

    uint32_t time = print_job_timer.duration();

    uint16_t h = time / 3600;
    uint16_t m = time / 60;
    uint16_t s = time % 60;

    sprintf(buffer, "M992 %02hu:%02hu:%02hu\r\n", h, m, s);

    mks_wifi_out_add((uint8_t *)buffer, strlen(buffer));

}

void mks_m994(void)
{
    char buffer[100] = {0};

    sprintf(buffer, "M994 %s;%d\n", filenameOpen, card.getFileSize());
    
    mks_wifi_out_add((uint8_t *)buffer, strlen(buffer));
}

void mks_m105(void)
{
    char tempBuf[100];
    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();

    if (target_extruder < 0)
        return;

    mks_wifi_out_add((uint8_t *)responseCmd, strlen(responseCmd));

    memset(tempBuf, 0, 100);

    sprintf((char *)tempBuf, "T:%.1f /%.1f B:%.1f /%.1f T0:%.1f /%.1f T1:0.0 /0.0 @:0 B@:0\r\n",
            Temperature::degHotend(target_extruder), (float)Temperature::degTargetHotend(target_extruder),
            Temperature::degBed(), (float)Temperature::degTargetBed(),
            Temperature::degHotend(target_extruder), (float)Temperature::degTargetHotend(target_extruder));

    mks_wifi_out_add((uint8_t *)tempBuf, strlen(tempBuf));

}

void mks_m997(void)
{
    char tempBuff[30] = {0x00};

    if (printingIsPaused())
    {
        sprintf(tempBuff, "%s", m997_pause);
    }
    else if (printingIsActive())
    {
         sprintf(tempBuff, "%s", m997_printing);
    }
    else
    {
        sprintf(tempBuff, "%s", m997_idle);
    }

    mks_wifi_out_add((uint8_t *)tempBuff, strlen(tempBuff));
}

void mks_m115(void)
{
    mks_wifi_out_add((uint8_t *)m115_firmware, strlen(m115_firmware));
}

void mks_m27(void)
{
    char tempBuff[20] = {0};

    if (printingIsActive())
    {
        sprintf(tempBuff, "M27 %d\r\n", card.percentDone());
    }
    else
    {
        sprintf(tempBuff, "M27 %d\r\n", 0);
    }

    mks_wifi_out_add((uint8_t *)tempBuff, strlen(tempBuff));
}

void mks_m30(char *filename)
{

    filename[0] = '0';
    DEBUG("M30: %s", filename);
    sd_delete_file(filename);

    SERIAL_ECHOPGM(STR_OK);
    SERIAL_EOL();
}

void mks_m23(char *filename)
{
    char dosfilename[30];
    uint8_t dot_pos;

    DEBUG("M23: %s", filename);

    memset(filenameOpen, 0, sizeof(filenameOpen) );
    
    if (filename[1] == ':')
    {
        memcpy(filenameOpen, filename, strlen(filename));
        memcpy(filename, &filename[2], strlen(filename));
    }
    else
    {
        sprintf(filenameOpen, "1:/%s", filename);
    }
    

    if (filename[0] == '/')
    {
        DEBUG("Strip slash");
        for (uint32_t i = 0; i < strlen(filename); i++)
        {
            filename[i] = filename[i + 1];
        }
        DEBUG("Fixed name: %s", filename);
    }

    //Имя файла может быть меньше 12 символов, но с расширением .gcode
    //С конца имени файла шагаем к началу, считаем сколько символов до точки
    dot_pos = 0;
    for (char *fn = (filename + strlen(filename) - 1); fn > filename; --fn)
    {
        dot_pos++;
        if (*fn == '.')
            break;
    }

    if ((strlen(filename) > 12) || (dot_pos > 4))
    {
        DEBUG("Long file name");
        if (get_dos_filename(filename, dosfilename))
        {
            strcpy(CardReader::longFilename, filename); //Для отображения на экране
            DEBUG("DOS file name: %s", dosfilename);
            card.openFileRead(dosfilename);
        }
        else
        {
            ERROR("Can't find dos file name");
        }
    }
    else
    {
        DEBUG("DOS file name");
        card.openFileRead(filename);
    }

    if (card.isFileOpen())
        mks_wifi_out_add((uint8_t *)"File selected\r\n", strlen("File selected\r\n"));
    else
    {
        mks_wifi_out_add((uint8_t *)"file.open failed\r\n", strlen("file.open failed\r\n"));
    }

    mks_wifi_out_add((uint8_t *)responseCmd, strlen(responseCmd));
}

void mks_m20(void)
{
    char tmp[200] = {0};
    char Fstream[200] = {0};
    uint8_t fileCnt = 0;
    char start[] = "Begin file list\r\n";
    char end[] = "End file list\r\n";
    char empty[] = "notFile.gcode\r\n";

    if (!card.isMounted())
        card.mount();

    mks_wifi_out_add((uint8_t *)start, strlen(start));

    if (card.isMounted())
    {
        fileCnt = card.get_num_Files();

        for (size_t i = 0; i < fileCnt; i++)
        {
            card.getfilename_sorted(SD_ORDER(i, fileCnt));
            strcpy(tmp, card.longFilename);
            strcpy(Fstream, tmp);

            if (card.flag.filenameIsDir)
                strcat_P(Fstream, PSTR(".DIR"));
            strcat_P(Fstream, PSTR("\r\n"));

            mks_wifi_out_add((uint8_t *)Fstream, strlen(Fstream));
        }
    }

    delay(3);

    mks_wifi_out_add((uint8_t *)end, strlen(end));

    delay(3);

    mks_wifi_out_add((uint8_t *)responseCmd, strlen(responseCmd));
}

void mks_m25(void)
{
    mks_wifi_out_add((uint8_t *)responseCmd, strlen(responseCmd));

    // Set initial pause flag to prevent more commands from landing in the queue while we try to pause
    #if ENABLED(SDSUPPORT)
      if (IS_SD_PRINTING()) card.pauseSDPrint();
    #endif

    #if ENABLED(POWER_LOSS_RECOVERY) && DISABLED(DGUS_LCD_UI_MKS)
      if (recovery.enabled) recovery.save(true);
    #endif

    print_job_timer.pause();
}

void mks_m26(void)
{
   card.flag.abort_sd_printing = true;
}

#endif
