#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <RTClib.h>
#include <Adafruit_INA219.h>

const long BAUD = 4800;
const bool IGNORE_TIME_WINDOW = true;
const bool PARK_AT_START = true;
const bool SERVOS_ON = true;
const bool ENABLE_INA = true;
const bool SERIAL_ALSO_WITH_SD = true;
const bool SET_CLOCK = false;
const uint8_t START_HOUR = 9;
const uint8_t END_HOUR   = 17;
const int tol   = 30;
const int dtime = 10;
const int HORIZ_DIR = 1;
const int VERT_DIR  = 1;
const unsigned long LOG_MS       = 5000;
const unsigned long POWER_MS     = 1000;
const unsigned long INA_RETRY_MS = 10000;
const uint8_t INA_PANEL_ADDR = 0x40;
const uint8_t INA_UNO_ADDR   = 0x41;
const uint8_t HORIZ_PIN = 5, VERT_PIN = 4, SD_CS = 10;
int ldrlt = A0;
int ldrrt = A3;
int ldrld = A1;
int ldrrd = A2;

Servo horizontal;
int servohori = 180;
int servohoriLimitHigh = 175;
int servohoriLimitLow  = 5;

Servo vertical;
int servovert = 45;
int servovertLimitHigh = 160;
int servovertLimitLow  = 1;

RTC_DS3231 rtc;
Adafruit_INA219 inaPanel(INA_PANEL_ADDR);
Adafruit_INA219 inaUno(INA_UNO_ADDR);
int lt = 0, rt = 0, ld = 0, rd = 0;
bool rtcOk = false, sdOk = false, wasActive = false;
bool inaPanelOk = false, inaUnoOk = false;
float pV = 0, pmA = 0, pmW = 0;
float uV = 0, umA = 0, umW = 0;
float netmW = 0, netmWh = 0;
unsigned long lastTrack = 0, lastLog = 0, lastPower = 0, lastEnergy = 0, lastRetry = 0;

bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void park() {
  servohori = 180;
  servovert = 45;
  horizontal.write(servohori);
  vertical.write(servovert);
}

void retryIna() {
  if (!ENABLE_INA) return;
  if (inaPanelOk && inaUnoOk) return;
  if (millis() - lastRetry < INA_RETRY_MS) return;
  lastRetry = millis();
  if (!inaPanelOk) inaPanelOk = i2cPresent(INA_PANEL_ADDR) && inaPanel.begin();
  if (!inaUnoOk)   inaUnoOk   = i2cPresent(INA_UNO_ADDR) && inaUno.begin();
}

void readPower() {
  if (inaPanelOk) {
    pV  = inaPanel.getBusVoltage_V();
    pmA = inaPanel.getCurrent_mA();
    pmW = inaPanel.getPower_mW();
  } else {
    pV = pmA = pmW = 0;
  }
  if (inaUnoOk) {
    uV  = inaUno.getBusVoltage_V();
    umA = inaUno.getCurrent_mA();
    umW = inaUno.getPower_mW();
  } else {
    uV = umA = umW = 0;
  }
  unsigned long t = millis();
  if (inaPanelOk && inaUnoOk) {
    netmW = pmW - umW;
    netmWh += netmW * (t - lastEnergy) / 3600000.0;
  } else {
    netmW = 0;
  }
  lastEnergy = t;
}

void track() {
  lt = analogRead(ldrlt);
  rt = analogRead(ldrrt);
  ld = analogRead(ldrld);
  rd = analogRead(ldrrd);

  int avt = (lt + rt) / 2;
  int avd = (ld + rd) / 2;
  int avl = (lt + ld) / 2;
  int avr = (rt + rd) / 2;

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
    snprintf(ts, sizeof(ts), "t+%lus", millis() / 1000);
    strcpy(name, "TEST.CSV");
  }

  if (!sdOk || SERIAL_ALSO_WITH_SD) {
    Serial.print(ts);
    Serial.print(F("  lt ")); Serial.print(lt);
    Serial.print(F(" rt ")); Serial.print(rt);
    Serial.print(F(" ld ")); Serial.print(ld);
    Serial.print(F(" rd ")); Serial.print(rd);
    Serial.print(F("  hori ")); Serial.print(servohori);
    Serial.print(F(" vert ")); Serial.print(servovert);
    Serial.print(F("  P ")); Serial.print(pV, 2); Serial.print(F("V "));
    Serial.print(pmA, 0); Serial.print(F("mA ")); Serial.print(pmW, 0); Serial.print(F("mW"));
    Serial.print(F("  U ")); Serial.print(uV, 2); Serial.print(F("V "));
    Serial.print(umA, 0); Serial.print(F("mA ")); Serial.print(umW, 0); Serial.print(F("mW"));
    Serial.print(F("  net ")); Serial.print(netmW, 0); Serial.print(F("mW "));
    Serial.print(netmWh, 2); Serial.println(F("mWh"));
  }

  if (!sdOk) return;

  bool isNew = !SD.exists(name);
  File f = SD.open(name, FILE_WRITE);
  if (!f) return;
  if (isNew) f.println(F("time,lt,rt,ld,rd,hori,vert,pV,pmA,pmW,uV,umA,umW,netmW,netmWh"));
  f.print(ts);
  f.print(','); f.print(lt);
  f.print(','); f.print(rt);
  f.print(','); f.print(ld);
  f.print(','); f.print(rd);
  f.print(','); f.print(servohori);
  f.print(','); f.print(servovert);
  f.print(','); f.print(pV, 2);
  f.print(','); f.print(pmA, 1);
  f.print(','); f.print(pmW, 1);
  f.print(','); f.print(uV, 2);
  f.print(','); f.print(umA, 1);
  f.print(','); f.print(umW, 1);
  f.print(','); f.print(netmW, 1);
  f.print(','); f.println(netmWh, 3);
  f.close();
}

void setup() {
  Serial.begin(BAUD);
  Serial.println(F("Ok."));

  if (SERVOS_ON) {
    horizontal.attach(HORIZ_PIN);
    vertical.attach(VERT_PIN);
    if (PARK_AT_START) park();
  } else {
    Serial.println(F("Servos off"));
  }

  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif

  rtcOk = i2cPresent(0x68) && rtc.begin();
  if (!rtcOk) {
    Serial.println(F("No clock found: running without time"));
  } else {
    Serial.println(F("Clock ready"));
    if (SET_CLOCK || rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println(F("Clock set"));
    }
  }

  if (ENABLE_INA) {
    inaPanelOk = i2cPresent(INA_PANEL_ADDR) && inaPanel.begin();
    inaUnoOk   = i2cPresent(INA_UNO_ADDR) && inaUno.begin();
    Serial.println(inaPanelOk ? F("INA219 #1 (panel) ready") : F("No INA219 #1: running without it"));
    Serial.println(inaUnoOk   ? F("INA219 #2 (Uno) ready")   : F("No INA219 #2: running without it"));
  } else {
    Serial.println(F("INA219 disabled"));
  }

  sdOk = SD.begin(SD_CS);
  if (sdOk) Serial.println(F("SD card ready"));
  else      Serial.println(F("No SD card: printing to Serial only"));

  delay(2500);
  lastEnergy = lastPower = lastRetry = millis();
  Serial.println(F("Tracker ready"));
}

void loop() {
  bool active = true;
  if (rtcOk && !IGNORE_TIME_WINDOW) {
    DateTime now = rtc.now();
    active = (now.hour() >= START_HOUR && now.hour() < END_HOUR);
  }

  if (!active) {
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
  if (t - lastPower >= POWER_MS) {
    lastPower = t;
    retryIna();
    readPower();
  }
  if (t - lastLog >= LOG_MS) {
    lastLog = t;
    logData();
  }
}