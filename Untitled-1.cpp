#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <map>

// =========================
// WiFi Credentials
// =========================
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// =========================
// SinricPro Credentials
// =========================
#define APP_KEY       "YOUR_APP_KEY"
#define APP_SECRET    "YOUR_APP_SECRET"

// =========================
// Device IDs
// =========================
#define DEVICE_ID_1   "6a2ffb8229c6be3342603ba5"
#define DEVICE_ID_2   "YOUR_DEVICE_ID_2"
#define DEVICE_ID_3   "YOUR_DEVICE_ID_3"
#define DEVICE_ID_4   "YOUR_DEVICE_ID_4"

// =========================
// ESP32 Relay Pins
// =========================
#define RelayPin1 23
#define RelayPin2 22
#define RelayPin3 21
#define RelayPin4 19

// =========================
// ESP32 Switch Pins
// =========================
#define SwitchPin1 18
#define SwitchPin2 5
#define SwitchPin3 17
#define SwitchPin4 16

// =========================
// WiFi LED
// =========================
#define wifiLed 2

#define BAUD_RATE 115200
#define DEBOUNCE_TIME 250

typedef struct {
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;

std::map<String, deviceConfig_t> devices = {
  {DEVICE_ID_1, {RelayPin1, SwitchPin1}},
  {DEVICE_ID_2, {RelayPin2, SwitchPin2}},
  {DEVICE_ID_3, {RelayPin3, SwitchPin3}},
  {DEVICE_ID_4, {RelayPin4, SwitchPin4}}
};

typedef struct {
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;

// =========================
// Relay Setup
// =========================
void setupRelays() {
  for (auto &device : devices) {
    pinMode(device.second.relayPIN, OUTPUT);
    digitalWrite(device.second.relayPIN, HIGH); // Relay OFF
  }
}

// =========================
// Switch Setup
// =========================
void setupFlipSwitches() {
  for (auto &device : devices) {

    flipSwitchConfig_t flipSwitchConfig;

    flipSwitchConfig.deviceId = device.first;
    flipSwitchConfig.lastFlipSwitchChange = 0;
    flipSwitchConfig.lastFlipSwitchState = HIGH;

    int flipSwitchPIN = device.second.flipSwitchPIN;

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;

    pinMode(flipSwitchPIN, INPUT_PULLUP);
  }
}

// =========================
// SinricPro Callback
// =========================
bool onPowerState(const String &deviceId, bool &state) {

  Serial.printf("Device %s turned %s\n",
                deviceId.c_str(),
                state ? "ON" : "OFF");

  int relayPIN = devices[deviceId].relayPIN;

  digitalWrite(relayPIN, !state);

  return true;
}

// =========================
// Physical Switch Control
// =========================
void handleFlipSwitches() {

  unsigned long actualMillis = millis();

  for (auto &flipSwitch : flipSwitches) {

    if (actualMillis - flipSwitch.second.lastFlipSwitchChange > DEBOUNCE_TIME) {

      int flipSwitchPIN = flipSwitch.first;

      bool flipSwitchState = digitalRead(flipSwitchPIN);

      if (flipSwitchState != flipSwitch.second.lastFlipSwitchState) {

        flipSwitch.second.lastFlipSwitchChange = actualMillis;
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;

        if (flipSwitchState == LOW) {

          String deviceId = flipSwitch.second.deviceId;

          int relayPIN = devices[deviceId].relayPIN;

          bool relayState = digitalRead(relayPIN);

          digitalWrite(relayPIN, !relayState);

          SinricProSwitch &mySwitch = SinricPro[deviceId];

          mySwitch.sendPowerStateEvent(relayState);
        }
      }
    }
  }
}

// =========================
// WiFi Setup
// =========================
void setupWiFi() {

  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(wifiLed, LOW);
}

// =========================
// SinricPro Setup
// =========================
void setupSinricPro() {

  for (auto &device : devices) {

    const char *deviceId = device.first.c_str();

    SinricProSwitch &mySwitch = SinricPro[deviceId];

    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);

  SinricPro.restoreDeviceStates(true);
}

// =========================
// Setup
// =========================
void setup() {

  Serial.begin(BAUD_RATE);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, HIGH);

  setupRelays();
  setupFlipSwitches();

  setupWiFi();

  setupSinricPro();
}

// =========================
// Loop
// =========================
void loop() {

  SinricPro.handle();

  handleFlipSwitches();
}