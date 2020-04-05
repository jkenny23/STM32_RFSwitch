/*
 *  This sketch sends ads1115 current sensor data via HTTP POST request to thingspeak server.
 *  It needs the following libraries to work (besides the esp8266 standard libraries supplied with the IDE):
 *
 *  - https://github.com/adafruit/Adafruit_ADS1X15
 *
 *  designed to run directly on esp8266-01 module, to where it can be uploaded using this marvelous piece of software:
 *
 *  https://github.com/esp8266/Arduino
 *
 *  2015 Tisham Dhar
 *  licensed under GNU GPL
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <EEPROM.h>

#define RFSW_1 PB9
#define RFSW_2 PB8
#define OD_1 PB7
#define OD_2 PB6

#define DEF_LEVEL 0
#define DEF_DELAY 10
#define MIN_DELAY 1
#define MAX_DELAY 650

const char vers[] = "0.1-04052020"; 

//Initialize reference voltage with default until read out of EEPROM
#define RFSWDELAYINIT 50
volatile uint16 RFSWDELAY = RFSWDELAYINIT;
volatile uint32 tick = 0;
uint32 last_tick = 0;
uint32 last_ms_tick = 0;

uint8 level_1 = DEF_LEVEL;
uint16 delay_1 = DEF_DELAY;
uint8 level_2 = DEF_LEVEL;
uint16 delay_2 = DEF_DELAY;
uint8 rfsw_1 = DEF_LEVEL;
uint8 rfsw_2 = DEF_LEVEL;
uint8 rfsw_pos = 1;
uint8 rfsw_last_pos = 2;
uint16 count1 = 0;
uint16 count2 = 0;
uint16 countrf = 0;
uint16 rfswdelay = RFSWDELAY;

//32 chars with /r/n + 8 corrections (e.g. 4 backspace + 4 chars)
#define MAXCHARS 40
char receivedChars[MAXCHARS];
boolean newData = false;

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarkers[] = {'t', '?', 'v', 's', 'o', 'r', 'z'};
  char endMarkers[] = {'\n', '\r'};
  char rc;

  // if (Serial.available() > 0) {
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (rc == '\b')
      Serial.print("\b \b");
    else
      Serial.print(rc);
        
    if (recvInProgress == true) {
      //Serial.println("Receive in progress");
      if (rc == endMarkers[0] || rc == endMarkers[1]){
        //Serial.println("Received endMarker");
        receivedChars[ndx] = '\0'; // terminate the string
        /*Serial.print("ndx=");
          Serial.println(ndx);
          Serial.print("rc=");
          Serial.print(rc);*/
        recvInProgress = false;
        ndx = 0;
        newData = true;
      } 
      else if (rc == '\b') { //Backspace = do not increment character index, do not write new character
        /*Serial.println("Received BS");
        Serial.print("ndx=");
        Serial.println(ndx);
        Serial.print("rc=");
        Serial.print(rc);*/
        if (ndx == 0) {
          ndx = 0;
        }
        else {
          //receivedChars[ndx] = 0;
          ndx--;
        }
      }
      else {
        /*Serial.println("Received normal");
          Serial.print("ndx=");
          Serial.println(ndx);
          Serial.print("rc=");
          Serial.print(rc);*/
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= MAXCHARS) {
          ndx = MAXCHARS - 1;
        }
      }
    }

    else if (rc == startMarkers[0] || rc == startMarkers[1] || rc == startMarkers[2] || rc == startMarkers[3]
             || rc == startMarkers[4] || rc == startMarkers[5] || rc == startMarkers[6]) {
      /*Serial.println("Received startMarker");
        Serial.print("ndx=");
        Serial.println(ndx);
        Serial.print("rc=");
        Serial.print(rc);*/
      recvInProgress = true;
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= MAXCHARS) {
        ndx = MAXCHARS - 1;
      }
    }

    else if (rc == endMarkers[0] || rc == endMarkers[1]) {
      Serial.print("\r\n");
      Serial.print("> ");
    }
  }
}

unsigned int fast_atoi_leading_pos( const char * p )
{
    unsigned int x = 0;
    ++p;
    while (*p >= '0' && *p <= '9') {
        x = (x*10) + (*p - '0');
        ++p;
    }
    return x;
}

int fast_atoi_leading_neg( const char * p )
{
    int x = 0;
    bool neg = false;
    ++p;
    if (*p == '-') {
        neg = true;
        ++p;
    }
    while (*p >= '0' && *p <= '9') {
        x = (x*10) + (*p - '0');
        ++p;
    }
    if (neg) {
        x = -x;
    }
    return x;
}

void printMenu(uint8 menu2) 
{
  switch (menu2) {
    default:
      Serial.print("\r\n");
      Serial.println("> Select Mode:");
      Serial.println(">  RF Switch");
      Serial.println(">   r[1-2]");
      Serial.println(">  Open Drain Output");
      Serial.println(">   o[1-2] l[level: 0 = open, 1 = low] d[delay, x0.1s]");
      Serial.println(">            def.0,                      def.10 (1s)");
      Serial.println(">  Cal R/W Mode");
      Serial.println(">   t[r/w] a[address: 0-999] d[data, unsigned int (0-65535)]");
      Serial.println(">  Version");
      Serial.println(">   v");
      Serial.println(">  Soft Reset");
      Serial.println(">   z");
      Serial.println(">  Help (Prints this menu)");
      Serial.println(">   ?");
      Serial.print("\r\n");
      Serial.print("> ");
      break;
  }
}

void parseOD1(uint8 nArgs, char* args[])
{
  uint8 i;
  int16 strVal = 0;
  
  //Set default charge current/voltage/cutoff
  level_1 = DEF_LEVEL;
  delay_1 = DEF_DELAY;

  Serial.print("\r\n");
  if(nArgs>1)
  {
    for(i=1; i<nArgs; i++)
    {
      switch (args[i][0])
      {
        case 'l':
          Serial.print("> Using level: ");
          strVal = fast_atoi_leading_pos(args[i]);
          if(strVal>1)
            strVal = 1;
          level_1 = strVal;
          Serial.println(level_1);
          break;
        case 'd':
          Serial.print("> Using delay: ");
          strVal = fast_atoi_leading_pos(args[i]);
          if(strVal<MIN_DELAY)
            strVal = MIN_DELAY;
          else if(strVal>MAX_DELAY)
            strVal = MAX_DELAY;
          delay_1 = strVal;
          Serial.print(delay_1*100);
          Serial.println("ms");
          break;
        default:
          break;
      }
    }
  }
  
  if(level_1 == DEF_LEVEL)
  {
    Serial.print("> Using def level: ");
    Serial.println(level_1);
  }
  if(delay_1 == DEF_DELAY)
  {
    Serial.print("> Using def delay: ");
    Serial.print(delay_1*100);
    Serial.println("ms");
  }
}

void parseOD2(uint8 nArgs, char* args[])
{
  uint8 i;
  int16 strVal = 0;
  
  //Set default charge current/voltage/cutoff
  level_2 = DEF_LEVEL;
  delay_2 = DEF_DELAY;

  Serial.print("\r\n");
  if(nArgs>1)
  {
    for(i=1; i<nArgs; i++)
    {
      switch (args[i][0])
      {
        case 'l':
          Serial.print("> Using level: ");
          strVal = fast_atoi_leading_pos(args[i]);
          if(strVal>2)
            strVal = 2;
          level_2 = strVal;
          Serial.println(level_2);
          break;
        case 'd':
          Serial.print("> Using delay: ");
          strVal = fast_atoi_leading_pos(args[i]);
          if(strVal<MIN_DELAY)
            strVal = MIN_DELAY;
          else if(strVal>MAX_DELAY)
            strVal = MAX_DELAY;
          delay_2 = strVal;
          Serial.print(delay_2*100);
          Serial.println("ms");
          break;
        default:
          break;
      }
    }
  }
  
  if(level_2 == DEF_LEVEL)
  {
    Serial.print("> Using def level: ");
    Serial.println(level_2);
  }
  if(delay_2 == DEF_DELAY)
  {
    Serial.print("> Using def delay: ");
    Serial.print(delay_2*100);
    Serial.println("ms");
  }
}

void setup() {
  unsigned short int wData = 0, estatus = 0;
  
  Serial.begin(115200);
  pinMode(RFSW_1,OUTPUT);
  digitalWrite(RFSW_1,LOW);
  pinMode(RFSW_2,OUTPUT);
  digitalWrite(RFSW_2,LOW);
  pinMode(OD_1,OUTPUT);
  digitalWrite(OD_1,LOW);
  pinMode(OD_2,OUTPUT);
  digitalWrite(OD_2,LOW);

  delay(5000);

  Timer2.pause(); // Pause the timer while we're configuring it
  Timer2.setPrescaleFactor(2);
  Timer2.setOverflow(36000); //36 MHz/36000 = 1000 Hz
  //Set up an interrupt on channel 1
  Timer2.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  Timer2.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
  Timer2.attachCompare1Interrupt(handler_loop);
  Timer2.refresh(); //Refresh the timer's count, prescale, and overflow

  estatus = EEPROM.init();
  if(estatus != 0)
  {
    Serial.print("> EEPROM init err: ");
    Serial.println(estatus, HEX);
  }
  
  Serial.println("> Init cal from EEPROM");
  estatus = EEPROM.read(0, &wData);
  if(estatus == 0)
  {
    if(wData > 1000 || wData < 1) //ideal = 1.0556V
    {
      Serial.println("> RF switch delay out of range, using default.");
    }
    else
    {
      Serial.print("> RF switch delay value: 1.0");
      Serial.print(wData);
      Serial.println("ms");
      RFSWDELAY = wData;
    }
  }
  else
  {
    Serial.println("> RF switch delay not found, using default.");
  }
  
  Serial.print("> SW ver: ");
  Serial.println(vers);
  
  Timer2.resume(); //Start the timer counting
}

void loop() {
  uint8 i = 0;
  uint16 estatus = 0, wAddress = 0, wData = 0;
  char *args[8];
  recvWithStartEndMarkers();
  if (newData == true)
  {
    args[i] = strtok(receivedChars, " ");
    while (args[i] != NULL) //i = number of arguments in received string
    {
      args[++i] = strtok(NULL, " ");
    }
    switch (args[0][0]) {
      case 'o':
        if(args[0][1] == '1')
        {
          parseOD1(i, args);
          if(level_1 == 1)
          {
            digitalWrite(OD_1,HIGH);
            count1 = 0;
            Serial.println("\r\n");
            Serial.print("OD1: ");
            Serial.print(level_1);
            Serial.print(", ");
            Serial.print(delay_1*10);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        if(args[0][1] == '2')
        {
          parseOD2(i, args);
          if(level_2 == 1)
          {
            digitalWrite(OD_2,HIGH);
            count2 = 0;
            Serial.println("\r\n");
            Serial.print("OD2: ");
            Serial.print(level_2);
            Serial.print(", ");
            Serial.print(delay_2*10);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        break;
      case 'r':
        if(args[0][1] == '1')
        {
          rfsw_pos = 1;
          Serial.println("\r\n");
          Serial.print("RFSW: ");
          Serial.print(rfsw_pos);
          Serial.print("\r\n> ");
        }
        else if(args[0][1] == '2')
        {
          rfsw_pos = 2;
          Serial.println("\r\n");
          Serial.print("RFSW: ");
          Serial.print(rfsw_pos);
          Serial.print("\r\n> ");
        }
        break;
      case 't':
        if(args[0][1] == 'w')
        {
          Serial.println("\r\n> Writing EEPROM: ");
          //aXXX = address in dec.
          wAddress = fast_atoi_leading_pos(args[1]);
          //dXXX = data in dec.
          wData = fast_atoi_leading_pos(args[2]);
          Serial.print("> Address 0x");
          Serial.print(wAddress, HEX);
          Serial.print(", Data = dec. ");
          Serial.println(wData);
          estatus = EEPROM.write(wAddress, wData);
          if(estatus == 0)
          {
            Serial.print("> Write finished, Status: ");
            Serial.println(estatus, HEX);
          }
          else
          {
            Serial.print("> EEPROM error, code: ");
            Serial.println(estatus, HEX);
          }
        }
        else if(args[0][1] == 'r')
        {
          Serial.println("\r\n> Reading EEPROM: ");
          wAddress = fast_atoi_leading_pos(args[1]);
          Serial.print("> Address 0x");
          Serial.print(wAddress, HEX);
          estatus = EEPROM.read(wAddress, &wData);
          Serial.print(", Data = dec. ");
          Serial.println(wData);
          if(estatus == 0)
          {
            Serial.print("> Read finished, Status: ");
            Serial.println(estatus, HEX);
          }
          else
          {
            Serial.print("> EEPROM error, code: ");
            Serial.println(estatus, HEX);
          }
        }
        break;
      case 'v':        
        Serial.print("\r\n> Software version: ");
        Serial.println(vers);
        Serial.print("\r\n> ");
        break;
      case 's':
        Serial.println("\r\n");
        Serial.print("RFSW: ");
        Serial.print(rfsw_pos);
        Serial.print(", O1: ");
        Serial.print(level_1);
        Serial.print(", O2: ");
        Serial.println(level_2);
        Serial.print("t: ");
        Serial.print(tick);
        Serial.print(", lt: ");
        Serial.print(last_tick);
        Serial.print(", lmt: ");
        Serial.print(last_ms_tick);
        Serial.print(", crf: ");
        Serial.print(countrf);
        Serial.print(", rfsd: ");
        Serial.println(rfswdelay);
        Serial.print("c1: ");
        Serial.print(count1);
        Serial.print(", c2: ");
        Serial.print(count2);
        Serial.print(", d1: ");
        Serial.print(delay_1);
        Serial.print(", d2: ");
        Serial.print(delay_2);
        Serial.print(", rfsd: ");
        Serial.println(rfswdelay);
        Serial.print("\r\n> ");
        break;
      case '?':
        printMenu(0);
        break;
      case 'z':
        //Soft reset
        nvic_sys_reset();
        break;
      default:
        printMenu(0);
        break;
    }

    newData = false;
  }
  //Tenths timer, update 10 Hz
  if((tick - last_tick) >= 100)
  {
    last_tick = tick;
    count1++;
    if(count1 >= delay_1)
    {
      digitalWrite(OD_1,LOW);
      level_1 = 0;
    }
    count2++;
    if(count2 >= delay_2)
    {
      digitalWrite(OD_2,LOW);
      level_2 = 0;
    }
  }
  //Millis timer, update 1kHz
  if((tick - last_ms_tick) >= 1)
  {
    last_ms_tick = tick;
    countrf++;
    if(countrf >= rfswdelay)
    {
      digitalWrite(RFSW_1,LOW);
      digitalWrite(RFSW_2,LOW);
    }
  }

  if(rfsw_pos != rfsw_last_pos)
  {
    if(rfsw_pos == 1)
    {
      digitalWrite(RFSW_1,HIGH);
      countrf = 0;
      rfsw_last_pos = 1;
    }
    else if(rfsw_pos == 2)
    {
      digitalWrite(RFSW_2,HIGH);
      countrf = 0;
      rfsw_last_pos = 2;
    }
  }
}

//1kHz interrupt
void handler_loop(void) {
  tick++;
}

