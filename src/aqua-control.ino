#include <Arduino.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <FastLED.h>

#define ON true
#define OFF false
#define SCHEDULE_INTERVAL 2000
#define RED_PIN  9
#define GREEN_PIN 10
#define BLUE_PIN 6
#define MAIN_LIGHT_PIN 3
#define CO2_PIN 5

const byte PROGMEM gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

struct lightSchedule {
  CRGB color;
  byte intensity;
  byte hour;
  byte minute;
};

struct toggleSchedule {
  boolean status;
  byte hour;
  byte minute;
};

const CRGB colorSunrise = CRGB(150, 120, 0);
const CRGB colorSunset = CRGB(255, 60, 0);
const CRGB colorDay = CRGB(0, 200, 255);
const CRGB colorDayShade = CRGB(70, 110, 120);
const CRGB colorNight = CRGB(0, 0, 0);

const lightSchedule schedule[] = {
  { color: colorNight,    intensity: 0,   hour: 0,  minute: 0  },
  { color: colorSunrise,  intensity: 20,  hour: 7,  minute: 30 },
  { color: colorDay,      intensity: 230, hour: 8,  minute: 30 },
  { color: colorDayShade, intensity: 20,  hour: 14, minute: 30 },
  { color: colorDay,      intensity: 230, hour: 16, minute: 30 },
  { color: colorSunset,   intensity: 20,  hour: 20, minute: 30 },
  { color: colorNight,    intensity: 0,   hour: 21, minute: 30 }
};

const toggleSchedule co2Schedule[] = {
  { status: ON,   hour: 7,  minute: 30 },
  { status: OFF,  hour: 12, minute: 0  },
  { status: ON,   hour: 16, minute: 30 },
  { status: OFF,  hour: 19, minute: 0  }
};

unsigned long lastScheduleInterval = 0;
CRGB currentColor = CRGB(0, 0, 0);
byte currentIntensity = 0;
boolean currentCO2Status = OFF;

void setup() {
  Serial.begin(9600);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(MAIN_LIGHT_PIN, OUTPUT);
  pinMode(CO2_PIN, OUTPUT);
}

void loop() {
  TimeElements tm;
  unsigned long now = millis();

  if (now - lastScheduleInterval >= SCHEDULE_INTERVAL
      && RTC.read(tm) && isValidTime(tm)) {
    lastScheduleInterval = now;

    byte length = sizeof(schedule) / sizeof(lightSchedule);
    for (byte i = length - 1; i > 0; i--) {
      lightSchedule item = schedule[i];
      if (tm.Hour > item.hour || (tm.Hour == item.hour && tm.Minute >= item.minute)) {
        currentColor = item.color;
        currentIntensity = item.intensity;
        break;
      }
    }

    length = sizeof(toggleSchedule) / sizeof(co2Schedule);
    for (byte i = length - 1; i > 0; i--) {
      toggleSchedule item = co2Schedule[i];
      if (tm.Hour > item.hour || (tm.Hour == item.hour && tm.Minute >= item.minute)) {
        currentCO2Status = item.status;
        break;
      }
    }

    updateLight();
    updateCO2();
  }
}

boolean isValidTime(const tmElements_t tm) {
  return tm.Hour <= 24 && tm.Hour >= 0;
}

void updateLight() {
  analogWrite(MAIN_LIGHT_PIN, 255 - currentIntensity);
  showAnalogRGB(currentColor);
}

void updateCO2() {
  digitalWrite(CO2_PIN, currentCO2Status == ON ? HIGH : LOW);
}

void showAnalogRGB(const CRGB& rgb) {
  analogWrite(RED_PIN, pgm_read_byte(&gamma[rgb.r]));
  analogWrite(GREEN_PIN, pgm_read_byte(&gamma[rgb.g]));
  analogWrite(BLUE_PIN, pgm_read_byte(&gamma[rgb.b]));
}
