// Travel distance: 17.5mm
// Travel distance per big wheel revolution: 13mm
// Steps per revolution (1/256 microstepping): 102400
// Total steps (1/256 microstepping): 138240
// Travel per step (1/256 microstepping): 126.953125nm


#include <Arduino.h>
#include "ElectronicFocuser.h"
#include <RotaryEncoder.h>
#include "OneButton.h"

#define ENCODER_CLK PA6
#define ENCODER_DT PA5
#define ENCODER_SW  PA4
#define BUZZERPIN PA0
#define LIMITSWITCHPIN PB12

RotaryEncoder *encoder = nullptr;
OneButton button(ENCODER_SW, true);

unsigned long pressStartTime, cycleTime;
bool longPress=false;
int longClickCycles=0;
int longClickCycleTime=1000; // ms

const int DRIVER_A4988 = 1;
const int DRIVER_DRV8825 = 2;
const int DRIVER_STSPIN220 = 3;
const int DRIVER_STSPIN820 = 4;


int working=0;


int motorDirection= 0;
int motorEnable=0;
unsigned long steps=0;
unsigned long freq=0;
int duration=0;
int microstepping=0;
int ms1 = 0;
int ms2 = 0;
int ms3 = 0;
int tempdir,tempstep;


unsigned int previousPress = 0;
int contactDebounce = 20;
bool enableMotion=true;


//int motorDriver = DRIVER_A4988;
//int motorDriver = DRIVER_DRV8825;
int motorDriver = DRIVER_STSPIN820;
//int motorDriver = DRIVER_STSPIN220;


void checkTicks()
{
  // include all buttons here to be checked
  button.tick(); // just call tick() to check the state.
}

// this function will be called when the button was pressed 1 time only.
void singleClick()
{
  Serial2.println("singleClick() detected.");
  tone(BUZZERPIN,2300,150);
} // singleClick

// this function will be called when the button was pressed 2 times in a short timeframe.
void doubleClick()
{
  Serial2.println("doubleClick() detected.");
  tone(BUZZERPIN,1000,600);
  //ledState = !ledState; // reverse the LED
  //digitalWrite(PIN_LED, ledState);
} // doubleClick

// this function will be called when the button was pressed multiple times in a short timeframe.
void multiClick()
{
  Serial2.print("multiClick(");
  Serial2.print(button.getNumberClicks());
  Serial2.println(") detected.");

  //ledState = !ledState; // reverse the LED
  //digitalWrite(PIN_LED, ledState);
} // multiClick

// this function will be called when the button was held down for 1 second or more.
void pressStart()
{
  longPress=true;
  longClickCycles=0;
  Serial2.println("pressStart()");
  pressStartTime = millis() - 1000; // as set in setPressTicks()
  cycleTime=pressStartTime;
} // pressStart()

// this function will be called when the button was released after a long hold.
void pressStop()
{
  longPress=false;
  Serial2.print("pressStop(");
  Serial2.print(millis() - pressStartTime);
  Serial2.println(") detected.");
  delay(500);
  tone(BUZZERPIN,1200,200);
  delay(150);
  tone(BUZZERPIN,1000,300);
} // pressStop()


void LimitSwitchInterrupt()
{
  if(!digitalRead(LIMITSWITCHPIN))
  {
    if( (millis() - previousPress) > contactDebounce )
    {
     enableMotion=false;
      previousPress = millis();
      Serial2.println("Limit Switch Debounced!!!");
    }
  }
  Serial2.println("Limit Switch!!!");
}


void setup()
{
  // pin definitions
  pinMode(STDBY, OUTPUT);
  digitalWrite(STDBY, LOW); //device in stand-by
  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW); // output stage disabled
  pinMode(M1, OUTPUT );
  pinMode(M2, OUTPUT );
  pinMode(M3, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(BUZZERPIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(LIMITSWITCHPIN), LimitSwitchInterrupt, FALLING);
  digitalWrite(BUZZERPIN, 0);
  if(motorDriver == DRIVER_A4988 || motorDriver == DRIVER_DRV8825)
  {
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, HIGH);
  }
  delay (100);
  modbus_f.begin(19200);
  Serial2.begin(19200);
//  Serial2.begin(9600);
  Serial2.println("Serial2 started.");

  displayMenu();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
{ // Address 0x3D for 128x64
    Serial2.println(F("SSD1306 allocation failed"));
//    for(;;);
  }
//  delay(2000);
//  displaySize1Text();
//  displaySize2Text();

  displaySize2Char("H", 0, 0, true);
  displaySize2Char("N", 12, 16, false);
  displaySize2Char("MDM", 24, 0, false);

//  displaySize1Char("H", 0, 0, true);
//  displaySize1Char("N", 6, 8, false);
//  displaySize1Char("MDM", 12, 0, false);
//  displaySize1Char("W", 6, 16, false);


Serial2.println("InterruptRotator example for the RotaryEncoder library.");
//  encoder = new RotaryEncoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::TWO03);
//  encoder = new RotaryEncoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::FOUR0);
  encoder = new RotaryEncoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::FOUR3);

 
  
  
  Serial2.println("One Button Example with interrupts.");
  // setup interrupt routine
  // when not registering to the interrupt the sketch also works when the tick is called frequently.
  attachInterrupt(digitalPinToInterrupt(ENCODER_SW), checkTicks, CHANGE);

  // link the xxxclick functions to be called on xxxclick event.
  button.attachClick(singleClick);
  button.attachDoubleClick(doubleClick);
  button.attachMultiClick(multiClick);

  button.setPressTicks(1000); // that is the time when LongPressStart is called
  button.attachLongPressStart(pressStart);
  button.attachLongPressStop(pressStop);

}

void displayMenu()
{
 // Serial monitor menu
  Serial2.println("");
  Serial2.println(F(" m = mode")); 
  Serial2.println(F(" r = dir anti clockwise "));
  Serial2.println(F(" t = dir clockwise"));
  Serial2.println(F(" e = driver on "));
  Serial2.println(F(" o = driver in stand by"));
  Serial2.println(F(" s = step"));
  Serial2.println(F(" f = frequency for tone drive"));
  Serial2.println(F(" d = delay (us) for pulse drive"));
  Serial2.println(F(" p = pulse drive"));
  Serial2.println(F(" l = tone drive"));
  Serial2.println(F(" c = enableMotion"));
  Serial2.println(F(" z = display this menu"));
  Serial2.println(F(" v = display motion values"));
  Serial2.println("");
  //
}


int pulseStep (unsigned long numberOfSteps, int motionDirection, int duration)
{
  if(!enableMotion)
  {
    Serial2.println("Motion disabled!");
    return 0;
  }
  digitalWrite(DIR, motionDirection);
  delay(10);
  for ( int i = 0; i < numberOfSteps; i++)
  {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(duration);
//    delay(duration);   
    digitalWrite(STEP, LOW); 
//    delay(duration);   
    delayMicroseconds(duration);
    if(!enableMotion) // motion can be disabled by LimitSwitch interrupt
      i=numberOfSteps;
  }
  return 1;
}


int toneStep (unsigned long numberOfSteps, int motionDirection, unsigned long frequency)
{
  if(!enableMotion)
  {
    Serial2.println("Motion disabled!");
    return 0;
  }
  digitalWrite(DIR, motionDirection);
  delay(10);
  unsigned long duration=1000*(unsigned long)numberOfSteps/(unsigned long)frequency;
  if (numberOfSteps != 0)
    tone(STEP,(unsigned int)frequency,duration);
  else
    tone(STEP, (unsigned int) frequency ); // turn on, no time limit
//  noTone(STEP,true);
  return 1;
}



  unsigned long readNumber()
  {
    unsigned long charsread = 0;
    unsigned long tempMillis = millis();
    byte chars = 0;
    do
    {
      if (Serial2.available())
      {
        byte tempChar = Serial2.read();
        chars++;
        if ((tempChar >= 48) && (tempChar <= 57))//is it a number?
        {
          charsread = (charsread * 10) + (tempChar - 48);
        }
        else if ((tempChar == 10) || (tempChar == 13))
        {
          //exit at CR/LF
          break;
        }
       }
    }
  while ((millis() - tempMillis < 2000) && (charsread <= 100000) && (chars < 10));
  return (charsread);
}

void ValidateMicrostepMode()
{
  if(microstepping<1)
  {
    Serial2.println(F("Microstep value can not be negative! Microstep mode will be set to full step."));
    microstepping=1;
  }
  switch(motorDriver)
  {
    case DRIVER_STSPIN820:
    if(microstepping==64)
    {
      Serial2.println(F("Wrong microstepping option for STSPIN820 driver! Microstep mode will be set to 1/32 step."));
      microstepping=32;
    }
    break;
    case DRIVER_DRV8825:
    if(microstepping>32)
    {
      Serial2.println(F("Wrong microstepping option for STSPIN820 driver!Microstep mode will be set to 1/32 step."));
      microstepping=32;
    }
    break;
    case DRIVER_A4988:
    if(microstepping>16)
    {
      Serial2.println(F("Wrong microstepping option for STSPIN820 driver!Microstep mode will be set to 1/16 step."));
      microstepping=16;
    }
    break;
  }
  switch(microstepping)
  {
    case 1:
      Serial2.println(F("Microstepping 1/1 step selected"));
    break;
    case 2:
      Serial2.println(F("Microstepping 1/2 step selected"));
    break;
    case 4:
      Serial2.println(F("Microstepping 1/4 step selected"));
    break;
    case 8:
      Serial2.println(F("Microstepping 1/8 step selected"));
    break;
    case 16:
      Serial2.println(F("Microstepping 1/16 step selected"));
    break;
    case 32:
      Serial2.println(F("Microstepping 1/32 step selected"));
    break;
    case 64:
      Serial2.println(F("Microstepping 1/64 step selected"));
    break;
    case 128:
      Serial2.println(F("Microstepping 1/128 step selected"));
    break;
    case 256:
      Serial2.println(F("Microstepping 1/256 step selected"));
    break;
    default:
      Serial2.println(F("Wrong microstepping option! Microstep mode will be set to full step."));
      microstepping=1;
    break;
  }
}



void displayValues()
{
  Serial2.println("");
  if(motorEnable)
    Serial2.println("Motor enabled");
  else
    Serial2.println("Motor disabled");
  if(motorDirection)
    Serial2.println("Motor direction: clockwise");
  else
    Serial2.println("Motor direction: counterclockwise");
  ValidateMicrostepMode();
  Serial2.print(F("Number of steps: "));
  Serial2.println(steps, DEC);
  Serial2.print(F("Frequency-for tone drive (Hz): "));
  Serial2.println(freq, DEC);
  Serial2.print(F("Pulse delay-for pulse drive (us): "));
  Serial2.println(duration, DEC);
  Serial2.println("");
  Serial2.println("Type z - for menu");
  Serial2.println("");
}


void operationSelect()
{
 //check serial
  if (Serial2.available())
  {
    byte command = Serial2.read();
    switch (command)
    {
  // enable the device ( out of stand by )
    case 'e': 
          Serial2.println("Motor enabled");
          motorEnable=1;
          selectMicrostepMode(microstepping);
//          digitalWrite(STDBY, HIGH);
          delay(1);
    break;
  // disable the device - stand by
    case 'o': 
          Serial2.println("Motor disabled");
          motorEnable=0;
          digitalWrite(STDBY, LOW);
    break;
    case 't': 
          Serial2.println("Motor direction: clockwise");
          motorDirection=1;
    break;
    case 'r': 
          Serial2.println("Motor direction: counterclockwise");
          motorDirection=0;
    break;
    case 's': //step number
           steps=readNumber();
           Serial2.print(F("Number of steps: "));
           Serial2.println(steps, DEC);
    break;
    case 'f': // speed number
           freq=readNumber();
           Serial2.print(F("Frequency for tone drive (Hz): "));
           Serial2.println(freq, DEC);
    break;
    case 'd': // pulse delay
           duration=readNumber();
           Serial2.print(F("Pulse delay for pulse drive (us): "));
           Serial2.println(duration, DEC);
    break;
    case 'm': //microstepping selection 
           microstepping=readNumber();
           ValidateMicrostepMode();
           selectMicrostepMode(microstepping);
//           Serial2.print(F("Microstep mode: "));
//           Serial2.println(microstepping, DEC);
    break;
    case 'l': // tone drive
           Serial2.println(F("Driving motor by tone"));
           toneStep(steps,motorDirection,freq);
    break;
    case 'p': // pulse drive
           Serial2.println(F("Driving motor by pulse"));
           pulseStep(steps,motorDirection,duration);
    break;
    case 'c':
           enableMotion=true;
           Serial2.println("Motion Enabled");
    break;
    case 'z':
           displayMenu();
    break;
    case 'v':
           displayValues();
    break;
    }
  }
}

void selectMicrostepMode(int microstep)
{
  switch(motorDriver)
  {
    case DRIVER_A4988:
      switch(microstep)
      {
      case 1:
           ms1=0;
           ms2=0;
           ms3=0;
      break;
      case 2:
           ms1=1;
           ms2=0;
           ms3=0;
      break;
      case 4:
           ms1=1;
           ms2=0;
           ms3=1;
      break;
      case 8:
           ms1=1;
           ms2=1;
           ms3=0;
      break;
      case 16:
           ms1=1;
           ms2=1;
           ms3=1;
      break;
    }
    digitalWrite(M1, ms1);
    digitalWrite(M2, ms2);
    digitalWrite(M3, ms3);
    break;
    case DRIVER_DRV8825:
      switch(microstep)
      {
      case 1:
           ms1=0;
           ms2=0;
           ms3=0;
      break;
      case 2:
           ms1=1;
           ms2=0;
           ms3=0;
      break;
      case 4:
           ms1=0;
           ms2=1;
           ms3=0;
      break;
      case 8:
           ms1=1;
           ms2=1;
           ms3=0;
      break;
      case 16:
           ms1=0;
           ms2=0;
           ms3=1;
      break;
      case 32:
           ms1=1;
           ms2=0;
           ms3=1;
      break;
    }
    digitalWrite(M1, ms1);
    digitalWrite(M2, ms2);
    digitalWrite(M3, ms3);
    break;
    case DRIVER_STSPIN220:
      digitalWrite(STDBY, HIGH);
      switch(microstep)
      {
        case 1:
          ms1=0;
          ms2=0;
          tempdir=0;
          tempstep=0;
        break;
        case 32:
          ms1=0;
          ms2=1;
          tempdir=0;
          tempstep=0;
        break;
        case 128:
           ms1=1;
           ms2=0;
           tempdir=0;
           tempstep=0;
        break;
        case 256:
           ms1=1;
           ms2=1;
           tempdir=0;
           tempstep=0;
        break;
        case 4:
           ms1=0;
           ms2=1;
           tempdir=1;
           tempstep=0;
        break;
        case 64:
           ms1=1;
           ms2=1;
           tempdir=1;
           tempstep=0;
        break;
        case 2:
           ms1=1;
           ms2=0;
           tempdir=0;
           tempstep=1;
        break;
        case 8:
           ms1=1;
           ms2=1;
           tempdir=0;
           tempstep=1;
        break;
        case 16:
           ms1=1;
           ms2=1;
           tempdir=1;
           tempstep=1;
        break;
      }
      digitalWrite(M1, ms1);
      digitalWrite(M2, ms2);
      digitalWrite(DIR, tempdir);
      digitalWrite(STEP, tempstep);  
      digitalWrite(STDBY, LOW);
      delay(100);
      /*if(microstep==16)
      {
        digitalWrite(DIR, HIGH);
        digitalWrite(STEP, HIGH);
      }*/
    break;
    case DRIVER_STSPIN820:
      switch(microstep)
      {
      case 1:
           ms1=0;
           ms2=0;
           ms3=0;
      break;
      case 2:
           ms1=1;
           ms2=0;
           ms3=0;
      break;
      case 4:
           ms1=0;
           ms2=1;
           ms3=0;
      break;
      case 8:
           ms1=1;
           ms2=1;
           ms3=0;
      break;
      case 16:
           ms1=0;
           ms2=0;
           ms3=1;
      break;
      case 32:
           ms1=1;
           ms2=0;
           ms3=1;
      break;
      case 128:
           ms1=0;
           ms2=1;
           ms3=1;
      break;
      case 256:
           ms1=1;
           ms2=1;
           ms3=1;
      break;
    }
    digitalWrite(M1, ms1);
    digitalWrite(M2, ms2);
    digitalWrite(M3, ms3);
    break;
  }
  if(motorEnable)
    digitalWrite(STDBY, HIGH);

}
void loop()
{
  operationSelect();
//  simpleCommand();
  modbus_f.dataBuffer[0]=12;
  modbus_f.dataBuffer[1]=13;
  modbus_f.dataBuffer[2]=14;
  modbus_f.dataBuffer[3]=15;
	modbus_f.poll(4);

  static int pos = 0;
  encoder->tick(); // just call tick() to check the state.
  int newPos = encoder->getPosition();
  if (pos != newPos) {
    Serial2.print("pos:");
    Serial2.print(newPos);
    Serial2.print(" dir:");
    Serial2.println((int)(encoder->getDirection()));
    pos = newPos;
  } // if


  // keep watching the push button, even when no interrupt happens:
 button.tick();
 if( (millis() - cycleTime > longClickCycleTime) && longPress)
 {
    longClickCycles++;
    Serial2.print("LongPressCycles: "); Serial2.println(longClickCycles);
    cycleTime = millis();
//    displaySize2Char("", 24, 0, false);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(24, 32);
    display.println(longClickCycles);
    display.display(); 
    tone(BUZZERPIN,1500,150);
 }

}






//*
void displaySize1Text()
// 21 columns
// 8 rows (2 yellow)
{
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello, world! 1234567123456789012345678901123456789012345678901123456789012345678901123456789012345678901123456789012345678901123456789012345678901123456789012345678901");
  display.display(); 
}

void displaySize2Text()
// 10 columns
// 4 rows (1 yellow)
{
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
//  display.println("Hello,1234world!123412345678901234567890");
  display.println("Hello,");
  display.println("world!");
  display.display(); 
}

void displaySize1Char(char* txt, int x, int y, bool eraseDisplay)
// 21 columns
// 8 rows (2 yellow)
// width: 6 pix.
// heigth: 8 pix.
{
  if(eraseDisplay)
    display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(txt);
  display.display(); 
}

void displaySize2Char(char* txt, int x, int y, bool eraseDisplay)
// 10 columns
// 4 rows (1 yellow)
// width: 12 pix.
// heigth: 16 pix.
{
  if(eraseDisplay)
    display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(txt);
  display.display(); 
}
//*/


