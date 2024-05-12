#ifndef ARDUINO_H
#define ARDUINO_H
#include <Arduino.h>
#endif

#ifndef COMMON_H
#define COMMON_H
#include <WiFi.h> 
#include <ArduinoOTA.h>
#include "knoepfe.h" //Graphic data for buttons
#include "Adafruit_ILI9341.h" //Display driver
#include <XPT2046_Touchscreen.h> //Touchscreen driver
#include <TouchEvent.h> //Examines touchscreen events
#include <rtime.h>
#include "tft_display.h"
#include "stations.h"
#include "wlan.h"
#include "ota.h"
#include "websrvr.h"
#define AUDIOTASK_CORE      0                               // 0 or 1
#define AUDIOTASK_PRIO      2                               // 0 ... 24  Priority of the Task (0...configMAX_PRIORITIES -1)
#define I2S_COMM_FMT        0                               // (0) MAX98357A PCM5102A CS4344, (1) LSBJ (Least Significant Bit Justified format) PT8211

#define I2S_DOUT         18
#define I2S_BCLK          8
#define I2S_LRC          17
#define I2S_MCLK          0


//esp32 library to save preferences in flash
#include <Preferences.h>

//definitions for text output alignment
#define ALIGNLEFT 0
#define ALIGNCENTER 1
#define ALIGNRIGHT 2

//pin to be used for light sensor
//#define LDR 36
#define LDR 2

//instance of prefernces
extern Preferences pref;
extern Preferences sender;

//structure for station list
typedef struct {
  char url[150];  //stream url
  char name[32];  //stations name
  uint8_t enabled;//flag to activate the station
} Station;

#define STATIONS 30 //number of stations in the list
#endif //COMMON_H

//gloabal variables
extern Station stationlist[STATIONS];    //list of available stations
//variables to hold configuration data
extern String ssid;                 //ssid for WLAN connection
extern String pkey;                 //passkey for WLAN connection
extern String ntp;                  //NTP server url
extern uint8_t curStation ;         //index for current selected station in stationlist
extern uint8_t curGain ;            //current loudness
extern float_t fadeGain ;           //current volume while fading
extern float_t fadeInStep ;         //Steps by how much the volume is in-/de-creased every second.
extern boolean fadeIn ;             //Flag to fade in the music
extern float_t fadeOutStep ;        //Steps by how much the volume is in-/de-creased every second.
extern boolean fadeOut ;            //Flag to fade out the music
extern uint16_t fadeTimer ;         //Countdown timer for fade-in/out
extern uint8_t volumeSet ;          //Volume to be set
extern uint8_t snoozeTime ;         //snooze time in minutes
extern uint16_t fadeInTime ;        //Ramp-Up time in seconds to reach max volume
extern uint16_t fadeOutTime ;       //Ramp-Down time in seconds to reach max volume
extern uint16_t alarmDuration ;     //duration of alarm in minutes without interaction until it's autmatically turned off.
extern boolean alarmsActive ;       //flag if alarms are turend on or off
extern boolean alarm1Active ;       //flag if first alarm is active or not
extern uint16_t alarm1 ;            //first alarm time 6:30
extern uint8_t alarmday1;           //valid weekdays (example 00111110 means monday through friday)
extern boolean alarm2Active ;       //flag if second alarm is active or not
extern uint16_t alarm2 ;            //second alarm time 8:00
extern uint8_t alarmday2;           //valid week days (example 01000001 means sunday and saturday)
extern uint8_t actStation;          //index for current station in station list used for streaming 
extern uint8_t bright ;             //brightness in percent. 0 means use LDR to control brightness
//other global variables
extern uint32_t lastchange;         //time of last selection change
extern uint8_t snoozeWait ;         //remaining minutes until radio is turned off due to snooze
extern uint16_t alarmtime ;         //next relevant alarm time
extern uint8_t alarmday ;           //weekday for next relevant alarm or 8 means alarm disabled
extern char title[64];              //character array to hold meta data message
extern bool newTitle ;              //flag to signal a new title
extern uint32_t tick ;              //last tick-counter value to trigger timed event every 60 seconds
extern uint32_t secTick ;           //last tick-counter value to trigger timed event every second
extern uint32_t discon;             //tick-counter value to calculate disconnected time
extern uint16_t minutes;            //current number of minutes since midnight
extern uint8_t weekday;             //current weekday
extern struct tm ti;                //time structure with current time
extern int16_t lastldr;             //last value from LDR sensor to detect changes
extern uint32_t start_conf;         //time of entering config screen
extern boolean connected;           //flag to signal active connection
extern boolean radio ;              //flag to signal radio output
extern boolean clockmode;           //flag to signal clock is shown on the screen
extern boolean alarmTripped;        //flag to signal that an alarm has started radio playback
extern uint8_t alarmRestartWait;    //remaining minutes until radio is restarted due to alarm-snooze

//predefined function from modul tft_display.ino
void displayMessage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* text, uint8_t align = ALIGNLEFT, boolean big = false, uint16_t fc = ILI9341_WHITE , uint16_t bg = ILI9341_BLACK, uint8_t lines = 1 );
void findNextAlarm();  //main.cpp
void setGain(uint8_t gain);  // Convert gain into Volume value and set volume
void displayAlarmState();  //tft_display.cpp
void saveList(); //websrvr.cpp
void reorder(uint8_t oldpos, uint8_t newpos);
void setStationData();
void setup_webserver();
void displayClear();
void setBGLight(uint8_t prct);
void showProgress(uint32_t prc);
void toggleRadio(boolean off, boolean ramp = false);
void showRadio();

//Schreibfaul Commons:


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// output on serial terminal
#define ANSI_ESC_BLACK      "\033[30m"
#define ANSI_ESC_RED        "\033[31m"
#define ANSI_ESC_GREEN      "\033[32m"
#define ANSI_ESC_YELLOW     "\033[33m"
#define ANSI_ESC_BLUE       "\033[34m"
#define ANSI_ESC_MAGENTA    "\033[35m"
#define ANSI_ESC_CYAN       "\033[36m"
#define ANSI_ESC_WHITE      "\033[37m"
#define ANSI_ESC_BROWN      "\033[38;5;130m"
#define ANSI_ESC_ORANGE     "\033[38;5;214m"

#define ANSI_ESC_RESET      "\033[0m"
#define ANSI_ESC_BOLD       "\033[1m"
#define ANSI_ESC_FAINT      "\033[2m"
#define ANSI_ESC_ITALIC     "\033[3m"
#define ANSI_ESC_UNDERLINE  "\033[4m"

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static bool _newLine = false;
extern SemaphoreHandle_t mutex_rtc;
extern RTIME rtc;
#define SerialPrintfln(...) {xSemaphoreTake(mutex_rtc, portMAX_DELAY); \
 /* line feed */            if(_newLine){_newLine = false; Serial.println("");} \
                            Serial.printf("%s ", rtc.gettime_s()); \
                            Serial.printf(__VA_ARGS__); \
                            Serial.printf("\033[0m"); \
                            Serial.println(""); \
                            xSemaphoreGive(mutex_rtc);}

#define SerialPrintfcr(...) {xSemaphoreTake(mutex_rtc, portMAX_DELAY); \
 /* carriage return */      Serial.printf("%s ", rtc.gettime_s()); \
                            Serial.printf(__VA_ARGS__); \
                            Serial.printf("\033[0m"); \
                            Serial.print("  \r"); \
                            _newLine = true; \
                            xSemaphoreGive(mutex_rtc);}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//prototypes (audiotask.cpp)
void           audioInit();
void           audioTaskDelete();
void           audioSetVolume(uint8_t vol);
uint8_t        audioGetVolume();
uint32_t       audioGetBitRate();
boolean        audioConnecttohost(const char* host, const char* user = "", const char* pwd = "");
boolean        audioConnecttoFS(const char* filename, uint32_t resumeFilePos = 0);
uint32_t       audioStopSong();
void           audioSetTone(int8_t param0, int8_t param1, int8_t param2, int8_t param3);
uint32_t       audioInbuffFilled();
uint32_t       audioInbuffFree();
uint32_t       audioInbuffSize();
boolean        audioIsRunning();
uint32_t       audioGetStackHighWatermark();
uint32_t       audioGetCodec();
boolean        audioPauseResume();
void           audioConnectionTimeout(uint32_t timeout_ms, uint32_t timeout_ms_ssl);
uint32_t       audioGetFileSize();
uint32_t       audioGetFilePosition();
uint16_t       audioGetVUlevel();
uint32_t       audioGetFileDuration();
uint32_t       audioGetCurrentTime();

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline int32_t str2int(const char* str) {
    int32_t len = strlen(str);
    if(len > 0) {
        for(int32_t i = 0; i < len; i++) {
            if(!isdigit(str[i])) {
                log_e("NaN");
                return 0;
            }
        }
        // return stoi(str);
        return atoi(str);
    }
    return 0;
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline bool startsWith(const char* base, const char* searchString) {
    char c;
    while((c = *searchString++) != '\0')
        if(c != *base++) return false;
    return true;
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline bool endsWith(const char* base, const char* searchString) {
    int32_t slen = strlen(searchString);
    if(slen == 0) return false;
    const char* p = base + strlen(base);
     while(p > base && isspace(*p)) p--;  // rtrim
    p -= slen;
    if(p < base) return false;
    return (strncmp(p, searchString, slen) == 0);
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline int32_t indexOf(const char* haystack, const char* needle, int32_t startIndex) {
    const char* p = haystack;
    for(; startIndex > 0; startIndex--)
        if(*p++ == '\0') return -1;
    char* pos = strstr(p, needle);
    if(pos == nullptr) return -1;
    return pos - haystack;
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline void SerialPrintflnCut(const char* item, const char* color, const char* str) {
    uint8_t maxLength = 100;
    if(strlen(str) > maxLength) {
        String f = str;
        SerialPrintfln("%s%s%s ... %s", item, color, f.substring(0, maxLength - 25).c_str(), f.substring(f.length() - 20, f.length()).c_str());
    }
    else { SerialPrintfln("%s%s%s", item, color, str); }
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————


#pragma once