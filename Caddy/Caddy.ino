//Make this readable and futureproofing
//Current pin
#define CurrPin A0
//Voltage pins 
#define Cell1Pin A1
#define Cell2Pin A2
#define Cell3Pin A3
#define Cell4Pin A11
//Thermistor pins
#define NTC1Pin A8
#define NTC2Pin A9
#define NTC3Pin A10
#define NTC4Pin A11

//Resistor values
#define Cell1R1 12000
#define Cell1R2 3000
#define Cell2R1 18000
#define Cell2R2 2000
#define Cell3R1 18000
#define Cell3R2 1300
#define Cell4R1 82000
#define Cell4R2 4300

//This will handle the screen drawing and calculations
#include "screenhandler.h"

//Timer interrupts
#include <Metro.h>

//Instantiate an interrupt set to 5s to switch the screens
Metro serialMetro = Metro(5000);

//Which screen should be showed (Main/Temps)
int screen = 0;

void setup() {
  //Open the serial port
  Serial.begin(9600);
  
  //Initialise screen
  screenInit();

  //MCU status LED
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);

  //buzzer
  pinMode(6, OUTPUT);
}

void loop() {
  //If the timer elapsed, show the next screen and loop
  if (serialMetro.check() == 1) {
  screen++;
    if (screen >= 1)
    {
      screen = 0;
    }
  }

  //Rebuild screen to the selected one
  //All the calculations will be handled on the spot while building
  switch (screen)
  {
    case 0:
    //build the main screen showing the voltages, current and power draw
      buildVoltageScreen();
      break;
    case 1:
    //build the temperatures screen
      buildTempScreen();
      break;
  }
  
  //Send data through serial
  sendToSerial();
}

void sendToSerial() {
  //Voltages
  float Cell1;
  float Cell2;
  float Cell3;
  float Cell4;
  //Current
  float Cur;
  //Power
  float Pow;
  //Temperatures
  float T1;
  float T2;
  float T3;
  float T4;

  //Calculate these values and store as floats
  Cell1 = getVolt(analogRead(Cell1Pin), Cell1R1, Cell1R2, 1);
  Cell2 = getVolt(analogRead(Cell2Pin), Cell2R1, Cell2R2, 2);
  Cell3 = getVolt(analogRead(Cell3Pin), Cell3R1, Cell3R2, 3);
  Cell4 = getVolt(analogRead(Cell4Pin), Cell4R1, Cell4R2, 4);

  Cur = getCurr(analogRead(CurrPin));
  Pow = getCurr(analogRead(CurrPin)) * getVolt(analogRead(Cell4Pin), Cell4R1, Cell4R2, 4);

  T1 = getTemp(analogRead(NTC1Pin));
  T2 = getTemp(analogRead(NTC2Pin));
  T3 = getTemp(analogRead(NTC3Pin));
  T4 = getTemp(analogRead(NTC4Pin));
  
  /*
   * Send the data using the serial port with one long string using this format:
   * Vx: yy.yy => Battery X has yy.yy volts
   * C: yy.yy  => Current flow is yy.yy amps
   * P: yy.yy  => Power being put out of the battery is yy.yy watts
   * Tx: yy.yy => Battery X is at yy.yy°C
   * Data is seperated using ";"
   * Delay between transmittions while testing is 67ms at 528MHz
   */ 
  Serial.println("V1:" + (String)Cell1 + ";V2:" + (String)Cell2 + ";V3:" + (String)Cell3 + ";V4:" + (String)Cell4 + ";C:" + (String)Cur + ";P:" + (String)Pow + ";T1:" + (String)T1 + ";T2:" + (String)T2 + ";T3:" + (String)T3 + ";T4:" + (String)T4);
}
