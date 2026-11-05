#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <RTClib.h>

// ---------- Settings ----------
const long BAUD = 4800;                 // set the Serial Monitor to the same speed
const bool IGNORE_TIME_WINDOW = true;   // true = always track (bench test)
const bool PARK_AT_START = true;       // true = move to the start position when powered on
const uint8_t START_HOUR = 9;
const uint8_t END_HOUR   = 17;
const int tol   = 30;             // tolerance: adjust for sensitivity
const int dtime = 10;             // ms between tracking steps
const int HORIZ_DIR = 1;          // use -1 if it turns away from the light (left/right)
const int VERT_DIR  = 1;          // use -1 if it turns away from the light (up/down)
const unsigned long LOG_MS = 5000;      // print (and save, if a card is in) every 5 seconds
const bool SET_CLOCK = false;     // true once to set the clock, then false and upload again

// ---------- Servos ----------
Servo horizontal;                 // horizontal servo motor
int servohori = 180;
int servohoriLimitHigh = 175;
int servohoriLimitLow  = 5;

Servo vertical;                   // vertical servo motor
int servovert = 45;
int servovertLimitHigh = 160;
int servovertLimitLow  = 1;

// ---------- Pins ----------
int ldrlt = A0;                   // Top-Left LDR
int ldrrt = A3;                   // Top-Right LDR
int ldrld = A1;                   // Bottom-Left LDR
int ldrrd = A2;                   // Bottom-Right LDR
const uint8_t HORIZ_PIN = 5, VERT_PIN = 4, SD_CS = 10;   // keep 10 to 13 for the SD module

// ---------- Variables ----------
RTC_DS3231 rtc;
int lt = 0, rt = 0, ld = 0, rd = 0;
bool rtcOk = false, sdOk = false, wasActive = false;
unsigned long lastTrack = 0, lastLog = 0;

bool i2cPresent(uint8_t addr) {   // true only if a device answers at this address
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void park() {                     // go back to the starting position
  servohori = 180;
  servovert = 45;
  horizontal.write(servohori);
  vertical.write(servovert);
}

void track() {
  lt = analogRead(ldrlt);
  rt = analogRead(ldrrt);
  ld = analogRead(ldrld);
  rd = analogRead(ldrrd);

  int avt = (lt + rt) / 2;        // average top
  int avd = (ld + rd) / 2;        // average bottom
  int avl = (lt + ld) / 2;        // average left
  int avr = (rt + rd) / 2;        // average right

  int dvert  = avt - avd;
  int dhoriz = avl - avr;

  if (abs(dvert) > tol) {
    if (avt > avd) servovert -= VERT_DIR;
    else           servovert += VERT_DIR;
    servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);
    vertical.write(servovert);
  }

  if (abs(dhoriz) > tol) {
    if (avl > avr) servohori -= HORIZ_DIR;
    else           servohori += HORIZ_DIR;
    servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);
    horizontal.write(servohori);
  }
}

void logData() {
  char ts[20], name[13];
  if (rtcOk) {
    DateTime now = rtc.now();
    snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d", (int)now.year(),
             (int)now.month(), (int)now.day(), (int)now.hour(), (int)now.minute(),
             (int)now.second());
    snprintf(name, sizeof(name), "%02d%02d%02d.CSV",
             (int)(now.year() % 100), (int)now.month(), (int)now.day());
  } else {
    snprintf(ts, sizeof(ts), "t+%lus", millis() / 1000);   // no clock: seconds since start
    strcpy(name, "TEST.CSV");
  }

  // Always print to the Serial Monitor (works with or without a card or clock)
  Serial.print(ts);
  Serial.print(F("  lt ")); Serial.print(lt);
  Serial.print(F(" rt ")); Serial.print(rt);
  Serial.print(F(" ld ")); Serial.print(ld);
  Serial.print(F(" rd ")); Serial.print(rd);
  Serial.print(F("  hori ")); Serial.print(servohori);
  Serial.print(F(" vert ")); Serial.println(servovert);

  if (!sdOk) return;              // no card: skip saving, keep running

  bool isNew = !SD.exists(name);
  File f = SD.open(name, FILE_WRITE);
  if (!f) return;                 // could not open the file: skip, keep running
  if (isNew) f.println(F("time,lt,rt,ld,rd,hori,vert"));
  f.print(ts);
  f.print(','); f.print(lt);
  f.print(','); f.print(rt);
  f.print(','); f.print(ld);
  f.print(','); f.print(rd);
  f.print(','); f.print(servohori);
  f.print(','); f.println(servovert);
  f.close();                      // closing saves the data safely
}

void setup() {
  Serial.begin(BAUD);
  Serial.println(F("Ok."));

  // 1) Servos first, so the tracker never depends on the clock or the card
  horizontal.attach(HORIZ_PIN);
  vertical.attach(VERT_PIN);
  if (PARK_AT_START) park();

  // 2) Clock (optional): a stuck or missing I2C device can no longer freeze the sketch
  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);       // give up after 25 ms instead of waiting forever
#endif
  rtcOk = i2cPresent(0x68) && rtc.begin();
  if (!rtcOk) {
    Serial.println(F("No clock found: running without time"));
  } else {
    Serial.println(F("Clock ready"));
    if (SET_CLOCK || rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));   // set to the time of upload
      Serial.println(F("Clock set"));
    }
  }

  // 3) SD card (optional)
  sdOk = SD.begin(SD_CS);
  if (sdOk) Serial.println(F("SD card ready"));
  else      Serial.println(F("No SD card: printing to Serial only"));

  delay(2500);
  Serial.println(F("Tracker ready"));
}

void loop() {
  bool active = true;
  if (rtcOk && !IGNORE_TIME_WINDOW) {
    DateTime now = rtc.now();
    active = (now.hour() >= START_HOUR && now.hour() < END_HOUR);
  }

  if (!active) {                  // outside the time window: park and wait
    if (wasActive) {
      park();
      wasActive = false;
    }
    delay(1000);
    return;
  }
  wasActive = true;

  unsigned long t = millis();
  if (t - lastTrack >= (unsigned long)dtime) {
    lastTrack = t;
    track();
  }
  if (t - lastLog >= LOG_MS) {
    lastLog = t;
    logData();
  }
}