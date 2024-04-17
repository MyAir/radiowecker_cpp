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
extern uint8_t snoozeTime ;          //snooze time in minutes
extern uint16_t alarm1 ;            //first alarm time 6:30
extern uint8_t alarmday1;   //valid weekdays (example 00111110 means monday through friday)
extern uint16_t alarm2 ;            //second alarm time 8:00
extern uint8_t alarmday2;   //valid week days (example 01000001 means sunday and saturday)
extern uint8_t actStation;           //index for current station in station list used for streaming 
extern uint8_t bright ;              //brightness in percent. 0 means use LDR to control brightness
//other global variables
extern uint32_t lastchange;          //time of last selection change
extern uint8_t snoozeWait ;           //remaining minutes fro snooze
extern uint16_t alarmtime ;           //next relevant alarm time
extern uint8_t alarmday ;             //weekday for next relevant alarm or 8 means alarm disabled
extern char title[64];                   //character array to hold meta data message
extern bool newTitle ;            //flag to signal a new title
extern uint32_t tick ;                //last tick-counter value to trigger timed event
extern uint32_t discon;              //tick-counter value to calculate disconnected time
extern uint16_t minutes;                 //current number of minutes since midnight
extern uint8_t weekday;                  //current weekday
extern struct tm ti;                     //time structure with current time
extern int16_t lastldr;                  //last value from LDR sensor to detect changes
extern uint32_t start_conf;              //time of entering config screen
extern boolean connected;                //flag to signal active connection
extern boolean radio ;            //flag to signal radio output
extern boolean clockmode;         //flag to signal clock is shown on the screen

//predefined function from modul tft_display.ino
void displayMessage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* text, uint8_t align = ALIGNLEFT, boolean big = false, uint16_t fc = ILI9341_WHITE , uint16_t bg = ILI9341_BLACK, uint8_t lines = 1 );
void findNextAlarm();  //main.cpp
void showNextAlarm();  //tft_display.cpp
void saveList(); //websrvr.cpp
void reorder(uint8_t oldpos, uint8_t newpos);
void stopPlaying();
bool startUrl(String url);
void setStationData();
void setup_webserver();
void setGain(float gain);
void displayClear();
void setBGLight(uint8_t prct);
void showProgress(uint32_t prc);
void toggleRadio(boolean off);
void showRadio();
#pragma once