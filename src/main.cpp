#include <credentials.h>
#include <Arduino.h>
#include <WiFi.h> 
#include <ArduinoOTA.h>
#include "knoepfe.h" //Graphic data for buttons
#include "Adafruit_ILI9341.h" //Display driver
#include <XPT2046_Touchscreen.h> //Touchscreen driver
#include <TouchEvent.h> //Examines touchscreen events
#include "audio.h"
#include "tft_display.h"
#include "stations.h"
#include "wlan.h"
#include "ota.h"
#include "websrvr.h"


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
Preferences pref;
Preferences sender;

//structure for station list
typedef struct {
  char url[150];  //stream url
  char name[32];  //stations name
  uint8_t enabled;//flag to activate the station
} Station;

#define STATIONS 30 //number of stations in the list

//gloabal variables
uint8_t             _resetResaon = (esp_reset_reason_t) ESP_RST_UNKNOWN;

Station stationlist[STATIONS];    //list of available stations
//variables to hold configuration data
String ssid = MY_SSID;            //ssid for WLAN connection
String pkey = MY_PKEY;            //passkey for WLAN connection
String ntp = "de.pool.ntp.org";   //NTP server url
uint8_t curStation = 0;           //index for current selected station in stationlist
uint8_t curGain = 200;            //current loudness
uint8_t snoozeTime = 30;          //snooze time in minutes
uint16_t alarmDuration = 30;      //duration of alarm in minutes without interaction until it's autmatically turned off.
boolean alarmsActive = false;     //flag if all alarms are active or not
boolean alarm1Active  = false;    //flag if first alarm is active or not
uint16_t alarm1 = 390;            //first alarm time 6:30
uint8_t alarmday1 = 0B00111110;   //valid weekdays (example 00111110 means monday through friday)
boolean alarm2Active  = false;    //flag if second alarm is active or not
uint16_t alarm2 = 480;            //second alarm time 8:00
uint8_t alarmday2 = 0B01000001;   //valid week days (example 01000001 means sunday and saturday)
uint8_t actStation = 0;           //index for current station in station list used for streaming 
uint8_t bright = 25;              //brightness in percent. 0 means use LDR to control brightness
//other global variables
uint32_t lastchange = 0;          //time of last selection change
uint8_t snoozeWait = 0;           //remaining minutes until radio is turned off due to snooze
uint16_t alarmtime = 0;           //next relevant alarm time
uint8_t alarmday = 8;             //weekday for next relevant alarm or 8 means alarm disabled
char title[64];                   //character array to hold meta data message
bool newTitle = false;            //flag to signal a new title
uint32_t tick = 0;                //last tick-counter value to trigger timed event
uint32_t discon = 0;              //tick-counter value to calculate disconnected time
uint16_t minutes;                 //current number of minutes since midnight
uint8_t weekday;                  //current weekday
struct tm ti;                     //time structure with current time
int16_t lastldr;                  //last value from LDR sensor to detect changes
uint32_t start_conf;              //time of entering config screen
boolean connected;                //flag to signal active connection
boolean radio = false;            //flag to signal radio output
boolean clockmode = true;         //flag to signal clock is shown on the screen
boolean alarmTripped = false;     //flag to signal that an alarm has started radio playback
uint8_t alarmRestartWait = 0;     //remaining minutes until radio is restarted due to alarm-snooze

//predefined function from modul tft_display.ino
void displayMessage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* text, uint8_t align = ALIGNLEFT, boolean big = false, uint16_t fc = ILI9341_WHITE , uint16_t bg = ILI9341_BLACK, uint8_t lines = 1 );
void findNextAlarm();  //main.cpp
void displayAlarmState();  //tft_display.cpp
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

//calculate date and time for next expected alarm
//set alarmtime to the time on the day and alarmday on weekday for alarm
//or 8 if no alarm exists
void findNextAlarm() {
  Serial.println("Search next alarm time");
  int wd; 
  uint8_t mask;
  if (getLocalTime(&ti)) { //get current date and time
    wd = weekday; //variable to iterate over weekdays starting with today
    alarmday = 8;  //alarmday = 8 means no alarm
    mask = 1 << wd;  //mask to filter weekday to be tested
    // Serial.printf("alarmday = %i, wd = %i, minutes = %i,  alarm1 = %i, alarm2 = %i, AlarmsActive = %s, alarm1Active = %s, alarm2Active = %s\n",alarmday, wd, minutes, alarm1,alarm2, alarmsActive ? "true" : "false", alarm1Active ? "true" : "false", alarm2Active ? "true" : "false");

    //Search current weekday for future alarm:
    //Test if alarm1 is active.
    if (alarm1Active){
      //Test if alarm1 is set for current weekday in the future.
      if ((alarmday == 8) && ((alarmday1 & mask) != 0) && (alarm1 > minutes) ) { alarmtime = alarm1; alarmday = wd;}
    }
    //test if alarm2 is active.
    if (alarm2Active){
      //test if no alarm1 was found today in the future but alarm2 is set for today in the future or
      //     if alarm1 was found for today in the future but an earlier alarm2 is also set for today in the future. 
      if (((alarmday == 8) && ((alarmday2 & mask) != 0) && (alarm2 > minutes) ) ||
          ((alarmday != 8) && ((alarmday2 & mask) != 0) && (alarm2 > minutes) && (alarm2 < alarm1)))
        { alarmtime = alarm2; alarmday = wd;}
    }

    //if no future alarms were found today, search the next weekdays until weekday before today:
    if (alarmday == 8) { 
      //No future alarm was found for today. Search the next weekdays until weekday before today.
      do {
        wd++; //next weekday
        if (wd > 7) wd = 0; //In case we went from Saturday (wd=7) to Sunday (wd=0), reset wd from 8 back to 0. 
        mask = 1 << wd;

        //Test if alarm1 is active.
        if (alarm1Active){
          //Alarm1 is active.

          //Test if alarm1 is set for this day.
          if ((alarmday == 8) && ((alarmday1 & mask) != 0) ) { alarmtime = alarm1; alarmday = wd;}
        }
        //Test if alarm2 is active.
        if (alarm2Active){
          //Alarm2 is active.

          //Test if alarm1 was not found this wekday but alarm2 is set for this weekday or
          //     if alarm1 was found this weekday but alarm alarm2 is also set for this weekday at an earlier time than alarm1.
          if (((alarmday == 8) && ((alarmday2 & mask) != 0)) ||
              ((alarmday != 8) && ((alarmday2 & mask) != 0) && (alarm2 < alarm1))) 
            { alarmtime = alarm2; alarmday = wd;}
        }
      } while ((alarmday == 8) && (wd != weekday)); //continue until an valid alarm was found or a week is over
    }
    
    //If still no alarm, search for alarms today in the past:
    if (alarmday == 8){
        //Note: wd is back to current weekday due to exit criteria of do loop before.

        //Test if alarm1 is active.
        if (alarm1Active){
          //Alarm1 is active:
          //Test if any alarm1 is set for today.
          if ((alarmday == 8) && ((alarmday1 & mask) != 0) ) { alarmtime = alarm1; alarmday = wd;}
        }
        //Test if alarm2 is active.
        if (alarm2Active){
          //Alarm2 is active:
          //Test if no alarm1 is set today but some alarm2 is set for today or
          //     if some alarm1 is set today but alarm alarm2 is also set today but at an earlier time.
          if (((alarmday == 8) && ((alarmday2 & mask) != 0)) ||
              ((alarmday != 8) && ((alarmday2 & mask) != 0) && (alarm2 < alarm1))) 
            { alarmtime = alarm2; alarmday = wd;}
        }
    }
    Serial.printf("Next alarm %i on %i\n",alarmtime,alarmday);
  }
}

//setup
void setup() {
  #ifdef BUILD_TYPE_DEV //Build type for development
  sleep(5);
  #endif
  Serial.begin(115200);
  //Serial.printf("ssid = %s, pkey= %s \n", ssid.c_str(), pkey.c_str());
  Serial.println("Load preferences");
  Serial.printf("Total heap: %d\n", ESP.getHeapSize());
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

  // Display reset reason (from Schreibfaul1)
  const char* rr = NULL;
  _resetResaon = esp_reset_reason();
  switch(_resetResaon){
      case ESP_RST_UNKNOWN:    rr = "Reset reason can not be determined"; break;
      case ESP_RST_POWERON:    rr = "Reset due to power-on event"; break;
      case ESP_RST_EXT:        rr = "Reset by external pin (not applicable for ESP32)"; break;
      case ESP_RST_SW:         rr = "Software reset via esp_restart"; break;
      case ESP_RST_PANIC:      rr = "Software reset due to exception/panic"; break;
      case ESP_RST_INT_WDT:    rr = "Reset (software or hardware) due to interrupt watchdog"; break;
      case ESP_RST_TASK_WDT:   rr = "Reset due to task watchdog"; break;
      case ESP_RST_WDT:        rr = "Reset due to other watchdogs"; _resetResaon = 1; break;
      case ESP_RST_DEEPSLEEP:  rr = "Reset after exiting deep sleep mode"; break;
      case ESP_RST_BROWNOUT:   rr = "Brownout reset (software or hardware)"; break;
      case ESP_RST_SDIO:       rr = "Reset over SDIO"; break;
  }
  Serial.printf("RESET_REASON: %s", rr);
  Serial.print("\n\n");
  
  title[0] = 0;
  //preferences will be saved in the EPROM of the ESP32 to keep the values 
  //when power supply will be interrupted
  //Two topics "radio" and "senderliste" are defined. We will mount both
  pref.begin("radio", false);
  sender.begin("senderlist",false);
  //get values from preferences
  if (pref.isKey("ssid")) ssid = pref.getString("ssid");
  if (pref.isKey("pkey")) pkey = pref.getString("pkey");
  if (pref.isKey("ntp")) ntp = pref.getString("ntp");
  curGain = 50; //default value
  if (pref.isKey("gain")) curGain = pref.getUShort("gain");
  snoozeTime = 30; //default value
  if (pref.isKey("snooze")) snoozeTime = pref.getUShort("snooze");
  alarmDuration = 30; //default value
  if (pref.isKey("aladur")) alarmDuration = pref.getUInt("aladur");
  bright = 80; //default value
  if (pref.isKey("bright")) bright = pref.getUShort("bright");
  if (pref.isKey("alarmsActive")) alarmsActive = pref.getBool("alarmsActive");
  if (pref.isKey("alarm1Active")) alarm1Active = pref.getBool("alarm1Active");
  if (pref.isKey("alarm2Active")) alarm2Active = pref.getBool("alarm2Active");
  alarm1 = 390;    //6:30
  if (pref.isKey("alarm1")) alarm1 = pref.getUInt("alarm1");
  alarmday1 = 0B00111110; //mo-fr
  if (pref.isKey("alarmday1")) alarmday1 = pref.getUShort("alarmday1");
  alarm2 = 480;    //8:00
  if (pref.isKey("alarm2")) alarm2 = pref.getUInt("alarm2");
  alarmday2 = 0B01000001; //so,sa
  if (pref.isKey("alarmday2")) alarmday2 = pref.getUShort("alarmday2");
  alarmtime = 0;
  alarmday = 8; //alarm is off
  curStation = 0; //default value
  //set current station to saved value if available
  if (pref.isKey("station")) curStation = pref.getUShort("station");
  if (curStation >= STATIONS) curStation = 0; //check to avoid invalid station number
  actStation = curStation;   //set active station to current station 
  Serial.printf("station %i, gain %i, ssid %s, ntp %s\n", curStation, curGain, ssid.c_str(), ntp.c_str());
  //run setup functions in the sub parts
  setup_audio(); //setup audio streams
  setup_display(); //setup display interface
  setup_senderList(); //load station list from preferences
  setGain(0); //set the current gain
  //Try to connect WLAN show progress on display 
  displayClear();
  displayMessage(5, 10, 310, 30, "Connect WLAN", ALIGNCENTER, true, ILI9341_YELLOW, ILI9341_BLACK,1);
  displayMessage(5, 50, 310, 30, ssid.c_str(), ALIGNCENTER, true, ILI9341_GREEN, ILI9341_BLACK,1);
  Serial.println("Connect WiFi");
  connected = initWiFi(ssid, pkey);
  if (connected) { //successful connection
    //setup real time clock
    configTzTime("CET-1CEST,M3.5.0/03,M10.5.0/03", ntp.c_str());
    //show date and time and the name of the station
    delay(500);
    //fill timestarukture ti, minutes and weekday with now
    getLocalTime(&ti);
    minutes = ti.tm_hour * 60 + ti.tm_min;
    weekday = ti.tm_wday;
    Serial.println("Start");
    findNextAlarm(); //Setup: Find next alarm.
    showClock(); //Setup: Display time and alarm state.
    
  } else { //connection not successful
    //we have no connection. A message will be shown on the display
    displayClear();
    displayMessage(5, 10, 310, 30, "Not connected", ALIGNCENTER, true, ILI9341_RED, ILI9341_BLACK,1);
    displayMessage(5, 60, 310, 30, "connect to SSID radioweckerconf", ALIGNCENTER, false, ILI9341_WHITE, ILI9341_BLACK,1);
    displayMessage(5, 90, 310, 30, "Configuration on IP: 192.168.4.1", ALIGNCENTER, false, ILI9341_WHITE, ILI9341_BLACK,1);
    //remember current tick count to allow
    //a retry after five minutes disconnected
    discon = millis();
  }
  Serial.println("Start webserver");
  //setup the web server and the over the air update
  setup_webserver();
  setup_ota();
  //remember the tick count for the timed event
  tick = millis();
  //subtract no of current seconds from tick count to get first time update on the minute.
  if (connected) tick = tick - (ti.tm_sec * 1000);
  start_conf = 0;
}


void loop() {
  //Check over the air update
  ArduinoOTA.handle();
  //Check for http requests
  webserver_loop();
  //check for touch events
  touch_loop();
  //after 10 seconds switch back from config screen to clock screen
  if (!clockmode && ((millis() - start_conf) > 10000)) {
    clockmode = true; //Display timeout: Switch bach to clock mode.
    showClock(); //Display timeout: Show clock and alarm state.
  }
  //show metadata if radio is active
  if (newTitle && radio && clockmode) {
    showTitle();
    newTitle = false;
  }
  //detect a disconnect
  if (connected && (WiFi.status() != WL_CONNECTED)){
    if (connected) {
      //remember tick count for automatic retry
      //MyAir Note: Only set discon time once when first disconnect is deteced.
      discon = millis();
    }
    //MyAir Note: set connected to false AFTER discon time has been saved once.
    connected = false;
    //show message on display
    displayClear();
    displayMessage(5, 10, 310, 30, "Connection lost", ALIGNCENTER, true, ILI9341_RED, ILI9341_BLACK,1);
  }
  //if radio is on get new stream data
  if (connected && radio) {
    audio_loop();
  }
  //if brightness is set to 0, brightness will be controlled by ambient light
  if (bright == 0) {
    int16_t tmp = analogRead(LDR);
    int16_t diff = lastldr - tmp;
    diff = abs(diff);
    //set BG light if clock is displayed
    if (clockmode && (diff > 50)) {
      setBGLight(bright);
      Serial.printf("ldr %i letzter %i\n",tmp,lastldr);
      //remember current ambient light to detect changes
      lastldr = tmp;
    }
  } 
  //timed event updatetime display every minute  
  if ((millis() - tick) > 60000) {
    tick = millis();
    //get date and time information
    if (connected  && getLocalTime(&ti)) {
      minutes = ti.tm_hour * 60 + ti.tm_min;
      weekday = ti.tm_wday;
    }
    //set BG light if clock is displayed
    if (connected && clockmode) {
      setBGLight(bright);
      displayDateTime();
    }

    // Snooze Timer: Turn radio off once snoozeWait is back to zero or display message at what time it's being turend off.
    if (snoozeWait > 0) { //Radio should be turned off soon:
      snoozeWait--; //Decrease snoonzeWait by 1 minute.
      if (snoozeWait == 0) { //Radio should be turned off now:
        toggleRadio(true); //snoozeWait timeout: Turn radio off.
        if (alarmTripped) { //Radio was turned off due to alarm timeout:
          // Clear alarm flags.
          alarmTripped = false; // snoozeWait alarm-timeout: Clear alarmTripped in case alarm timeout has been reached.
          alarmRestartWait = 0; // snoozeWait alarm-timeout: clear alarm-snooze time.
          findNextAlarm(); //snoozeWait alarm-timeout: Search for next alarm.
        }
        showClock(); //snoozeWait: Display clock and alarm state.
      } else { //Radio should be turned off but not yet:
        displayAlarmState(); //snoozeWait: Display alarm state.
      }
    }

    //Alarm-Restart timer: Restart alarm if alarm-snooze is active:
    if (alarmRestartWait > 0) { //The alarm should be restarted soon:
      alarmRestartWait--; //Decrease restart timer by 1 minute.
      if (alarmRestartWait == 0) { //Alarm shoud be restarted now:
        snoozeWait = alarmDuration; //alarmRestartWait: Set snooze time for auto-turnOff.
        toggleRadio(false); // alarmRestartWait: Turn radio back on.
        showClock();  //Display clock, radio and alarm state.
      } else { //Alarm shoud be restarted but not yet:
        displayAlarmState(); //alarmStateWait: Display when alarm is reactivated.
      }
    }

    //Alarm trigger:
    //if alrms are active and an active alarm was found, check for day and time.
    if ((alarmsActive) && (alarmday < 8) && getLocalTime(&ti)) {
      //if alarm day and time is reached turn radio on and get values for next expected alarm
      if ((alarmday == weekday) && (minutes == alarmtime)) {
        // Set flag that alrm is tripped.
        alarmTripped = true;
        // Set snooze Timer so alarm does not sound longer than defined alarm duration.
        snoozeWait = alarmDuration;
        // Turn on radio.
        toggleRadio(false); // Alarm tripped. Turn radio on.
        showClock();
      }
    }
  } //end if 1 minute timed events.

  //do a restart if device was disconnected for more then 5 minutes
  if (!connected && ((millis() - discon) > 300000)) ESP.restart();
}
