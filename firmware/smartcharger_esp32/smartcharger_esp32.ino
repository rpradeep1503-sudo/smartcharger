// SmartCharger ESP32 demo: emulator mode (SAFE, no real battery charging).
// Pot on GPIO34 = fake SOC. DS18B20 on GPIO4 = temperature. PWM on GPIO25 = commanded current.
// Libraries (Library Manager): OneWire, DallasTemperature. Board package: esp32 by Espressif (3.x).
// NOTE: not yet tested on hardware. Test one sensor at a time first.
#include <OneWire.h>
#include <DallasTemperature.h>

const int PIN_POT = 34, PIN_TEMP = 4, PIN_PWM = 25;
const float CAP_AH = 2.5, V_MAX = 4.2;
const float MAX_CURRENT = 2.5 * CAP_AH;   // amps at 2.5C

OneWire oneWire(PIN_TEMP);
DallasTemperature sensors(&oneWire);

float ocv(float soc) { return 3.0 + 1.2 * soc - 0.15 * exp(-12.0 * soc) + 0.1 * soc * soc * soc; }

float adaptiveCurrent(float soc, float temp, float volt) {
  float c = (soc < 0.30) ? 2.5 : (soc < 0.60) ? 2.5 * 0.7 : (soc < 0.80) ? 2.5 * 0.4 : 0.15;
  if (temp > 45) return 0;                       // hard safety cutoff
  if (temp > 38) c *= (45 - temp) / 7.0;
  if (temp < 10) c = min(c, 0.2f);
  if (volt >= V_MAX) c *= 0.5;
  return c * CAP_AH;
}

void setup() {
  Serial.begin(115200);
  sensors.begin();
  ledcAttach(PIN_PWM, 5000, 8);                  // ESP32 core 3.x. For core 2.x use ledcSetup + ledcAttachPin.
  Serial.println("soc,temp_C,volt_V,current_A,status");
}

void loop() {
  float soc = analogRead(PIN_POT) / 4095.0;      // pot sets fake SOC 0..1
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  if (temp < -100) temp = 25;                    // sensor missing: assume 25 C
  float volt = ocv(soc);
  float cmd = adaptiveCurrent(soc, temp, volt);
  ledcWrite(PIN_PWM, (int)(255 * constrain(cmd / MAX_CURRENT, 0.0f, 1.0f)));
  Serial.printf("%.2f,%.1f,%.2f,%.2f,%s\n", soc, temp, volt, cmd, temp > 45 ? "OVERTEMP" : "OK");
  delay(500);
}
