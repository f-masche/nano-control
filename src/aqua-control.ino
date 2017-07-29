#include <Arduino.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <FastLED.h>
#include <LiquidCrystal_PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SET_TIME false
#define TEMP_SENSOR_PIN 8
#define MAX_TEMP 24
#define FAN_PIN 6
#define DEMO_MODE false
#define DEMO_INTERVAL 600
#define MAIN_LIGHT_PIN 3
#define SUNRISE 3600L * 7
#define SUNSET 3600L * 21

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
LiquidCrystal_PCF8574 lcd(0x3F);

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

byte temperatureChar[8] = {
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110,
};

byte fanChar[8] = {
  B10101,
  B01010,
  B00000,
  B10101,
  B01010,
  B00000,
  B10101,
  B01010,
};

byte lightChar[8] = {
  B00100,
  B10101,
  B01110,
  B11111,
  B01110,
  B10101,
  B00100,
  B00000,
};

tmElements_t tm;
unsigned long demoTime = 3600 * 7L;
unsigned long currentSeconds = 0;

unsigned long getTime(byte hours, byte minutes);

void setup() {
  Serial.begin(9600);
  while (!Serial); // wait until Arduino Serial Monitor opens

#if SET_TIME
  setTime();
#endif
  setupLcd();
  setupClock();
  pinMode(MAIN_LIGHT_PIN, OUTPUT);
  sensors.begin();
}

void loop() {
#if DEMO_MODE
 currentSeconds = demoTime;
 demoTime += 5;
#else
  currentSeconds = second() + minute() * 60L + hour() * 3600L;
#endif

  byte lightIntensity = getSunlight(SUNRISE, SUNSET, currentSeconds);
  setLight(lightIntensity);
  lcd.setBacklight(lightIntensity >= 5 ? 1 : 0);
  
  float temperature = getTemperature();
  byte fanSpeed = getFanSpeed(temperature, MAX_TEMP);
  setFanSpeed(fanSpeed);

  printDisplay(lightIntensity, temperature, fanSpeed);
}

void setupLcd() {
  lcd.begin(16, 2);
  lcd.createChar(0, temperatureChar);
  lcd.createChar(1, fanChar);
  lcd.createChar(2, lightChar);
  lcd.setBacklight(1);
}

void setTime() {
  if (getDate(__DATE__) && getTime(__TIME__)) {
    RTC.write(tm);
  }
}

void setupClock() {
  setSyncProvider(RTC.get);
  setSyncInterval(60);
  if(timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }
}



float getTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

byte getFanSpeed(float temperature, byte maxTemp) {
  float dt = temperature - maxTemp;
  if (dt > 0) {
    return min(255, dt * 255);
  } else {
    return 0;
  }
}

void setFanSpeed(byte fanSpeed) {
  analogWrite(FAN_PIN, fanSpeed);
}

byte getSunlight(long sunrise, long sunset, long time) {
  if (time < sunrise || time > sunset) {
    return 0;
  }

  return cos((time - sunrise) * (PI * 2 / (sunset - sunrise)) - PI) * 127 + 127;
}

void setLight(byte intensity) {
  analogWrite(MAIN_LIGHT_PIN, 255 - intensity);
}

unsigned long getTime(byte hours, byte minutes) {
  return 3600L * hours + 60L * minutes;
}

void print2digits(int num) {
  if(num < 10) {
    lcd.print('0');
  }
  lcd.print(num);
}

void printDisplay(byte lightIntensity, float temperature, byte fanSpeed) {
  lcd.setCursor(0,0);
  print2digits(hour());
  lcd.print(':');
  print2digits(minute());
  lcd.print(':');
  print2digits(second());

  int currentIntensityPercent = lightIntensity / 255.0 * 100;
  if(currentIntensityPercent < 10) {
    lcd.print(' ');
  }
  if(currentIntensityPercent < 100) {
    lcd.print(' ');
  }

  lcd.print((char)2);
  lcd.print(currentIntensityPercent);
  lcd.print('%');

  lcd.setCursor(0,1);
  lcd.print((char)0);
  lcd.print(temperature);
  lcd.print((char)223);
  lcd.print('C');
  lcd.print(' ');

  byte fanSpeedPercent = fanSpeed / 255.0 * 100;
  if(fanSpeedPercent < 10) {
    lcd.print(' ');
  }
  if(fanSpeedPercent < 100) {
    lcd.print(' ');
  }
  lcd.print((char)1);
  lcd.print(fanSpeedPercent);
  lcd.print('%');
}

bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

bool getTime(const char *str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}
