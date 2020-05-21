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

#define HW_1P0
//#define BREADBOARD

#ifdef BREADBOARD
  #define RFSW_1 PB9
  #define RFSW_2 PB8
  #define OD_1 PB7
  #define OD_2 PB6
  #define RELAY PB5
#endif
#ifdef HW_1P0
  #define RFSW_1 PB13
  #define RFSW_2 PB15
  #define RFSW_3 PA9
  #define RFSW_4 PA15
  #define RFSW_5 PB4
  #define RFSW_6 PB6
  #define RFSW_7 PB7
  #define OD_1 PB12
  #define OD_2 PB14
  #define OD_3 PA8
  #define OD_4 PA10
  #define OD_5 PB3
  #define OD_6 PB5
  #define RELAY PB11 //USB2.0, 1=on, 0=off
  #define RELAY2 PB10 //USB3.0, 1=on, 0=off
  #define USB2ISNS PB1
  #define USB2VSNS PB0
  #define USB3ISNS PA7
  #define USB3VSNS PA6
  #define AIREF PA5
  #define V12V PA4
  #define TMP1 PA3
  #define TMP2 PA2
  #define AUXADC1 PA1
  #define AUXADC2 PA2
  #define AUXIO1 PB8
  #define AUXIO2 PB9
  #define AUXI13 PC13
#endif

#define DEF_LEVEL 0
#define DEF_RFPOS 1
#define DEF_DELAY 10
#define MIN_DELAY 1
#define MAX_DELAY 650

const char vers[] = "0.1-04302020"; 

//Initialize reference voltage with default until read out of EEPROM
#define RFSWDELAYINIT 50
volatile uint16 RFSWDELAY = RFSWDELAYINIT;
volatile uint32 tick = 0;
uint32 last_tick = 0;
uint32 last_ms_tick = 0;

uint8 level_1 = DEF_LEVEL;
uint8 last_level_1 = DEF_LEVEL;
uint16 delay_1 = DEF_DELAY;
uint8 level_2 = DEF_LEVEL;
uint8 last_level_2 = DEF_LEVEL;
uint16 delay_2 = DEF_DELAY;
#ifdef HW_1P0
  uint8 level_3 = DEF_LEVEL;
  uint8 last_level_3 = DEF_LEVEL;
  uint16 delay_3 = DEF_DELAY;
  uint8 level_4 = DEF_LEVEL;
  uint8 last_level_4 = DEF_LEVEL;
  uint16 delay_4 = DEF_DELAY;
  uint8 level_5 = DEF_LEVEL;
  uint8 last_level_5 = DEF_LEVEL;
  uint16 delay_5 = DEF_DELAY;
  uint8 level_6 = DEF_LEVEL;
  uint8 last_level_6 = DEF_LEVEL;
  uint16 delay_6 = DEF_DELAY;
#endif
uint8 rfsw_1 = DEF_LEVEL;
uint8 rfsw_2 = DEF_LEVEL;
#ifdef HW_1P0
  uint8 rfsw_3 = DEF_LEVEL;
  uint8 rfsw_4 = DEF_LEVEL;
  uint8 rfsw_5 = DEF_LEVEL;
  uint8 rfsw_6 = DEF_LEVEL;
  uint8 rfsw_7 = DEF_LEVEL;
#endif
uint8 rfsw_1_pos = 1;
uint8 rfsw_1_lpos = 1;
uint8 rfsw_2_pos = 1;
uint8 rfsw_2_lpos = 1;
uint8 rfsw_3_pos = 1;
uint8 rfsw_3_lpos = 1;
uint8 rfsw_4_pos = 1;
uint8 rfsw_4_lpos = 1;
uint8 rfsw_5_pos = 1;
uint8 rfsw_5_lpos = 1;
uint8 rfsw_6_pos = 1;
uint8 rfsw_6_lpos = 1;
uint8 rfsw_7_pos = 1;
uint8 rfsw_7_lpos = 1;
uint8 relay_pos = 0;
uint8 relay2_pos = 0;
uint16 count1 = 0;
uint16 count2 = 0;
uint16 count3 = 0;
uint16 count4 = 0;
uint16 count5 = 0;
uint16 count6 = 0;
uint16 countrf = 0;
uint16 rfswdelay = RFSWDELAY;

//Initialize reference voltage with default until read out of EEPROM
#define REFAVALINIT 3096824.2f
volatile float REFAVAL = REFAVALINIT;
//EEPROM address for reference value: 1
#define REFAVALADDR 1
//Original ADC2I = 620.606
//Original ADC2V = 846.281
//Voltage reported / Voltage real * ADC2VxINIT orig = ADC2VxINIT new
//Voltage reads high => increase ADC2V value
//Current reads high => increase ADC2I value
//EEPROM address for USB2.0 voltage value: 1
#define ADC2V1ADDR 1
//EEPROM address for USB3.0 voltage value: 2
#define ADC2V2ADDR 2
//10k/18.1k -> 0-5.973V -> ADC/4096*5973 -> 
#define ADC2V1INIT 685.752f //5.008V = 7.235V
#define ADC2V2INIT 685.752f
//todo: reset adc2v2init during "l" calibration for storing correction factors
//EEPROM address for slot 1 current value: 3
#define ADC2I1ADDR 3
#define ADC2I1INIT 1241.212f //50m shunt
#define ADC2I2ADDR 4
#define ADC2I2INIT 1241.212f
//EEPROM address for buffer current value: 5
#define V12VADDR 5
#define V12VINIT 3.46436f //x/4096*3.3*1000*4.3
volatile float V122V = V12VINIT;
volatile float ADC2I1 = ADC2I1INIT;
volatile float ADC2V1 = ADC2V1INIT;
volatile float ADC2I2 = ADC2I2INIT;
volatile float ADC2V2 = ADC2V2INIT;
volatile float corr_factor = 1.0f;
#define TOFFS1 0.0f
#define TOFFS2 0.0f

uint32 adciref = 0;
float vusb2 = 0;
float iusb2 = 0;
float vusb3 = 0;
float iusb3 = 0;
uint16 v12v_i = 0;

//32 chars with /r/n + 8 corrections (e.g. 4 backspace + 4 chars)
#define MAXCHARS 40
char receivedChars[MAXCHARS];
boolean newData = false;

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarkers[] = {'t', '?', 'v', 's', 'o', 'r', 'z', 'l', 'm'};
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
             || rc == startMarkers[4] || rc == startMarkers[5] || rc == startMarkers[6] || rc == startMarkers[7] || rc == startMarkers[8]) {
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
      #ifdef BREADBOARD
      Serial.print("\r\n");
      Serial.println("> Select Mode:");
      Serial.println(">  RF Switch");
      Serial.println(">   r[1-2]");
      Serial.println(">  Relay (1=off, 2=on)");
      Serial.println(">   l[1-2]");
      Serial.println(">  Open Drain Output");
      Serial.println(">   o[1-2] l[level: 0 = open, 1 = low] d[delay, x0.1s]");
      Serial.println(">            def.0,                      def.10 (1s)");
      Serial.println(">  Cal R/W Mode");
      Serial.println(">   t[r/w] a[address: 0-999] d[data, unsigned int (0-65535)]");
      Serial.println(">  Version");
      Serial.println(">   v");
      Serial.println(">  Status");
      Serial.println(">   s");
      Serial.println(">  Soft Reset");
      Serial.println(">   z");
      Serial.println(">  Help (Prints this menu)");
      Serial.println(">   ?");
      Serial.print("\r\n");
      Serial.print("> ");
      break;
      #endif
      #ifdef HW_1P0
      Serial.print("\r\n");
      Serial.println("> Select Mode:");
      Serial.println(">  RF Switch");
      Serial.println(">   r[1-7]");
      Serial.println(">  USB 2.0 Power (1=off, 2=on)");
      Serial.println(">   l[1-2]");
      Serial.println(">  USB 3.0 Power (1=off, 2=on)");
      Serial.println(">   m[1-2]");
      Serial.println(">  Open Drain Output");
      Serial.println(">   o[1-6] l[level: 0 = open, 1 = low] d[delay, x0.1s]");
      Serial.println(">            def.0,                      def.10 (1s)");
      Serial.println(">  Cal R/W Mode");
      Serial.println(">   t[r/w] a[address: 0-999] d[data, unsigned int (0-65535)]");
      Serial.println(">  Version");
      Serial.println(">   v");
      Serial.println(">  Status");
      Serial.println(">   s");
      Serial.println(">  Soft Reset");
      Serial.println(">   z");
      Serial.println(">  Help (Prints this menu)");
      Serial.println(">   ?");
      Serial.print("\r\n");
      Serial.print("> ");
      break;
      #endif
  }
}

void parseOD(uint8 nArgs, char* args[], uint8* level_n, uint16* delay_n)
{
  uint8 i;
  int16 strVal = 0;
  
  //Set default charge current/voltage/cutoff
  *level_n = DEF_LEVEL;
  *delay_n = DEF_DELAY;

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
          *level_n = strVal;
          Serial.println(*level_n);
          break;
        case 'd':
          Serial.print("> Using delay: ");
          strVal = fast_atoi_leading_pos(args[i]);
          if(strVal<MIN_DELAY)
            strVal = MIN_DELAY;
          else if(strVal>MAX_DELAY)
            strVal = MAX_DELAY;
          *delay_n = strVal;
          Serial.print(*delay_n*100);
          Serial.println("ms");
          break;
        default:
          break;
      }
    }
  }
  
  if(*level_n == DEF_LEVEL)
  {
    Serial.print("> Using def level: ");
    Serial.println(*level_n);
  }
  if(*delay_n == DEF_DELAY)
  {
    Serial.print("> Using def delay: ");
    Serial.print(*delay_n*100);
    Serial.println("ms");
  }
}

void parseRF(uint8 nArgs, char* args[], uint8* pos_n)
{
  uint8 i;
  int16 strVal = 0;
  
  //Set default charge current/voltage/cutoff
  *pos_n = DEF_RFPOS;

  Serial.print("\r\n");
  if(nArgs>1)
  {
    for(i=1; i<nArgs; i++)
    {
      switch (args[i][0])
      {
        case 'l':
          Serial.print("> Using position: ");
          strVal = fast_atoi_leading_pos(args[i]);
          if(strVal<1)
            strVal = 1;
          if(strVal>2)
            strVal = 2;
          *pos_n = strVal;
          Serial.println(*pos_n);
          break;
        default:
          break;
      }
    }
  }
  
  if(*pos_n == DEF_RFPOS)
  {
    Serial.print("> Using def position: ");
    Serial.println(*pos_n);
  }
}

void setup() {
  int i = 0;
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
  pinMode(RELAY,OUTPUT);
  digitalWrite(RELAY,LOW);
  /*#define RFSW_1 PB13
    #define RFSW_2 PB15
    #define RFSW_3 PA9
    #define RFSW_4 PA15
    #define RFSW_5 PB4
    #define RFSW_6 PB6
    #define RFSW_7 PB7
    #define OD_1 PB12
    #define OD_2 PB14
    #define OD_3 PA8
    #define OD_4 PA10
    #define OD_5 PB3
    #define OD_6 PB5
    #define RELAY PB11 //USB2.0, 1=on, 0=off
    #define RELAY2 PB10 //USB3.0, 1=on, 0=off
    #define USB2ISNS PB1
    #define USB2VSNS PB0
    #define USB3ISNS PA7
    #define USB3VSNS PA6
    #define AIREF PA5
    #define V12V PA4
    #define TMP1 PA3
    #define TMP2 PA2
    #define AUXADC1 PA1
    #define AUXADC2 PA2
    #define AUXIO1 PB8
    #define AUXIO2 PB9
    #define AUXI13 PC13*/
  #ifdef HW_1P0
    pinMode(RFSW_3,OUTPUT);
    digitalWrite(RFSW_3,LOW);
    pinMode(RFSW_4,OUTPUT);
    digitalWrite(RFSW_4,LOW);
    pinMode(RFSW_5,OUTPUT);
    digitalWrite(RFSW_5,LOW);
    pinMode(RFSW_6,OUTPUT);
    digitalWrite(RFSW_6,LOW);
    pinMode(RFSW_7,OUTPUT);
    digitalWrite(RFSW_7,LOW);
    pinMode(OD_3,OUTPUT);
    digitalWrite(OD_3,LOW);
    pinMode(OD_4,OUTPUT);
    digitalWrite(OD_4,LOW);
    pinMode(OD_5,OUTPUT);
    digitalWrite(OD_5,LOW);
    pinMode(OD_6,OUTPUT);
    digitalWrite(OD_6,LOW);
    pinMode(RELAY2,OUTPUT);
    digitalWrite(RELAY2,LOW);
    pinMode(USB2ISNS, INPUT_ANALOG);
    pinMode(USB2VSNS, INPUT_ANALOG);
    pinMode(USB3ISNS, INPUT_ANALOG);
    pinMode(USB3VSNS, INPUT_ANALOG);
    pinMode(TMP1, INPUT_ANALOG);
    pinMode(TMP2, INPUT_ANALOG);
    pinMode(AUXADC1, INPUT_ANALOG);
    pinMode(AUXADC2, INPUT_ANALOG);
    pinMode(V12V, INPUT_ANALOG);
    pinMode(AIREF, INPUT_ANALOG);
    adciref = analogRead(AIREF); //Iref voltage
  #endif

  delay(5000);

  Timer2.pause(); // Pause the timer while we're configuring it
  Timer2.setPrescaleFactor(2);
  Timer2.setOverflow(36000); //36 MHz/36000 = 1000 Hz
  //Set up an interrupt on channel 1
  Timer2.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  Timer2.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
  Timer2.attachCompare1Interrupt(handler_loop);
  Timer2.refresh(); //Refresh the timer's count, prescale, and overflow

  #ifdef BREADBOARD
  Serial.print("> HW Cfg: Breadboard");
  #endif
  #ifdef HW_1P0
  Serial.print("> HW Cfg: 1.0");
  #endif

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
      Serial.print("> RF switch delay value: ");
      Serial.print(wData);
      Serial.println("ms");
      RFSWDELAY = wData;
    }
  }
  else
  {
    Serial.println("> RF switch delay not found, using default.");
  }
  estatus = EEPROM.read(REFAVALADDR, &wData);
  if(estatus == 0)
  {
    if(wData > 2600 || wData < 2400)
    {
      Serial.println("> Ref cal out of range, using default.");
    }
    else
    {
      Serial.print("> Ref cal value: ");
      Serial.print(wData);
      Serial.println("mV");
      REFAVAL = ((float)wData)*1241.212;
    }
  }
  else
  {
    Serial.println("> Ref cal not found, using default.");
  }
  
  Serial.print("> SW ver: ");
  Serial.println(vers);

  Serial.println("> Init ref...");
  adciref = 0;
  for(i=0;i<1000;i++)
  {
    adciref += analogRead(AIREF); //Iref voltage
    //Serial.println(adciref);
  }
  Serial.print("> ADC corr: ");
  corr_factor = (REFAVAL/((float)adciref));
  Serial.print(corr_factor*100);
  Serial.print(" ");
  ADC2I1 = ADC2I1/corr_factor;
  Serial.print(ADC2I1);
  Serial.print(" ");
  ADC2I2 = ADC2I2/corr_factor;
  ADC2V1 = ADC2V1/corr_factor;
  Serial.println(ADC2V1);
  ADC2V2 = ADC2V2/corr_factor;
  adciref = (uint32)((float)adciref/1000.0); //Iref voltage
  
  Timer2.resume(); //Start the timer counting
}

void updateADCRef(){
  int i = 0;
  adciref = 0;
  for(i=0;i<1000;i++)
  {
    adciref += analogRead(AIREF); //Iref voltage
  }
  corr_factor = (REFAVAL/((float)adciref));
  ADC2I1 = ADC2I1/corr_factor;
  ADC2I2 = ADC2I2/corr_factor;
  ADC2V1 = ADC2V1/corr_factor;
  ADC2V2 = ADC2V2/corr_factor;
  adciref = (uint32)((float)adciref/1000.0); //Iref voltage
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
          parseOD(i, args, &level_1, &delay_1);
          if(level_1 == 1)
          {
            digitalWrite(OD_1,HIGH);
            count1 = 0;
            last_level_1 = 1;
            Serial.println("\r\n");
            Serial.print("OD1: ");
            Serial.print(level_1);
            Serial.print(", ");
            Serial.print(delay_1*100);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        else if(args[0][1] == '2')
        {
          parseOD(i, args, &level_2, &delay_2);
          if(level_2 == 1)
          {
            digitalWrite(OD_2,HIGH);
            count2 = 0;
            last_level_2 = 1;
            Serial.println("\r\n");
            Serial.print("OD2: ");
            Serial.print(level_2);
            Serial.print(", ");
            Serial.print(delay_2*100);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        #ifdef HW_1P0
        else if(args[0][1] == '3')
        {
          parseOD(i, args, &level_3, &delay_3);
          if(level_3 == 1)
          {
            digitalWrite(OD_3,HIGH);
            count3 = 0;
            last_level_3 = 1;
            Serial.println("\r\n");
            Serial.print("OD3: ");
            Serial.print(level_3);
            Serial.print(", ");
            Serial.print(delay_3*100);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        else if(args[0][1] == '4')
        {
          parseOD(i, args, &level_4, &delay_4);
          if(level_4 == 1)
          {
            digitalWrite(OD_4,HIGH);
            count4 = 0;
            last_level_4 = 1;
            Serial.println("\r\n");
            Serial.print("OD4: ");
            Serial.print(level_4);
            Serial.print(", ");
            Serial.print(delay_4*100);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        else if(args[0][1] == '5')
        {
          parseOD(i, args, &level_5, &delay_5);
          if(level_5 == 1)
          {
            digitalWrite(OD_5,HIGH);
            count5 = 0;
            last_level_5 = 1;
            Serial.println("\r\n");
            Serial.print("OD5: ");
            Serial.print(level_5);
            Serial.print(", ");
            Serial.print(delay_5*100);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        else if(args[0][1] == '6')
        {
          parseOD(i, args, &level_6, &delay_6);
          if(level_6 == 1)
          {
            digitalWrite(OD_6,HIGH);
            count6 = 0;
            last_level_6 = 1;
            Serial.println("\r\n");
            Serial.print("OD6: ");
            Serial.print(level_6);
            Serial.print(", ");
            Serial.print(delay_6*100);
            Serial.print("ms");
            Serial.print("\r\n> ");
          }
        }
        #endif
        break;
      case 'r':
        if(args[0][1] == '1')
        {
          //parseRF(i, args, &rfsw_1_pos);
          rfsw_1_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        else if(args[0][1] == '2')
        {
          //parseRF(i, args, &rfsw_2_pos);
          rfsw_2_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        #ifdef HW_1P0
        else if(args[0][1] == '3')
        {
          //parseRF(i, args, &rfsw_3_pos);
          rfsw_3_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        else if(args[0][1] == '4')
        {
          //parseRF(i, args, &rfsw_4_pos);
          rfsw_4_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        else if(args[0][1] == '5')
        {
          //parseRF(i, args, &rfsw_5_pos);
          rfsw_5_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        else if(args[0][1] == '6')
        {
          //parseRF(i, args, &rfsw_6_pos);
          rfsw_6_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        else if(args[0][1] == '7')
        {
          //parseRF(i, args, &rfsw_7_pos);
          rfsw_7_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        else if(args[0][1] == '8')
        {
          //parseRF(i, args, &rfsw_7_pos);
          rfsw_1_pos = 2;
          rfsw_2_pos = 2;
          rfsw_3_pos = 2;
          Serial.print("\r\n");
          Serial.print("> ");
        }
        #endif
        break;
      case 'l':
        if(args[0][1] == '1')
        {
          relay_pos = 0;
          digitalWrite(RELAY,LOW);
          Serial.println("\r\n");
          Serial.print("Relay: ");
          Serial.print(relay_pos);
          Serial.print("\r\n> ");
        }
        else if(args[0][1] == '2')
        {
          relay_pos = 1;
          digitalWrite(RELAY,HIGH);
          Serial.println("\r\n");
          Serial.print("Relay: ");
          Serial.print(relay_pos);
          Serial.print("\r\n> ");
        }
        break;
      case 'm':
        if(args[0][1] == '1')
        {
          relay2_pos = 0;
          digitalWrite(RELAY2,LOW);
          Serial.println("\r\n");
          Serial.print("Relay 2: ");
          Serial.print(relay2_pos);
          Serial.print("\r\n> ");
        }
        else if(args[0][1] == '2')
        {
          relay2_pos = 1;
          digitalWrite(RELAY2,HIGH);
          Serial.println("\r\n");
          Serial.print("Relay 2: ");
          Serial.print(relay2_pos);
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
        v12v_i = (int)(((float)analogRead(V12V)) * V122V);
        Serial.println("\r\n");
        Serial.print("12V: ");
        Serial.print(v12v_i);
        Serial.print(", USB2.0 V: ");
        Serial.print((int)((((float)analogRead(USB2VSNS)) / ADC2V1)*1000 + 0.5));
        Serial.print(", A: ");
        Serial.print((int)((((float)analogRead(USB2ISNS)) / ADC2I1)*1000 + 0.5));
        Serial.print(", En: ");
        Serial.println(relay_pos);
        Serial.print("USB3.0 V: ");
        Serial.print((int)((((float)analogRead(USB3VSNS)) / ADC2V2)*1000 + 0.5));
        Serial.print(", A: ");
        Serial.print((int)((((float)analogRead(USB3ISNS)) / ADC2I2)*1000 + 0.5));
        Serial.print(", En: ");
        Serial.print(relay2_pos);
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
      if(last_level_1 != level_1)
      {
        Serial.print("\r\nOD1: ");
        Serial.println(level_1);
        Serial.print("\r\n> ");
        last_level_1 = level_1;
      }
    }
    count2++;
    if(count2 >= delay_2)
    {
      digitalWrite(OD_2,LOW);
      level_2 = 0;
      if(last_level_2 != level_2)
      {
        Serial.print("\r\nOD2: ");
        Serial.println(level_2);
        Serial.print("\r\n> ");
        last_level_2 = level_2;
      }
    }
    #ifdef HW_1P0
    count3++;
    if(count3 >= delay_3)
    {
      digitalWrite(OD_3,LOW);
      level_3 = 0;
      if(last_level_3 != level_3)
      {
        Serial.print("\r\nOD3: ");
        Serial.println(level_3);
        Serial.print("\r\n> ");
        last_level_3 = level_3;
      }
    }
    count4++;
    if(count4 >= delay_4)
    {
      digitalWrite(OD_4,LOW);
      level_4 = 0;
      if(last_level_4 != level_4)
      {
        Serial.print("\r\nOD4: ");
        Serial.println(level_4);
        Serial.print("\r\n> ");
        last_level_4 = level_4;
      }
    }
    count5++;
    if(count5 >= delay_5)
    {
      digitalWrite(OD_5,LOW);
      level_5 = 0;
      if(last_level_5 != level_5)
      {
        Serial.print("\r\nOD5: ");
        Serial.println(level_5);
        Serial.print("\r\n> ");
        last_level_5 = level_5;
      }
    }
    count6++;
    if(count6 >= delay_6)
    {
      digitalWrite(OD_6,LOW);
      level_6 = 0;
      if(last_level_6 != level_6)
      {
        Serial.print("\r\nOD6: ");
        Serial.println(level_6);
        Serial.print("\r\n> ");
        last_level_6 = level_6;
      }
    }
    #endif
  }
  //Millis timer, update 1kHz
  if((tick - last_ms_tick) >= 1)
  {
    last_ms_tick = tick;
    countrf++;
    if(countrf >= rfswdelay)
    {
      digitalWrite(RFSW_1,LOW);
      rfsw_1_pos = 1;
      rfsw_1_lpos = 1;
      digitalWrite(RFSW_2,LOW);
      rfsw_2_pos = 1;
      rfsw_2_lpos = 1;
      #ifdef HW_1P0
      digitalWrite(RFSW_3,LOW);
      rfsw_3_pos = 1;
      rfsw_3_lpos = 1;
      digitalWrite(RFSW_4,LOW);
      rfsw_4_pos = 1;
      rfsw_4_lpos = 1;
      digitalWrite(RFSW_5,LOW);
      rfsw_5_pos = 1;
      rfsw_5_lpos = 1;
      digitalWrite(RFSW_6,LOW);
      rfsw_6_pos = 1;
      rfsw_6_lpos = 1;
      digitalWrite(RFSW_7,LOW);
      rfsw_7_pos = 1;
      rfsw_7_lpos = 1;
      #endif
    }
  }

  if(rfsw_1_pos == 2)
  {
    digitalWrite(RFSW_1,HIGH);
    if(rfsw_1_lpos == 1)
    {
      countrf = 0;
      rfsw_1_lpos = 2;
    }
  }
  if(rfsw_2_pos == 2)
  {
    digitalWrite(RFSW_2,HIGH);
    if(rfsw_2_lpos == 1)
    {
      countrf = 0;
      rfsw_2_lpos = 2;
    }
  }
  #ifdef HW_1P0
  if(rfsw_3_pos == 2)
  {
    digitalWrite(RFSW_3,HIGH);
    if(rfsw_3_lpos == 1)
    {
      countrf = 0;
      rfsw_3_lpos = 2;
    }
  }
  else if(rfsw_4_pos == 2)
  {
    digitalWrite(RFSW_4,HIGH);
    if(rfsw_4_lpos == 1)
    {
      countrf = 0;
      rfsw_4_lpos = 2;
    }
  }
  else if(rfsw_5_pos == 2)
  {
    digitalWrite(RFSW_5,HIGH);
    if(rfsw_5_lpos == 1)
    {
      countrf = 0;
      rfsw_5_lpos = 2;
    }
  }
  else if(rfsw_6_pos == 2)
  {
    digitalWrite(RFSW_6,HIGH);
    if(rfsw_6_lpos == 1)
    {
      countrf = 0;
      rfsw_6_lpos = 2;
    }
  }
  else if(rfsw_7_pos == 2)
  {
    digitalWrite(RFSW_7,HIGH);
    if(rfsw_7_lpos == 1)
    {
      countrf = 0;
      rfsw_7_lpos = 2;
    }
  }
  #endif
}

//1kHz interrupt
void handler_loop(void) {
  tick++;
}

