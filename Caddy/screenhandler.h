#ifndef SCREEN_HANDLE
#define SCREEN_HANDLE

void buildTempScreen();
void buildVoltageScreen();
void screenInit();
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max);
float getVolt(float analogIn, float R1, float R2, int CellID);
float getCurr(float analogIn);
float getTemp(float analogIn);
void drawVolt(char* txt, float analogIn, float R1, float R2, int x, int y, int CellID);
void drawTemps(char* txt, float analogIn, int x, int y, int CellID);
void checkForWarnings();
void clearWarnings();
void buzzer();
void drawLines();
void drawLinesTemps();

#endif
