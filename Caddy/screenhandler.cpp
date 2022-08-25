//Make this readable and futureproofing
//Current pin
#define CurrPin A0
//Voltage pins
#define Cell1Pin A1
#define Cell2Pin A2
#define Cell3Pin A3
#define Cell4Pin A12
//Thermistor pins
#define NTC1Pin A8
#define NTC2Pin A9
#define NTC3Pin A10
#define NTC4Pin A11
//Buzzer pin
#define buzzPin 6

//Resistor values
#define Cell1R1 12000
#define Cell1R2 3000
#define Cell2R1 18000
#define Cell2R2 2000
#define Cell3R1 18000
#define Cell3R2 1300
#define Cell4R1 82000
#define Cell4R2 4300

//Max/min acceptable values
#define minV 11.5 //Minimum voltage per battery cell
#define minT -10  //Minimum temperature 
#define maxT 45   //Maximum temperature
#define maxA 20   //Maximum current

//Screen libraries
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

//"Multithreading" for the buzzer
#include "TeensyThreads.h"

//Heaader files
#include "screenhandler.h"

//Timer interrupts
#include <Metro.h>

//Select screen type. The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

//Instantiate an interrupt instance set to 5s to try to reset the buzzer every 5s
Metro notiMetro = Metro(5000);

//If the buzzer can beep. Only here so it won't spam
bool buzz_rdy = true;
//If a notification is avaliable
bool noti_avaliable;

//Voltage warning states
bool V1Warn;
bool V2Warn;
bool V3Warn;
bool V4Warn;
//Current warning state
bool AWarn;
//Temperature warning states
bool T1Warn;
bool T2Warn;
bool T3Warn;
bool T4Warn;

//Initialise the screen
void screenInit()
{  
  u8g2.begin();           // Initialise screen
  u8g2.clearBuffer();     //Clear out the junk
   
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.setFontMode(0);    // enable transparent mode, because speed
}
 
//Rebuild the main screen
void buildVoltageScreen()
{
  //Create positioning struct for easy positioning
  struct posBatVol {
    int x;
    int y;
  };
  posBatVol positVol[3];

  //Adjust text origin points here
  /*
                                    -------------
                                    |   BattX:  |
      adjustable coordinate here -> *           |
                                    |   XX.XX V |
                                    -------------
  */
  //Voltage
  positVol[0].x = 15;
  positVol[0].y = 10;

  positVol[1].x = 83;
  positVol[1].y = 10;

  positVol[2].x = 15;
  positVol[2].y = 32;

  positVol[3].x = 83;
  positVol[3].y = 32;
  
  //Current
  int CurrX = 15;
  int CurrY = 54;

  //Power
  int WattX = 83;
  int WattY = 54;

  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

  //Update warning status
  checkForWarnings();

  //If there was a warning
  if (noti_avaliable)
  {
    //Start the buzzer thread
    threads.addThread(buzzer);
  }
  else
  {
    //Allow buzzer to run
    buzz_rdy = true;
  }
  //Place lines for separation
  drawLines();

  //If the timer elapsed, clear notifications
  if (notiMetro.check() == 1) {
    noti_avaliable = false;
  }

  //Once the screen is ready, draw all the values
  drawVolt("Cell1:", analogRead(Cell1Pin), Cell1R1, Cell1R2, positVol[0].x, positVol[0].y, 1);
  drawVolt("Cell2:", analogRead(Cell2Pin), Cell2R1, Cell2R2, positVol[1].x, positVol[1].y, 2);
  drawVolt("Cell3:", analogRead(Cell3Pin), Cell3R1, Cell3R2, positVol[2].x, positVol[2].y, 3);
  drawVolt("Cell4:", analogRead(Cell4Pin), Cell4R1, Cell4R2, positVol[3].x, positVol[3].y, 4);

  //Calculate current and power draw
  float current = getCurr(analogRead(CurrPin));
  float watt = getCurr(analogRead(CurrPin)) * getVolt(analogRead(Cell4Pin), Cell4R1, Cell4R2, 4);

  //Text buffer for the current and power
  char str[8];
  char str2[8];
  
  //Convert to const char* and show on screen
  u8g2.drawStr(CurrX, CurrY, "Current:");
  dtostrf(current, 3, 2, str);
  u8g2.drawStr(CurrX + 5, CurrY + 10, str);
  u8g2.drawStr(CurrX + 30, CurrY + 10, "A");

  //If the current is high, show an exclamation mark
  if (AWarn) { u8g2.drawStr(CurrX+41, CurrY+10, "!"); }

  //Same here
  u8g2.drawStr(WattX, WattY, "Power:");
  dtostrf(watt, 3, 2, str2);
  u8g2.drawStr(WattX, WattY + 10, str2);
  u8g2.drawStr(WattX + 30, WattY + 10, "W");

  //When the buffer is ready, send to the screen and free up memory
  u8g2.sendBuffer();
}
//Rebuild the temperature screen
void buildTempScreen()
{
  //Create positioning struct for easy positioning
  struct posNTCTemps {
    int x;
    int y;
  };
  posNTCTemps positNTC[3];

  //Adjust text origin points here
  /*
                                    -------------
                                    |   BattX:  |
      adjustable coordinate here -> *           |
                                    |   XX.XX C |
                                    -------------
  */
  //Temperatures
  positNTC[0].x = 15;
  positNTC[0].y = 15;

  positNTC[1].x = 83;
  positNTC[1].y = 15;

  positNTC[2].x = 15;
  positNTC[2].y = 45;

  positNTC[3].x = 83;
  positNTC[3].y = 45;

  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

  //Update warning status
  checkForWarnings();

  //If there was a warning
  if (noti_avaliable)
  {
    //Start the buzzer thread
    threads.addThread(buzzer);
  }
  else
  {
    //Allow buzzer to run
    buzz_rdy = true;
  }
  //Place lines for separation
  drawLinesTemps();

  //If the timer elapsed, clear notifications
  if (notiMetro.check() == 1) {
    noti_avaliable = false;
  }

  //Once the screen is ready, draw all the values
  drawTemps("NTC1:", analogRead(NTC1Pin), positNTC[0].x, positNTC[0].y, 1);
  drawTemps("NTC2:", analogRead(NTC2Pin), positNTC[1].x, positNTC[1].y, 2);
  drawTemps("NTC3:", analogRead(NTC3Pin), positNTC[2].x, positNTC[2].y, 3);
  drawTemps("NTC4:", analogRead(NTC4Pin), positNTC[3].x, positNTC[3].y, 4);

  //Send the prepared buffer to the screen
  u8g2.sendBuffer();
}


//Calculate and draw the voltage values
void drawVolt(char* txt, float analogIn, float R1, float R2, int x, int y, int CellID)
{
  //Text buffer
  char str[8];
  //Convert from 10-bit binary to voltage values and compensate for the voltage divider
  float vin = getVolt(analogIn, R1, R2, CellID);
  
  //Draw relative to adjustable coordinates
  u8g2.drawStr(x, y, txt);
  //If the voltage is below limit, show a warning
  if (vin < 3)
  {
    //Battery is NOT connected
    u8g2.drawStr(x, y + 10, "NC");
  }
  else if (vin < 11)
  {
      //Battery is connected, but low on voltage
      u8g2.drawStr(x, y + 10, "LOW!");
  }
  else
  {
    //convert to const char* and show on screen
    dtostrf(vin, 3, 2, str);
    u8g2.drawStr(x, y + 10, str);
    u8g2.drawStr(x + 28, y + 10, "V");
  }
  //If any of the values are concerning, show an exclamation mark next to them
  switch (CellID)
    {
      case 1:
        if (V1Warn) { u8g2.drawStr(x+40, y+10, "!"); }
        break;
      case 2:
        if (V2Warn) { u8g2.drawStr(x+40, y+10, "!"); }
        break;
      case 3:
        if (V3Warn) { u8g2.drawStr(x+40, y+10, "!"); }
        break;
      case 4:
        if (V4Warn) { u8g2.drawStr(x+40, y+10, "!"); }
        break;
    }
}
//Calculate and draw the temperature values
void drawTemps(char* txt, float analogIn, int x, int y, int CellID)
{
  //Text buffer
  char str[8];
  
  float temp = getTemp(analogIn);
  u8g2.drawStr(x, y, txt);
  dtostrf(temp, 3, 2, str);
  u8g2.drawStr(x, y + 10, str);
  u8g2.drawStr(x + 31, y + 10, "C");
  //If any of the values are concerning, show an exclamation mark next to them
  switch (CellID)
    {
      case 1:
        if (T1Warn) { u8g2.drawStr(x+42, y+10, "!"); }
        break;
      case 2:
        if (T2Warn) { u8g2.drawStr(x+42, y+10, "!"); }
        break;
      case 3:
        if (T3Warn) { u8g2.drawStr(x+42, y+10, "!"); }
        break;
      case 4:
        if (T4Warn) { u8g2.drawStr(x+42, y+10, "!"); }
        break;
    }
}
//Draw the lines seen on the main screen
void drawLines()
{
  u8g2.drawLine(64, 0, 64, 64);
  u8g2.drawLine(0, 22, 128, 22);
  u8g2.drawLine(0, 44, 128, 44);
}

//Draw the lines seen on the temperature screen
void drawLinesTemps()
{
  u8g2.drawLine(64, 0, 64, 64);
  u8g2.drawLine(0, 32, 128, 32);
}


//Calculate battery voltage and return as float
float getVolt(float analogIn, float R1, float R2, int CellID) {
  float val = analogIn; //read analog in
  float vout = (val * 3.3) / 1023.0; //Convert 10-bit to float voltage
  float vin = vout / (R2 / (R1 + R2)); //Ohms law
  
  //Calculate cell voltage by subtracting the other cell volatges. Recognise how much to subtract by identifying the current cell, and thank god for recursion.
  //Probably could do this better, but this board is only made for 4 cells anyway.
  switch(CellID)
  {
    case 4:
      vin = vin - getVolt(analogRead(Cell1Pin), Cell1R1, Cell1R2, 1) - getVolt(analogRead(Cell2Pin), Cell2R1, Cell2R2, 2) - getVolt(analogRead(Cell3Pin), Cell3R1, Cell3R2, 3);
      break;
    case 3:
      vin = vin - getVolt(analogRead(Cell1Pin), Cell1R1, Cell1R2, 1) - getVolt(analogRead(Cell2Pin), Cell2R1, Cell2R2, 2);
      break;
    case 2:
      vin = vin - getVolt(analogRead(Cell1Pin), Cell1R1, Cell1R2, 1);
      break;
      
    //This will cover cell ID 1 as well as 0 in case you don't want any subtraction
    default:
      vin = vin;
      break;     
  }
  
  return vin;
}
//Calculate current based on shunt output
float getCurr(float analogIn) {
  float vout = (analogIn * 3.3) / 1023.0; //Convert 10-bit binary to float voltage
  //Note that these values will have to be recalibrated if either the shunt or R2 on the board will change!
  float current = mapfloat(vout, 0.0, 3.3, 0.0, 22); //Map input voltage to battery current 
  
  return current;
}
//Calculate battery temperatures and return as float
float getTemp(float analogIn) {
  int Vo;
  float R1 = 10000;
  float logR2, R2, T;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
  
  Vo = analogIn;
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  return T;
}

//Update the warning buffer
void checkForWarnings()
{
  clearWarnings();
  //Check for voltage warnings
  if (getVolt(analogRead(Cell1Pin), Cell1R1, Cell1R2, 1) <= minV)
  {
    noti_avaliable = true;
    V1Warn = true;
  }
  if (getVolt(analogRead(Cell2Pin), Cell2R1, Cell2R2, 2) <= minV)
  {
    noti_avaliable = true;
    V2Warn = true;
  }
  if (getVolt(analogRead(Cell3Pin), Cell3R1, Cell3R2, 3) <= minV)
  {
    noti_avaliable = true;
    V3Warn = true;
  }
  if (getVolt(analogRead(Cell4Pin), Cell4R1, Cell4R2, 4) <= minV)
  {
    noti_avaliable = true;
    V4Warn = true;
  }

  if (getTemp(analogRead(NTC1Pin)) >= maxT || getTemp(analogRead(NTC1Pin)) <= minT)
  {
    noti_avaliable = true;
    T1Warn = true;
  }
  if (getTemp(analogRead(NTC2Pin)) >= maxT || getTemp(analogRead(NTC2Pin)) <= minT)
  {
    noti_avaliable = true;
    T2Warn = true;
  }
  if (getTemp(analogRead(NTC3Pin)) >= maxT || getTemp(analogRead(NTC3Pin)) <= minT)
  {
    noti_avaliable = true;
    T3Warn = true;
  }
  if (getTemp(analogRead(NTC4Pin)) >= maxT || getTemp(analogRead(NTC4Pin)) <= minT)
  {
    noti_avaliable = true;
    T4Warn = true;
  }
  if (getCurr(analogRead(CurrPin)) >= maxA)
  {
    noti_avaliable = true;
    AWarn = true;
  }
}
//CLear current warning statuses
void clearWarnings() {
  T1Warn = false;
  T2Warn = false;
  T3Warn = false;
  T4Warn = false;
  V1Warn = false;
  V2Warn = false;
  V3Warn = false;
  V4Warn = false;
}

//Buzzer thread, literally just beep it 2 times and flag as unavaliable
void buzzer()
{
  //If buzzer is avaliable
  if (buzz_rdy)
  {
    pinMode(buzzPin, OUTPUT);
    digitalWrite(buzzPin, HIGH);
    delay(50);
    digitalWrite(buzzPin, LOW);
    delay(100);
    digitalWrite(buzzPin, HIGH);
    delay(50);
    digitalWrite(buzzPin, LOW);
    buzz_rdy = false;
  }
}

//It's map() but with floats
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
