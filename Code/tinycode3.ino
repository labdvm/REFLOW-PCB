#include <SmoothThermistor.h>
#include <TinyWireM.h>
#include <USI_TWI_Master.h>
#include <Tiny4kOLED.h>

float T;
int lasttemp = -1;

int cal = 1;
int avgtemp;

int paste = 0;
int stage = 0;

bool heat = false;

unsigned long startmillis;
unsigned long targetmillis;

// Standard thermistor configuration
SmoothThermistor therm(A3, ADC_SIZE_10_BIT, 100000, 100000, 3950, 25, 5);

void setup(){
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW); // Force heater off on startup

  oled.begin();
  oled.setFont(FONT8X16);
  oled.on();
  oled.clear();
}

void loop(){
  // 1. Read Temperature Safely
  T = therm.temperature();
  avgtemp = (int)T;
  
  // Update main screen only if temperature changes to prevent flickering
  if(avgtemp != lasttemp && !heat){  
    oled.clear();
    oled.setCursor(0, 0);
    oled.print("Temp: ");
    oled.print(avgtemp);
    oled.print(" C");
    
    oled.setCursor(0, 2);
    oled.print("Paste: ");
    if(paste == 0) oled.print("Sn63/Pb37");
    if(paste == 1) oled.print("SAC 305");
    if(paste == 2) oled.print("Sn42/Bi57.6");
    if(paste == 3) oled.print("Test mode");
    lasttemp = avgtemp;
  }

  // 2. Read Buttons (Replaced stuck while loops with safe reads)
  int buttonVal = analogRead(A2);

  // Cycle Paste Type (Voltage window: 300 to 550)
  if(buttonVal > 300 && buttonVal < 550){  
    paste++;
    if(paste >= 4) {
      paste = 0;
    }
    
    oled.clear();
    oled.setCursor(0, 0);
    oled.print("Selected Solder:");
    oled.setCursor(0, 2);
    if(paste == 0) oled.print("Sn64 Pb37");
    if(paste == 1) oled.print("SAC 305");
    if(paste == 2) oled.print("Sn42 Bi57.6");
    if(paste == 3) oled.print("Test Value");
    
    delay(1000); // Give user time to see selection / button debounce
    lasttemp = -1; // Force screen redraw
  }

  // Enable Heating (Voltage window: above 600)
  if(buttonVal > 600 && !heat){  
    oled.clear();
    oled.setCursor(0, 0);
    oled.print("START HEATING!");
    delay(1500);
    heat = true;
  }

  // 3. Execute Selected Profile
  if(heat){
    if(paste == 0) runReflowProfile(125, 90000, 230, 30000); // Sn64: Soak 125C/90s, Reflow 230C/30s
    if(paste == 1) runReflowProfile(160, 90000, 250, 30000); // SAC305: Soak 160C/90s, Reflow 250C/30s
    if(paste == 2) runReflowProfile(110, 90000, 165, 30000); // Sn42: Soak 110C/90s, Reflow 165C/30s
    if(paste == 3) runReflowProfile(50, 10000, 60, 5000);    // Quick Test Run
    
    // Safety shutdown when done
    digitalWrite(1, LOW);
    heat = false;
    lasttemp = -1; // Force screen redraw
    oled.clear();
    oled.setCursor(0, 0);
    oled.print("Cycle Finished!");
    oled.setCursor(0, 2);
    oled.print("Cooling down...");
    delay(5000);
  }
}

// 4. Consolidated, reusable Profile Engine to save ATtiny memory space
void runReflowProfile(int soakTemp, unsigned long soakDurationMs, int reflowTemp, unsigned long reflowDurationMs) {
  
  // --- PHASE 1: RAMP TO SOAK ---
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("Ramp to Soak");
  
  while(1) {
    avgtemp = (int)therm.temperature();
    oled.setCursor(0, 2);
    oled.print("T: "); oled.print(avgtemp); oled.print("C / "); oled.print(soakTemp); oled.print("C  ");
    
    analogWrite(1, 255); // Full power heat up
    if(avgtemp >= soakTemp) break;
    delay(200);
  }

  // --- PHASE 2: SOAKING ---
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("Soaking Profile");
  startmillis = millis();
  
  while(millis() - startmillis < soakDurationMs) {
    avgtemp = (int)therm.temperature();
    oled.setCursor(0, 2);
    oled.print("T: "); oled.print(avgtemp); oled.print("C  ");
    oled.print((soakDurationMs - (millis() - startmillis)) / 1000); oled.print("s ");

    // Simple bang-bang thermostat control
    if(avgtemp < soakTemp) {
      analogWrite(1, 255);
    } else {
      analogWrite(1, 0);
    }
    delay(200);
  }

  // --- PHASE 3: RAMP TO REFLOW ---
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("Ramp to Reflow");
  
  while(1) {
    avgtemp = (int)therm.temperature();
    oled.setCursor(0, 2);
    oled.print("T: "); oled.print(avgtemp); oled.print("C / "); oled.print(reflowTemp); oled.print("C  ");
    
    analogWrite(1, 255);
    if(avgtemp >= reflowTemp) break;
    delay(200);
  }

  // --- PHASE 4: REFLOWING (MELTING) ---
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("Reflowing Paste");
  startmillis = millis();
  
  while(millis() - startmillis < reflowDurationMs) {
    avgtemp = (int)therm.temperature();
    oled.setCursor(0, 2);
    oled.print("T: "); oled.print(avgtemp); oled.print("C  ");
    oled.print((reflowDurationMs - (millis() - startmillis)) / 1000); oled.print("s ");

    if(avgtemp < reflowTemp) {
      analogWrite(1, 255);
    } else {
      analogWrite(1, 0);
    }
    delay(200);
  }
}
