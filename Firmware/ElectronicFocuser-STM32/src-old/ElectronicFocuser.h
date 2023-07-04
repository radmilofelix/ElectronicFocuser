#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "modbusSlave.h"

#define EN  PB4     
#define M1  PB6 // A4988 MS1; DRV8825 M0; STSPIN220 MODE1; STSPIN820 M1       
#define M2  PB7 // A4988 MS2; DRV8825 M1; STSPIN220 MODE2; STSPIN820 M2 
#define M3  PB8 // A4988 MS3; DRV8825 M2; STSPIN220 (1); STSPIN820 M3 
#define STDBY  PB5
#define STEP  PB1
#define DIR  PB3
#define RESET PB5


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


// Modbus
//#define REGROOFPOSITION				0
//#define REGCOMMANDFROMPI			1
//#define REGRESPONSETOPI				2
//#define REGSTATUS                   3 // Motor Enabled/Disabled



//  Use I2C-1
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//  Use I2C-2
TwoWire Wire1(PB11, PB10);// Use STM32 I2C2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);
HardwareSerial Serial2(USART2);   // PA3  (RX)  PA2  (TX)

void setup();
void loop();
void displayMenu();
int pulseStep (unsigned long numberOfSteps, int motionDirection, int duration);
int toneStep (unsigned long numberOfSteps, int motionDirection, unsigned long frequency);
unsigned long readNumber();
void ValidateMicrostepMode();
void displayValues();
void operationSelect();
void selectMicrostepMode(int microstep);

//*
void displaySize1Text();
void displaySize2Text();
void displaySize1Char(char* txt, int x, int y, bool eraseDisplay);
void displaySize2Char(char* txt, int x, int y, bool eraseDisplay);
//*/

ModbusSlave modbus_f(1,Serial1,PA7);
