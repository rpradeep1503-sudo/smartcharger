// SmartCharge rig: low-voltage EV-charger prototype on an ESP32. NO real battery is charged.
// A power resistor (load) stands in for the heat of fast charging. The ESP32 picks a charging
// current from an emulated battery (pot = start SOC), drives the load with PWM, measures it with
// the INA219, reads temperature with DS18B20 sensors and runs a smart cooling fan.
//
// Libraries (Arduino Library Manager): OneWire, DallasTemperature, Adafruit INA219 (and its dependencies).
// Board: "ESP32 Dev Module" (esp32 by Espressif). Works with core 2.x and 3.x.
// NOT yet tested on hardware: test each part alone first (see hardware/wiring.md).
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_INA219.h>

// ---------- pins ----------
const int PIN_ONEWIRE = 4;    // DS18B20 data (4.7k pull-up to 3.3V)
const int PIN_POT     = 34;   // emulated battery start SOC
const int PIN_PIR     = 27;   // "vehicle arrived" sensor
const int PIN_LOAD    = 25;   // MOSFET 1 gate: resistor load (charging power)
const int PIN_FAN     = 26;   // MOSFET 2 gate: cooling fan
// I2C for INA219: SDA 21, SCL 22

// ---------- PWM (works on core 2.x and 3.x) ----------
const int PWM_FREQ = 20000, PWM_BITS = 8, PWM_MAX = 255, CH_LOAD = 0, CH_FAN = 1;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define PWM_SETUP(pin, ch) ledcAttach(pin, PWM_FREQ, PWM_BITS)
  #define PWM_OUT(pin, ch, v) ledcWrite(pin, v)
#else
  #define PWM_SETUP(pin, ch) do { ledcSetup(ch, PWM_FREQ, PWM_BITS); ledcAttachPin(pin, ch); } while (0)
  #define PWM_OUT(pin, ch, v) ledcWrite(ch, v)
#endif

// ---------- virtual cell (same numbers as the Python model) ----------
const float CAP_AH = 2.5, R0 = 0.06, V_MAX = 4.2;
const float C_MAX_RATE = 2.5;                    // highest C-rate = 100% PWM
const float MAX_CMD_A = C_MAX_RATE * CAP_AH;     // 6.25 A virtual cell current at full PWM
const float SOC_END = 0.80;
const float TIMESCALE = 60.0;                    // 1 real second = 60 simulated seconds of charging

// ---------- thresholds (lower them for a cool room so the demo reacts) ----------
float T_WARN = 38.0, T_STOP = 45.0;              // derate above T_WARN, stop above T_STOP
float T_FAN_ON = 32.0, T_FAN_FULL = 40.0;        // smart fan ramp
const float FAN_W_MAX = 1.0;                     // watts at 100% duty (measure your fan)

enum State { IDLE, CHARGING, DONE, SAFETY };
const char* STATE_NAMES[] = {"IDLE", "CHARGING", "DONE", "SAFETY_STOP"};
State state = IDLE;
int fanMode = 2;                                 // 0 off, 1 always on, 2 smart
int strat = 1;                                   // 0 baseline 1C, 1 adaptive, 2 naive 2.5C

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensors(&oneWire);
Adafruit_INA219 ina;
WebServer server(80);

float socSim = 0.4, cmdA = 0, fanDuty = 0, tHot = 25, tAir = 25, busV = 0, loadmA = 0, fanWh = 0, potSoc = 0.4;
bool tHotValid = false, inaOk = false;
uint32_t pirReadyAt = 0, sessionStart = 0;

// ---------- model ----------
float ocv(float s) { return 3.0 + 1.2 * s - 0.15 * expf(-12.0 * s) + 0.1 * s * s * s; }

float adaptiveCurrent(float soc, float T, float v) {
  float c;
  if (soc < 0.30) c = C_MAX_RATE; else if (soc < 0.60) c = C_MAX_RATE * 0.7; else if (soc < 0.80) c = C_MAX_RATE * 0.4; else c = 0.15;
  if (T > T_STOP) return 0;
  if (T > T_WARN) c *= (T_STOP - T) / (T_STOP - T_WARN);
  if (T < 10) c = fminf(c, 0.2f);
  if (v >= V_MAX) c *= 0.5;
  return c * CAP_AH;
}

float chooseCurrent() {
  float v = ocv(socSim);
  if (strat == 0) return (socSim < SOC_END) ? 1.0 * CAP_AH : 0;
  if (strat == 2) return (socSim < SOC_END) ? C_MAX_RATE * CAP_AH : 0;
  return adaptiveCurrent(socSim, tHot, v);
}

float chooseFan(bool tempOk) {
  if (fanMode == 0) return 0;
  if (fanMode == 1) return 1;
  if (!tempOk || state == SAFETY) return 1;
  float d = constrain((tHot - T_FAN_ON) / (T_FAN_FULL - T_FAN_ON), 0.0f, 1.0f);
  if (d > 0 && d < 0.35f) d = 0.35f;             // fans stall at very low duty
  return d;
}

// ---------- sensors ----------
void tempTask() {
  static uint32_t tReq = 0; static bool pending = false;
  uint32_t now = millis();
  if (!pending && now - tReq >= 1000) { sensors.requestTemperatures(); tReq = now; pending = true; }
  else if (pending && now - tReq >= 800) {
    float a = sensors.getTempCByIndex(0), b = sensors.getTempCByIndex(1);
    tHotValid = (a > -100 && a < 125);
    if (tHotValid) tHot = a;
    tAir = (b > -100 && b < 125) ? b : tHot;
    pending = false;
  }
}

void startSession() {
  if (!tHotValid) { Serial.println("Cannot start: no temperature reading"); return; }
  socSim = fmaxf(0.05f, potSoc); fanWh = 0; sessionStart = millis(); state = CHARGING;
  Serial.println("Session started");
}

void controlStep(float dt) {
  potSoc = fminf(0.70f, analogRead(PIN_POT) / 4095.0f * 0.70f);   // pot: 0..70% start SOC
  bool tempOk = tHotValid && tHot < 125;
  if (state == IDLE) {
    cmdA = 0; socSim = potSoc;
    if (millis() > pirReadyAt && digitalRead(PIN_PIR) == HIGH) startSession();
  } else if (state == CHARGING) {
    if (!tempOk || tHot > T_STOP) { state = SAFETY; cmdA = 0; }
    else {
      cmdA = fminf(chooseCurrent(), MAX_CMD_A);
      socSim += cmdA * dt * TIMESCALE / (3600.0f * CAP_AH);
      if (socSim >= SOC_END) { socSim = SOC_END; state = DONE; cmdA = 0; }
    }
  } else if (state == SAFETY) {
    cmdA = 0;
    if (tempOk && tHot < T_STOP - 5) state = CHARGING;
  } else { cmdA = 0; }

  fanDuty = chooseFan(tempOk);
  fanWh += FAN_W_MAX * fanDuty * dt / 3600.0f;
  PWM_OUT(PIN_LOAD, CH_LOAD, (int)(constrain(cmdA / MAX_CMD_A, 0.0f, 1.0f) * PWM_MAX));
  PWM_OUT(PIN_FAN, CH_FAN, (int)(fanDuty * PWM_MAX));
  if (inaOk) { busV = ina.getBusVoltage_V(); loadmA = 0.8f * loadmA + 0.2f * ina.getCurrent_mA(); }
}

// ---------- reporting ----------
String jsonData() {
  float vv = fminf(V_MAX, ocv(socSim) + cmdA * R0);
  char b[320];
  snprintf(b, sizeof(b),
    "{\"soc\":%.3f,\"temp\":%.1f,\"current\":%.2f,\"volt\":%.2f,\"air\":%.1f,\"fan\":%.2f,\"load_mA\":%.0f,\"bus_V\":%.2f,\"fan_Wh\":%.3f,\"state\":\"%s\",\"fan_mode\":%d,\"strategy\":%d}",
    socSim, tHot, cmdA, vv, tAir, fanDuty, loadmA, busV, fanWh, STATE_NAMES[state], fanMode, strat);
  return String(b);
}

void handleCmd() {
  if (server.hasArg("s")) startSession();
  if (server.hasArg("x")) { state = IDLE; cmdA = 0; }
  if (server.hasArg("f")) fanMode = (fanMode + 1) % 3;
  if (server.hasArg("c")) strat = (strat + 1) % 3;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", jsonData());
}

void handleSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == 's') startSession();
    else if (ch == 'x') { state = IDLE; cmdA = 0; }
    else if (ch == 'f') { fanMode = (fanMode + 1) % 3; Serial.printf("fan mode %d (0 off, 1 on, 2 smart)\n", fanMode); }
    else if (ch == 'c') { strat = (strat + 1) % 3; Serial.printf("strategy %d (0 base, 1 adaptive, 2 naive)\n", strat); }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  PWM_SETUP(PIN_LOAD, CH_LOAD); PWM_SETUP(PIN_FAN, CH_FAN);
  PWM_OUT(PIN_LOAD, CH_LOAD, 0); PWM_OUT(PIN_FAN, CH_FAN, 0);
  Wire.begin(21, 22);
  inaOk = ina.begin();
  if (inaOk) ina.setCalibration_32V_2A(); else Serial.println("INA219 not found: check wiring");
  sensors.begin(); sensors.setWaitForConversion(false);
  Serial.printf("DS18B20 sensors found: %d\n", sensors.getDeviceCount());
  WiFi.softAP("SmartCharge", "charge1234");
  server.on("/", []() { server.send(200, "text/html", "<h3>SmartCharge rig</h3><a href=/data>data</a> | <a href='/cmd?s=1'>start</a> | <a href='/cmd?x=1'>reset</a> | <a href='/cmd?f=1'>fan mode</a> | <a href='/cmd?c=1'>strategy</a>"); });
  server.on("/data", []() { server.sendHeader("Access-Control-Allow-Origin", "*"); server.send(200, "application/json", jsonData()); });
  server.on("/cmd", handleCmd);
  server.begin();
  pirReadyAt = millis() + 30000;                 // PIR needs about 30 s to settle after power-up
  Serial.println("ms,state,soc,tHot,tAir,cmd_A,load_mA,bus_V,fan,fanMode,strategy");
  Serial.println("Commands: s start, x reset, f fan mode, c strategy. Wi-Fi: SmartCharge / charge1234, http://192.168.4.1/data");
}

void loop() {
  server.handleClient();
  handleSerial();
  tempTask();
  static uint32_t lastCtl = 0, lastPrint = 0;
  uint32_t now = millis();
  if (now - lastCtl >= 200) { controlStep((now - lastCtl) / 1000.0f); lastCtl = now; }
  if (now - lastPrint >= 1000) {
    lastPrint = now;
    Serial.printf("%lu,%s,%.3f,%.1f,%.1f,%.2f,%.0f,%.2f,%.2f,%d,%d\n", (unsigned long)now, STATE_NAMES[state], socSim, tHot, tAir, cmdA, loadmA, busV, fanDuty, fanMode, strat);
  }
}
