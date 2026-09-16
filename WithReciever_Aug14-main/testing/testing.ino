#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <time.h>
#include <Preferences.h>
#include <ArduinoOTA.h>

#define OLED_SDA 21
#define OLED_SCL 22

#define WIFI_SSID "ALT"
#define WIFI_PASSWORD "baDge7Es"
#define DATABASE_SECRET "DvhZNBNmaFVHMFSup70ezVZlEVHWwO87OrGK4Td7"
#define DATABASE_URL "https://pcc-stats-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Password required for wireless (OTA) reflashing from Arduino IDE - change
// this per-device if you want each room to require a different password,
// or leave it the same across all rooms for simplicity. Without a password,
// anyone on the same WiFi network could push arbitrary firmware to this
// device's IR-controlled relay/AC hardware.
#define OTA_PASSWORD "pccstats-ota-2026"

// CHANGE THIS TO YOUR ROOM ID (e.g., "Room_1", "Room_402", "Room_403", etc.)
#define ROOM_ID "Room_1"

// Set to true if this room has 2 AC units (requires 2 IR LEDs)
#define DUAL_UNIT_MODE true

const uint16_t kIrLedPin = 4;
const uint16_t kIrLedPin2 = 5;
const uint16_t kIrRecvPin = 14;
IRsend irsend(kIrLedPin);
IRsend irsend2(kIrLedPin2);
IRrecv irrecv(kIrRecvPin);
decode_results irResults;
const uint16_t kFrequency = 38;

// --- Koppel AC RAW Data (ON/OFF/TEMP_UP/TEMP_DOWN) ---
// Array sizes are NOT hardcoded - lengths are computed automatically below
// with sizeof(), so pasting in new raw codes here never mismatches the
// length used when sending them.
const uint16_t PROGMEM rawOn[] = {4850, 2500, 382, 428, 358, 406, 378, 424, 362, 956, 358, 924, 380, 426, 360, 404, 406, 388, 384, 422, 360, 946, 384, 402, 356, 442, 356, 426, 358, 948, 358, 404, 382, 404, 392, 424, 360, 426, 360, 426, 384, 930, 384, 400, 362, 944, 358, 426, 360, 416, 380, 948, 358, 428, 356, 428, 360, 436, 358, 424, 360, 924, 406, 400, 362, 416, 380, 426, 360, 424, 360, 428, 358, 438, 356, 430, 358, 426, 358, 406, 404, 414, 384, 400, 360, 426, 360, 424, 358, 438, 360, 426, 360, 428, 358, 402, 382, 436, 362, 424, 382, 898, 382, 426, 360, 436, 360, 426, 360, 924, 382, 390, 400, 428, 358, 924, 406, 404, 358, 404, 380, 956, 358, 428, 360, 402, 382, 428, 358, 436, 360, 424, 360, 426, 384, 404, 358, 436, 360, 940, 364, 924, 380, 946, 358, 428, 358, 21474, 4852};
const uint16_t PROGMEM rawOff[] = {4908, 2466, 416, 334, 452, 372, 412, 372, 414, 924, 392, 914, 390, 372, 414, 372, 414, 404, 392, 394, 390, 914, 390, 298, 488, 404, 392, 378, 408, 914, 390, 394, 390, 406, 390, 396, 390, 372, 414, 300, 486, 404, 390, 914, 390, 914, 390, 370, 416, 404, 390, 914, 392, 394, 390, 394, 392, 404, 392, 394, 390, 914, 390, 394, 392, 404, 392, 394, 390, 396, 390, 396, 390, 406, 390, 396, 390, 396, 390, 396, 390, 406, 392, 396, 390, 396, 390, 396, 390, 406, 390, 396, 390, 396, 390, 396, 390, 406, 390, 394, 392, 914, 390, 396, 370, 426, 388, 396, 366, 938, 390, 396, 390, 384, 390, 936, 390, 372, 412, 372, 412, 926, 390, 372, 414, 370, 414, 370, 416, 380, 416, 370, 414, 370, 416, 370, 416, 380, 416, 370, 416, 370, 416, 370, 416, 370, 416, 21416, 4908};
const uint16_t PROGMEM rawTempUp[] = {4908, 2466, 414, 370, 414, 370, 418, 368, 414, 924, 390, 916, 390, 370, 416, 370, 416, 380, 416, 370, 416, 916, 390, 372, 414, 380, 414, 368, 416, 914, 390, 370, 416, 380, 414, 914, 392, 376, 412, 912, 390, 382, 414, 336, 452, 368, 414, 914, 392, 378, 416, 916, 390, 372, 414, 370, 416, 382, 414, 368, 416, 914, 390, 370, 416, 380, 414, 372, 414, 370, 416, 324, 462, 334, 460, 372, 418, 368, 416, 368, 416, 382, 414, 370, 416, 372, 414, 370, 416, 382, 414, 370, 416, 324, 460, 370, 416, 406, 390, 370, 416, 918, 388, 370, 416, 366, 428, 370, 414, 916, 390, 372, 414, 404, 390, 916, 390, 370, 414, 372, 414, 384, 412, 372, 414, 370, 414, 372, 414, 382, 414, 372, 414, 372, 414, 370, 414, 406, 390, 372, 412, 914, 390, 914, 392, 916, 390, 21420, 4908};
const uint16_t PROGMEM rawTempDown[] = {4908, 2468, 414, 378, 406, 378, 406, 394, 390, 928, 388, 916, 392, 374, 408, 396, 388, 350, 428, 416, 386, 916, 390, 376, 386, 430, 366, 420, 390, 914, 390, 330, 456, 406, 392, 912, 394, 910, 390, 918, 362, 430, 390, 376, 410, 392, 368, 938, 390, 404, 368, 940, 390, 378, 406, 396, 370, 432, 384, 394, 368, 938, 368, 418, 368, 430, 388, 382, 404, 398, 388, 396, 390, 406, 368, 418, 366, 420, 366, 418, 368, 430, 366, 418, 366, 418, 368, 420, 366, 430, 368, 416, 368, 418, 390, 396, 368, 426, 368, 938, 366, 420, 366, 418, 368, 428, 390, 396, 390, 916, 392, 392, 368, 428, 368, 934, 392, 396, 390, 394, 390, 408, 390, 396, 390, 370, 414, 370, 414, 382, 414, 370, 416, 372, 412, 372, 416, 380, 414, 890, 418, 886, 414, 890, 414, 890, 416, 21422, 4906};
const int rawOnLen = sizeof(rawOn) / sizeof(rawOn[0]);
const int rawOffLen = sizeof(rawOff) / sizeof(rawOff[0]);
const int rawTempUpLen = sizeof(rawTempUp) / sizeof(rawTempUp[0]);
const int rawTempDownLen = sizeof(rawTempDown) / sizeof(rawTempDown[0]);

// --- Dynamic IR Code Library ---
// The codes actually transmitted live in these RAM buffers, not the
// compiled-in arrays above directly. They're initialized from those
// compiled-in codes, then refreshed from Firebase at
// /irRemoteLibrary/{ROOM_ID}/{command}/rawData (the Remote Library page) on
// boot and periodically after that. This means updating or replacing an
// AC's IR codes via the Remote Library no longer requires reflashing this
// device - only a brand new physical room still needs an initial flash.
// If Firebase has nothing (offline, or never captured), the last codes
// cached in flash (Preferences) are used; if there's no cache either, the
// compiled-in codes above are the final fallback, so this device is never
// left with zero usable codes.
#define MAX_IR_CODE_LEN 400
uint16_t irCodeOn[MAX_IR_CODE_LEN];
uint16_t irCodeOff[MAX_IR_CODE_LEN];
uint16_t irCodeTempUp[MAX_IR_CODE_LEN];
uint16_t irCodeTempDown[MAX_IR_CODE_LEN];
size_t irCodeOnLen = 0;
size_t irCodeOffLen = 0;
size_t irCodeTempUpLen = 0;
size_t irCodeTempDownLen = 0;
const unsigned long IR_LIBRARY_REFRESH_MS = 900000; // 15 minutes
unsigned long lastIRLibraryRefreshMillis = 0;

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

Preferences preferences;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 26
#define DHTTYPE DHT22
#define LED_PIN 27
#define RELAY_PIN 25
DHT dht(DHTPIN, DHTTYPE);

float minTemp = 20.0;
float maxTemp = 30.0;
unsigned long sendDataPrevMillis = 0;

// --- Cost/energy tracking for this room's AC unit(s) ---
// Measured via clamp meter for this specific Koppel unit (see git history
// "Ampere reading revision for calculation costs"): 16C -> ~3795W (16.5A,
// near-full compressor load), 30C -> ~253W (1.1A, compressor mostly cycled
// off). Mirrors AirconCostCalculator.getEffectiveWatts() in
// aircon-cost-calculator.js so firmware-accumulated cost lines up with
// what the analytics page computes.
const float COST_CAL_LOW_TEMP = 16.0;
const float COST_CAL_LOW_WATTS = 3795.0;
const float COST_CAL_HIGH_TEMP = 30.0;
const float COST_CAL_HIGH_WATTS = 253.0;

// Running daily energy total (kWh), accumulated incrementally at whatever
// target temp was active at each check-in - so a temp change partway
// through the day doesn't erase what was already used at the previous
// temp, unlike recomputing cost from one averaged temp after the fact.
unsigned long lastEnergyAccumMillis = 0;
String lastEnergyDateStr = "";
float dailyEnergyKwhUnit1 = 0.0;
#if DUAL_UNIT_MODE
float dailyEnergyKwhUnit2 = 0.0;
#endif

// Effective wattage for a given target temp, linearly interpolated
// between the two measured calibration points above, clamped at the ends.
float getEffectiveWattsForTemp(float targetTemp) {
  if (targetTemp <= COST_CAL_LOW_TEMP) return COST_CAL_LOW_WATTS;
  if (targetTemp >= COST_CAL_HIGH_TEMP) return COST_CAL_HIGH_WATTS;
  float frac = (targetTemp - COST_CAL_LOW_TEMP) / (COST_CAL_HIGH_TEMP - COST_CAL_LOW_TEMP);
  return COST_CAL_LOW_WATTS + (COST_CAL_HIGH_WATTS - COST_CAL_LOW_WATTS) * frac;
}

// Average power draw in kW for a unit currently targeting targetTemp.
// The calibration curve already represents real average draw, so unlike
// the rated-watts fallback this is not additionally scaled by duty cycle.
float getUnitPowerKw(float targetTemp) {
  return getEffectiveWattsForTemp(targetTemp) / 1000.0;
}

// A handful of retries with short delays for the one-time boot-time
// energy-resume read below - this runs right after WiFi/Firebase just
// connected, which is exactly when a transient read failure is most
// likely, and it's the one read this device most needs to succeed (a
// failure here silently re-triggers the "reboot erases today's cost"
// bug the resume logic exists to prevent).
bool getFloatWithRetry(const String &path, float &outValue, int maxAttempts = 3) {
  for (int attempt = 1; attempt <= maxAttempts; attempt++) {
    if (Firebase.RTDB.getFloat(&fbdo, path)) {
      outValue = fbdo.floatData();
      return true;
    }
    if (attempt < maxAttempts) {
      Serial.println("[Setup] Read failed for " + path + ", retrying (" + String(attempt) + "/" + String(maxAttempts) + ")...");
      delay(1000);
    }
  }
  return false;
}

bool automationEnabled = false;  // Controls humidity-based automation
bool temperatureAutomationEnabled = true;  // Temperature safety is always active (see Settings UI)
unsigned long automationStartTime = 0;  // Timestamp when automation was last enabled
String lastAutomationEvent = "";  // Description of last automation action
String lastAutomationEventType = "";  // Type: "humidity", "temperature", "power"
unsigned long lastAutomationEventTime = 0;  // Timestamp of last automation action
float lastAutomationEventPastTemp = 0.0;  // Unit 1 (or the only unit) target temp before automation
float lastAutomationEventUpdatedTemp = 0.0;  // Unit 1 (or the only unit) target temp after automation
#if DUAL_UNIT_MODE
float lastAutomationEventPastTemp2 = 0.0;  // Unit 2 target temp before automation - previously only Unit 1 was ever logged, even when Unit 2's setpoint also changed
float lastAutomationEventUpdatedTemp2 = 0.0;  // Unit 2 target temp after automation
#endif

// Online duration tracking variables
unsigned long onlineDurationSeconds = 0;  // Cumulative online duration in seconds
unsigned long onlineDurationSecondsUnit1 = 0;  // Cumulative online duration for unit 1
unsigned long onlineDurationSecondsUnit2 = 0;  // Cumulative online duration for unit 2
bool wasACOn = false;  // Previous AC state for tracking
unsigned long lastOnlineCheckMillis = 0;  // Last time we checked online duration

float MAX_HUMIDITY = 60.0;  // Default, will be updated from Firebase
float MIN_HUMIDITY = 45.0;  // Default, will be updated from Firebase
float humidityOccupiedThreshold = 60.0;  // From Firebase: automation/humidityOccupiedThreshold
float humidityEmptyThreshold = 45.0;     // From Firebase: automation/humidityEmptyThreshold
const unsigned long AUTOMATION_INTERVAL_MS = 900000; // 15 minutes between automation checks
const unsigned long STEP_PRESS_DELAY_MS = 400;
const unsigned long IR_SEND_COOLDOWN_MS = 60000; // 1 minute cooldown between IR sends
const unsigned long SERIAL_LOG_THROTTLE_MS = 5000; // 5 seconds between serial logs

unsigned long lastTempAutomationCheckMillis = 0;
unsigned long lastHumidityAutomationCheckMillis = 0;
unsigned long lastIRSendMillis = 0;
unsigned long lastSerialLogMillis = 0;

// Schedule Efficiency Tracking Variables - REMOVED (AC Power Automation disabled)

void handleACCommand(String cmd, int unit = 0, bool isAutomation = false);
void logAutomationEventToHistory();
void updateOLED(float currentTemp);
void runAutomation(float temp, float humidity);
void handleIRReceiver();
void showBootStatus(const char* line1, const char* line2 = "");
void loadDefaultIRCodes();
void loadIRCodesFromPreferences();
void saveIRCodesToPreferences();
void loadIRCodesFromLibrary(bool cacheOnSuccess);
bool loadIRCodeArrayFromJson(FirebaseJson &json, const char* path, uint16_t* outBuf, size_t &outLen);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== PCC S.T.A.T.S. System Starting ===");

  preferences.begin("pcc-automation", false);
  automationEnabled = preferences.getBool("autoEnabled", false);
  Serial.println("Loaded automation enabled state from Preferences: " + String(automationEnabled));

  // Start with the compiled-in codes, then overlay whatever was last cached
  // from Firebase - guarantees usable IR codes even before WiFi connects.
  loadDefaultIRCodes();
  loadIRCodesFromPreferences();

  Wire.begin(OLED_SDA, OLED_SCL);
  irsend.begin();
  irsend2.begin();
  irrecv.enableIRIn();
  Serial.println("[IR Receiver] IR receiver enabled on pin " + String(kIrRecvPin));
  Serial.println("[IR Sender] IR senders initialized - Unit 0 on pin " + String(kIrLedPin) + ", Unit 1 on pin " + String(kIrLedPin2));

  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  showBootStatus("Connecting WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // A true cold power-on can occasionally fail to associate on the very
  // first attempt (radio calibration timing, power-rail ramp-up) and then
  // just sit here forever with no retry - which is exactly why pressing
  // EN (forcing a fresh setup() once power has already stabilized) was
  // needed to get it online. Time out and restart instead of waiting
  // forever, so it recovers on its own.
  unsigned long wifiConnectStartMillis = millis();
  const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - wifiConnectStartMillis > WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println("[Setup] WiFi did not connect within 20s, restarting...");
      showBootStatus("WiFi failed", "Restarting...");
      delay(500);
      ESP.restart();
    }
  }
  Serial.println("WiFi connected");

  // Wireless (OTA) reflashing - once this is running, Arduino IDE shows this
  // device as a network port ("esp32-Room_X at <ip>") instead of needing a
  // USB cable for future uploads. USB still works as a fallback; this is
  // additive, not a replacement.
  String otaHostname = "esp32-" + String(ROOM_ID);
  ArduinoOTA.setHostname(otaHostname.c_str());
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("[OTA] Update starting: " + type);
    showBootStatus("OTA Update...", "Do not power off");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Update complete, rebooting...");
    showBootStatus("Update complete", "Rebooting...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed");
    else if (error == OTA_END_ERROR) Serial.println("End failed");
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] Ready - hostname: " + otaHostname);

  // Configure NTP time
  showBootStatus("WiFi connected", "Syncing time...");
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time sync...");
  
  // Wait for time to be set (with timeout)
  int timeout = 0;
  while (time(nullptr) < 1000000000 && timeout < 20) {
    Serial.print(".");
    delay(1000);
    timeout++;
  }
  Serial.println();
  
  time_t now = time(nullptr);
  Serial.print("Current time (epoch): ");
  Serial.println(now);
  
  if (now < 1000000000) {
    Serial.println("WARNING: NTP time sync failed! Time may be incorrect.");
  } else {
    struct tm* timeinfo = localtime(&now);
    Serial.print("NTP time synced: ");
    Serial.print(asctime(timeinfo));
  }

  showBootStatus("Time synced", "Connecting Firebase...");
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Setting up Firebase stream for: /" + String(ROOM_ID));
  if (!Firebase.RTDB.beginStream(&stream, "/" + String(ROOM_ID))) {
    Serial.println("Firebase stream failed: " + stream.errorReason());
  } else {
    Serial.println("Firebase stream started successfully");
    Serial.println("Stream path: /" + String(ROOM_ID));
  }

  // Load all room config (automation flags/thresholds + online duration) in a
  // single request instead of 8 separate sequential GET calls - each one was
  // a full network round trip, and they were the biggest contributor to the
  // long delay between "WiFi connected" and the OLED showing anything useful.
  showBootStatus("Firebase ready", "Loading config...");
  Serial.println("[Setup] Loading room configuration in a single request...");
  if (Firebase.RTDB.getJSON(&fbdo, "/" + String(ROOM_ID))) {
    FirebaseJson &roomJson = fbdo.jsonObject();
    FirebaseJsonData jsonData;

    if (roomJson.get(jsonData, "automation/enabled")) {
      automationEnabled = jsonData.boolValue;
      preferences.putBool("autoEnabled", automationEnabled);
      Serial.println("Synced automation enabled state from Firebase: " + String(automationEnabled));
      Serial.println(automationEnabled ? "✅ [Setup] Humidity automation is ENABLED" : "❌ [Setup] Humidity automation is DISABLED");
    } else {
      Serial.println("⚠️ [Setup] No automation/enabled in Firebase, using default: " + String(automationEnabled));
    }

    if (roomJson.get(jsonData, "automation/humidityOccupiedThreshold")) {
      float threshold = jsonData.floatValue;
      if (threshold > 0) {
        humidityOccupiedThreshold = threshold;
        MAX_HUMIDITY = threshold;
        Serial.println("Synced humidity occupied threshold from Firebase: " + String(humidityOccupiedThreshold) + "%");
      }
    } else {
      Serial.println("⚠️ [Setup] No humidity occupied threshold in Firebase, using default: " + String(MAX_HUMIDITY) + "%");
    }

    if (roomJson.get(jsonData, "automation/humidityEmptyThreshold")) {
      float threshold = jsonData.floatValue;
      if (threshold > 0) {
        humidityEmptyThreshold = threshold;
        MIN_HUMIDITY = threshold;
        Serial.println("Synced humidity empty threshold from Firebase: " + String(humidityEmptyThreshold) + "%");
      }
    } else {
      Serial.println("⚠️ [Setup] No humidity empty threshold in Firebase, using default: " + String(MIN_HUMIDITY) + "%");
    }

    if (roomJson.get(jsonData, "thresholds/minTemp")) {
      float temp = jsonData.floatValue;
      if (temp > 0) {
        minTemp = temp;
        Serial.println("Synced minTemp from Firebase: " + String(minTemp) + "°C");
      }
    }

    if (roomJson.get(jsonData, "thresholds/maxTemp")) {
      float temp = jsonData.floatValue;
      if (temp > 0) {
        maxTemp = temp;
        Serial.println("Synced maxTemp from Firebase: " + String(maxTemp) + "°C");
      }
    }

    if (roomJson.get(jsonData, "online_duration")) {
      onlineDurationSeconds = jsonData.intValue;
      Serial.println("[Setup] Loaded online_duration from Firebase: " + String(onlineDurationSeconds) + " seconds");
    } else {
      Serial.println("[Setup] No online_duration found in Firebase, starting at 0");
    }

    #if DUAL_UNIT_MODE
    if (roomJson.get(jsonData, "units/unit_1/online_duration")) {
      onlineDurationSecondsUnit1 = jsonData.intValue;
      Serial.println("[Setup] Loaded online_duration for unit 1 from Firebase: " + String(onlineDurationSecondsUnit1) + " seconds");
    } else {
      Serial.println("[Setup] No online_duration found for unit 1 in Firebase, starting at 0");
    }

    if (roomJson.get(jsonData, "units/unit_2/online_duration")) {
      onlineDurationSecondsUnit2 = jsonData.intValue;
      Serial.println("[Setup] Loaded online_duration for unit 2 from Firebase: " + String(onlineDurationSecondsUnit2) + " seconds");
    } else {
      Serial.println("[Setup] No online_duration found for unit 2 in Firebase, starting at 0");
    }
    #endif
  } else {
    Serial.println("⚠️ [Setup] Failed to load room configuration from Firebase: " + fbdo.errorReason());
    Serial.println("[Setup] Using default thresholds and automation settings");
  }

  // Resume today's energy/cost accumulator from Firebase instead of
  // restarting at 0 on every boot - otherwise a reboot (WiFi drop,
  // brownout, this device's own reconnect watchdog, an OTA reflash)
  // silently erases however much of the day's cost was already tracked,
  // which is exactly what was seen tonight.
  {
    time_t nowEnergyInit = time(nullptr);
    struct tm* energyInitTimeinfo = localtime(&nowEnergyInit);
    char energyInitDateStr[11];
    strftime(energyInitDateStr, sizeof(energyInitDateStr), "%Y-%m-%d", energyInitTimeinfo);
    lastEnergyDateStr = String(energyInitDateStr);

    String energyBasePath = "/analytics/dailyAnalytics/" + lastEnergyDateStr + "/" + String(ROOM_ID);
    #if DUAL_UNIT_MODE
    if (getFloatWithRetry(energyBasePath + "_unit_1/energyKwh", dailyEnergyKwhUnit1)) {
      Serial.println("[Setup] Resumed unit_1 energyKwh from Firebase: " + String(dailyEnergyKwhUnit1, 3) + " kWh");
    } else {
      Serial.println("[Setup] No unit_1 energyKwh found for today (or read kept failing), starting at 0");
    }
    if (getFloatWithRetry(energyBasePath + "_unit_2/energyKwh", dailyEnergyKwhUnit2)) {
      Serial.println("[Setup] Resumed unit_2 energyKwh from Firebase: " + String(dailyEnergyKwhUnit2, 3) + " kWh");
    } else {
      Serial.println("[Setup] No unit_2 energyKwh found for today (or read kept failing), starting at 0");
    }
    #else
    if (getFloatWithRetry(energyBasePath + "/energyKwh", dailyEnergyKwhUnit1)) {
      Serial.println("[Setup] Resumed energyKwh from Firebase: " + String(dailyEnergyKwhUnit1, 3) + " kWh");
    } else {
      Serial.println("[Setup] No energyKwh found for today (or read kept failing), starting at 0");
    }
    #endif
  }

  // Schedule configuration loading REMOVED (AC Power Automation disabled)

  showBootStatus("Config loaded", "Loading IR codes...");
  loadIRCodesFromLibrary(true);

  Serial.println("[Setup] Temperature Safety Automation: ENABLED (always active)");
  Serial.println("[Setup] Automation Interval: " + String(AUTOMATION_INTERVAL_MS / 1000) + " seconds");
  Serial.println("[Setup] IR Send Cooldown: " + String(IR_SEND_COOLDOWN_MS / 1000) + " seconds");

  FirebaseJson json;
  json.set(".sv", "timestamp");
  Firebase.RTDB.setJSON(&fbdo, "/" + String(ROOM_ID) + "/last_seen", &json);
  Firebase.RTDB.setBool(&fbdo, "/" + String(ROOM_ID) + "/online", true);
  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/device_room_id", ROOM_ID);

  #if DUAL_UNIT_MODE
  // Firebase is authoritative for AC state - do NOT force it to false here.
  // Forcing it off on every boot silently turned the dashboard power switch
  // off (with no IR sent) whenever the ESP32 rebooted (WiFi drop, brownout,
  // watchdog reset, etc.), which looked like the switch randomly turning off.
  // Just clear any stale in-flight command so it isn't re-applied on boot.
  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/ac_command", "IDLE");
  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/ac_command", "IDLE");
  Serial.println("[Setup] Preserving existing AC state from Firebase (not forcing off)");
  #endif

  showBootStatus("Setup complete", "Starting...");
  Serial.println("=== Setup complete ===");
}

void loop() {
  // Serviced first, every iteration, so a pending OTA upload gets picked up
  // promptly rather than waiting behind sensor reads/Firebase calls. Note:
  // the long delay(20000) steps inside the humidity automation below (and
  // similar blocking waits) still block this from running for their
  // duration - if an OTA upload happens to start during one of those, it
  // will likely time out. Not expected to matter in practice since uploads
  // are infrequent and short, but worth knowing if an upload ever stalls.
  ArduinoOTA.handle();

  // Explicit WiFi reconnect watchdog. Firebase.reconnectWiFi(true) (set in
  // setup()) is supposed to auto-recover WiFi during Firebase calls, but
  // it can get stuck if the AP itself drops the connection - the rest of
  // this loop (sensor reads, OLED updates) keeps running fine either way,
  // which is exactly why last_seen can go stale for hours while the
  // device looks alive: nothing else here ever retries WiFi.begin().
  static unsigned long lastWifiCheckMillis = 0;
  static unsigned long wifiDisconnectedSinceMillis = 0;
  if (millis() - lastWifiCheckMillis > 10000) {
    lastWifiCheckMillis = millis();
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDisconnectedSinceMillis == 0) {
        wifiDisconnectedSinceMillis = millis();
        Serial.println("⚠️ [WiFi Watchdog] Connection lost, will attempt to reconnect");
      }
      unsigned long disconnectedFor = millis() - wifiDisconnectedSinceMillis;
      showBootStatus("WiFi lost", "Reconnecting...");
      if (disconnectedFor < 120000) {
        // First 2 minutes: try the lighter-weight reconnect first
        Serial.println("[WiFi Watchdog] Attempting WiFi.reconnect()");
        WiFi.reconnect();
      } else {
        // Still down after 2 minutes: reset the WiFi driver state and
        // start fresh, in case it's stuck rather than just slow
        Serial.println("[WiFi Watchdog] Still down after 2 min, restarting WiFi.begin()");
        WiFi.disconnect();
        delay(100);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
    } else if (wifiDisconnectedSinceMillis != 0) {
      Serial.println("✅ [WiFi Watchdog] Reconnected after " + String((millis() - wifiDisconnectedSinceMillis) / 1000) + "s");
      wifiDisconnectedSinceMillis = 0;
    }
  }

  static unsigned long lastLoopLog = 0;
  if (millis() - lastLoopLog > 60000) {
    Serial.println("[Loop] ESP32 is running, millis: " + String(millis()));
    Serial.println("[Loop] Firebase ready: " + String(Firebase.ready() ? "YES" : "NO"));
    lastLoopLog = millis();
  }

    if (Firebase.ready() && Firebase.RTDB.readStream(&stream)) {
    if (stream.streamTimeout()) {
      return;
    }
    if (stream.dataType() == "null") {
      return;
    }
    
    String path = stream.dataPath();
    
    // Log important paths, ignore system paths that update every second/10s
    // and just spam the log with no useful information (last_seen,
    // ac_command, and online_duration - the latter written every second by
    // this same device's own online-duration tracking below).
    if (path != "/last_seen" && path.indexOf("last_seen") < 0 && !path.endsWith("ac_command") && path.indexOf("online_duration") < 0) {
      Serial.println("[Firebase] Path: " + path + ", Type: " + stream.dataType());
    }
    
    // Handle root path updates (entire room object)
    if (path == "/" && stream.dataType() == "json") {
      FirebaseJson &json = stream.jsonObject();
      FirebaseJsonData jsonData;
      if (json.get(jsonData, "ac_command")) {
        String cmd = jsonData.stringValue;
        Serial.println("[Firebase] Found ac_command in root object: " + cmd);
        if (cmd != "IDLE") {
          #if DUAL_UNIT_MODE
          Serial.println("[Firebase] Dual unit mode - sending to both units");
          handleACCommand(cmd, 1, false); // Manual command
          handleACCommand(cmd, 2, false); // Manual command
          #else
          Serial.println("[Firebase] Single unit mode - sending to unit 0");
          handleACCommand(cmd, 0, false); // Manual command
          #endif
        } else {
          Serial.println("[Firebase] Ignoring IDLE command");
        }
      }
    }
    
    if (path == "/thresholds/minTemp" || path == "/minTemp") {
      minTemp = stream.floatData();
      Serial.println("[Firebase] Updated minTemp: " + String(minTemp));
    } else if (path == "/thresholds/maxTemp" || path == "/maxTemp") {
      maxTemp = stream.floatData();
      Serial.println("[Firebase] Updated maxTemp: " + String(maxTemp));
    } else if (path == "/automation/enabled" || path.indexOf("automation/enabled") >= 0) {
      Serial.println("[Firebase Stream] automation/enabled path detected");
      bool newEnabledState = stream.boolData();
      Serial.println("[Firebase Stream] New enabled state: " + String(newEnabledState) + ", Current state: " + String(automationEnabled));
      if (newEnabledState != automationEnabled) {
        automationEnabled = newEnabledState;
        preferences.putBool("autoEnabled", automationEnabled);
        if (automationEnabled) {
          automationStartTime = millis();
          Serial.println("✅ [Firebase Stream] Humidity automation ENABLED at " + String(automationStartTime / 1000) + " seconds");
          // Log start time to Firebase for real-time display
          FirebaseJson json;
          json.set(".sv", "timestamp");
          Firebase.RTDB.setJSON(&fbdo, "/" + String(ROOM_ID) + "/automation/startTime", &json);
        } else {
          Serial.println("❌ [Firebase Stream] Humidity automation DISABLED");
          // Clear start time when disabled
          Firebase.RTDB.set(&fbdo, "/" + String(ROOM_ID) + "/automation/startTime", nullptr);
        }
      } else {
        Serial.println("[Firebase Stream] Automation state unchanged, no action needed");
      }
    } else if (path == "/automation" || path.indexOf("automation") >= 0) {
      FirebaseJson &json = stream.jsonObject();
      FirebaseJsonData jsonData;
      if (json.get(jsonData, "enabled")) {
        bool newEnabledState = jsonData.boolValue;
        if (newEnabledState != automationEnabled) {
          automationEnabled = newEnabledState;
          preferences.putBool("autoEnabled", automationEnabled);
          if (automationEnabled) {
            automationStartTime = millis();
            Serial.println("Humidity automation ENABLED at " + String(automationStartTime / 1000) + " seconds");
            // Log start time to Firebase for real-time display
            FirebaseJson startTimeJson;
            startTimeJson.set(".sv", "timestamp");
            Firebase.RTDB.setJSON(&fbdo, "/" + String(ROOM_ID) + "/automation/startTime", &startTimeJson);
          } else {
            Serial.println("Humidity automation DISABLED");
            // Clear start time when disabled
            Firebase.RTDB.set(&fbdo, "/" + String(ROOM_ID) + "/automation/startTime", nullptr);
          }
        }
      }
      if (json.get(jsonData, "humidityOccupiedThreshold")) {
        float newThreshold = jsonData.floatValue;
        if (newThreshold > 0 && newThreshold != humidityOccupiedThreshold) {
          humidityOccupiedThreshold = newThreshold;
          MAX_HUMIDITY = newThreshold;
          Serial.println("Humidity occupied threshold updated: " + String(humidityOccupiedThreshold) + "%");
        }
      }
      if (json.get(jsonData, "humidityEmptyThreshold")) {
        float newThreshold = jsonData.floatValue;
        if (newThreshold > 0 && newThreshold != humidityEmptyThreshold) {
          humidityEmptyThreshold = newThreshold;
          MIN_HUMIDITY = newThreshold;
          Serial.println("Humidity empty threshold updated: " + String(humidityEmptyThreshold) + "%");
        }
      }
    } else if (path == "/automation/humidityOccupiedThreshold") {
      float newThreshold = stream.floatData();
      if (newThreshold > 0 && newThreshold != humidityOccupiedThreshold) {
        humidityOccupiedThreshold = newThreshold;
        MAX_HUMIDITY = newThreshold;
        Serial.println("Humidity occupied threshold updated: " + String(humidityOccupiedThreshold) + "%");
      }
    } else if (path == "/automation/humidityEmptyThreshold") {
      float newThreshold = stream.floatData();
      if (newThreshold > 0 && newThreshold != humidityEmptyThreshold) {
        humidityEmptyThreshold = newThreshold;
        MIN_HUMIDITY = newThreshold;
        Serial.println("Humidity empty threshold updated: " + String(humidityEmptyThreshold) + "%");
      }
    } else if (path == "/ac_command" || path.endsWith("ac_command")) {
      String cmd = stream.stringData();
      if (cmd == "IDLE") {
        // Silently ignore IDLE commands without logging
        return;
      }
      
      Serial.print("[Firebase] ac_command received - Path: "); Serial.print(path);
      Serial.print(", Command: "); Serial.println(cmd);
      
      // Only process if this is NOT a unit-specific path (those are handled below)
      if (path.indexOf("/units/unit_1/") >= 0 || path.indexOf("/units/unit_2/") >= 0) {
        Serial.println("[Firebase] Skipping - unit-specific path handled by object handler");
        return;
      }

      Serial.println("[Firebase] Routing to Single Unit (unit 0)");
      handleACCommand(cmd, 0, false); // Manual command
    } else if (path == "/units/unit_1" || path == "/units/unit_1/") {
      FirebaseJson &json = stream.jsonObject();
      FirebaseJsonData jsonData;
      if (json.get(jsonData, "ac_command")) {
        String cmd = jsonData.stringValue;
        if (cmd != "IDLE") {
          Serial.println("[Firebase] Found ac_command in Unit 1: " + cmd);
          handleACCommand(cmd, 1, false); // Manual command
        }
      }
    } else if (path == "/units/unit_2" || path == "/units/unit_2/") {
      Serial.println("[Firebase] Unit 2 object updated");
      FirebaseJson &json = stream.jsonObject();
      FirebaseJsonData jsonData;
      if (json.get(jsonData, "ac_command")) {
        String cmd = jsonData.stringValue;
        Serial.println("[Firebase] Found ac_command in Unit 2: " + cmd);
        if (cmd != "IDLE") {
          handleACCommand(cmd, 2, false); // Manual command
        } else {
          Serial.println("[Firebase] Ignoring IDLE command");
        }
      }
    } else if (path == "/units/unit_1/targetTemp" || path.indexOf("/units/unit_1/targetTemp") >= 0) {
      Serial.println("[Firebase] Unit 1 targetTemp updated: " + String(stream.floatData()) + "°C");
      // Unit target temp update is handled by the ac_command sent separately
    } else if (path == "/units/unit_2/targetTemp" || path.indexOf("/units/unit_2/targetTemp") >= 0) {
      Serial.println("[Firebase] Unit 2 targetTemp updated: " + String(stream.floatData()) + "°C");
      // Unit target temp update is handled by the ac_command sent separately
    } else if (path == "/targetTemp" || path.indexOf("targetTemp") >= 0) {
      Serial.println("[Firebase] targetTemp updated: " + String(stream.floatData()) + "°C");
      // Target temp update is handled by the ac_command sent separately
    }
  }

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Sensor validation - filter out unrealistic readings
  static float lastValidTemp = 25.0; // Initialize with reasonable default
  static float lastValidHum = 60.0;
  
  if (!isnan(t) && !isnan(h)) {
    // Validate temperature range (DHT22 typically reads -40 to 80°C, but realistic indoor range is 15-35°C)
    if (t < -10 || t > 50 || h < 0 || h > 100) {
      Serial.print("[Sensor] Invalid reading filtered - Temp: "); Serial.print(t, 1);
      Serial.print("°C, Humidity: "); Serial.print(h, 1); Serial.println("%");
      // Use last valid values
      t = lastValidTemp;
      h = lastValidHum;
    } else {
      // Update last valid values
      lastValidTemp = t;
      lastValidHum = h;
    }
  } else {
    // Sensor read failed, use last valid values
    t = lastValidTemp;
    h = lastValidHum;
  }

  static unsigned long lastSensorLogMillis = 0;
  if (!isnan(t) && !isnan(h) && millis() - lastSensorLogMillis > 5000) {
    Serial.print("Live - Temp: "); Serial.print(t, 1); Serial.print("°C, Humidity: "); Serial.print(h, 1); Serial.println("%");
    lastSensorLogMillis = millis();
  }

  if (!isnan(t)) {
    bool isAlarm = (t < minTemp || t > maxTemp);
    digitalWrite(LED_PIN, isAlarm ? HIGH : LOW);
    digitalWrite(RELAY_PIN, (t < minTemp) ? LOW : HIGH);
  }

  updateOLED(t);

  handleIRReceiver();

  // Pick up any IR code changes saved to the Remote Library without needing
  // a reflash - a plain periodic re-fetch rather than a second permanent
  // Firebase stream, since code changes are rare and this is simpler and
  // more robust. A saved change takes up to IR_LIBRARY_REFRESH_MS to apply.
  if (millis() - lastIRLibraryRefreshMillis > IR_LIBRARY_REFRESH_MS) {
    lastIRLibraryRefreshMillis = millis();
    loadIRCodesFromLibrary(true);
  }

  // Schedule adherence check REMOVED (AC Power Automation disabled)

  if (!isnan(t) && !isnan(h)) {
    runAutomation(t, h);
  }

  // Track online duration - check every second
  if (millis() - lastOnlineCheckMillis >= 1000) {
    lastOnlineCheckMillis = millis();
    
    #if DUAL_UNIT_MODE
    bool isUnit1On = false;
    bool isUnit2On = false;
    Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/ac");
    if (fbdo.dataType() == "boolean") {
      isUnit1On = fbdo.boolData();
    }
    Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/ac");
    if (fbdo.dataType() == "boolean") {
      isUnit2On = fbdo.boolData();
    }
    
    if (isUnit1On) {
      onlineDurationSecondsUnit1++;
      Firebase.RTDB.setInt(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/online_duration", onlineDurationSecondsUnit1);
    }
    if (isUnit2On) {
      onlineDurationSecondsUnit2++;
      Firebase.RTDB.setInt(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/online_duration", onlineDurationSecondsUnit2);
    }
    // Total online duration is sum of both units
    onlineDurationSeconds = onlineDurationSecondsUnit1 + onlineDurationSecondsUnit2;
    Firebase.RTDB.setInt(&fbdo, "/" + String(ROOM_ID) + "/online_duration", onlineDurationSeconds);
    #else
    bool isACOn = false;
    Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/ac");
    if (fbdo.dataType() == "boolean") {
      isACOn = fbdo.boolData();
    }
    
    if (isACOn) {
      onlineDurationSeconds++;
      Firebase.RTDB.setInt(&fbdo, "/" + String(ROOM_ID) + "/online_duration", onlineDurationSeconds);
    }
    #endif
  }

  if (millis() - sendDataPrevMillis > 10000) {
    sendDataPrevMillis = millis();
    FirebaseJson updateData;
    updateData.add("temperature", t);
    updateData.add("humidity", h);
    updateData.add("status", (!isnan(t) && (t < minTemp || t > maxTemp)) ? "ALARM" : "NORMAL");
    Firebase.RTDB.updateNode(&fbdo, "/" + String(ROOM_ID), &updateData);

    FirebaseJson json;
    json.set(".sv", "timestamp");
    Firebase.RTDB.setJSON(&fbdo, "/" + String(ROOM_ID) + "/last_seen", &json);

    if (!isnan(t) && !isnan(h)) {
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
      char dateStr[11];
      char hourStr[3];
      strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);
      strftime(hourStr, sizeof(hourStr), "%H", timeinfo);

      String historyPath = "/history/" + String(ROOM_ID) + "/" + String(dateStr) + "/" + String(hourStr);

      // Read existing data first to preserve automation_events subdirectory
      Firebase.RTDB.getJSON(&fbdo, historyPath.c_str());
      FirebaseJson existingData;
      if (fbdo.jsonString() != "") {
        existingData.setJsonData(fbdo.jsonString());
      }

      FirebaseJson historyData;
      historyData.add("temperature", t);
      historyData.add("humidity", h);
      // Log targetTemp for accurate historical cost calculations
      #if DUAL_UNIT_MODE
      float targetTemp1 = 24.0; // Default fallback
      float targetTemp2 = 24.0; // Default fallback
      bool unit1AC = false;
      bool unit2AC = false;
      if (Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/targetTemp")) {
        targetTemp1 = fbdo.floatData();
      }
      if (Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/targetTemp")) {
        targetTemp2 = fbdo.floatData();
      }
      if (Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/ac")) {
        unit1AC = fbdo.boolData();
      }
      if (Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/ac")) {
        unit2AC = fbdo.boolData();
      }
      historyData.add("targetTemp1", targetTemp1);
      historyData.add("targetTemp2", targetTemp2);
      historyData.add("ac", unit1AC || unit2AC); // Log overall AC state (true if any unit is ON)
      historyData.add("unit1AC", unit1AC); // Log individual unit states
      historyData.add("unit2AC", unit2AC);
      #else
      float targetTemp = 24.0; // Default fallback
      bool acState = false;
      if (Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/targetTemp")) {
        targetTemp = fbdo.floatData();
      }
      if (Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/ac")) {
        acState = fbdo.boolData();
      }
      historyData.add("targetTemp", targetTemp);
      historyData.add("ac", acState); // Log AC state for usage hour reconstruction
      #endif

      // Incremental cost/energy accumulation: add up energy used since the
      // last check-in (at whatever target temp was active then) into a
      // running per-day total, so a temp change partway through the day
      // adds onto what was already accumulated instead of overwriting it.
      unsigned long nowMillisEnergy = millis();
      if (lastEnergyAccumMillis == 0) {
        lastEnergyAccumMillis = nowMillisEnergy; // first tick: nothing elapsed yet
      }
      String todayStr = String(dateStr);
      if (todayStr != lastEnergyDateStr) {
        dailyEnergyKwhUnit1 = 0.0;
        #if DUAL_UNIT_MODE
        dailyEnergyKwhUnit2 = 0.0;
        #endif
        lastEnergyDateStr = todayStr;
      }
      float elapsedHoursEnergy = (nowMillisEnergy - lastEnergyAccumMillis) / 3600000.0;
      lastEnergyAccumMillis = nowMillisEnergy;
      // Guard against an abnormal stall (blocked WiFi/Firebase call, long delay)
      // getting billed in one lump at whatever state happens to be active on
      // recovery. Normal ticks are ~10s; cap at 5 min so a stall only ever
      // costs a bounded amount instead of hours of wrongly-attributed usage.
      const float MAX_ENERGY_GAP_HOURS = 5.0 / 60.0;
      if (elapsedHoursEnergy > MAX_ENERGY_GAP_HOURS) {
        Serial.print("[Energy] Abnormal gap detected: "); Serial.print(elapsedHoursEnergy * 60.0, 1);
        Serial.println(" min. Capping to avoid lump-sum billing.");
        elapsedHoursEnergy = MAX_ENERGY_GAP_HOURS;
      }

      #if DUAL_UNIT_MODE
      if (unit1AC) dailyEnergyKwhUnit1 += getUnitPowerKw(targetTemp1) * elapsedHoursEnergy;
      if (unit2AC) dailyEnergyKwhUnit2 += getUnitPowerKw(targetTemp2) * elapsedHoursEnergy;
      Firebase.RTDB.setFloat(&fbdo, "/analytics/dailyAnalytics/" + todayStr + "/" + String(ROOM_ID) + "_unit_1/energyKwh", dailyEnergyKwhUnit1);
      Firebase.RTDB.setFloat(&fbdo, "/analytics/dailyAnalytics/" + todayStr + "/" + String(ROOM_ID) + "_unit_2/energyKwh", dailyEnergyKwhUnit2);
      #else
      if (acState) dailyEnergyKwhUnit1 += getUnitPowerKw(targetTemp) * elapsedHoursEnergy;
      Firebase.RTDB.setFloat(&fbdo, "/analytics/dailyAnalytics/" + todayStr + "/" + String(ROOM_ID) + "/energyKwh", dailyEnergyKwhUnit1);
      #endif
      // Log automation state and start time
      historyData.add("automationEnabled", automationEnabled);
      if (automationEnabled && automationStartTime > 0) {
        historyData.add("automationStartTime", automationStartTime / 1000); // Store as seconds since boot
      }

      // Schedule efficiency logging REMOVED (AC Power Automation disabled)

      // Preserve automation_events subdirectory if it exists
      FirebaseJsonData automationEventsData;
      if (existingData.get(automationEventsData, "automation_events")) {
        FirebaseJson automationEventsJson;
        automationEventsData.getJSON(automationEventsJson);
        historyData.add("automation_events", automationEventsJson);
        Serial.println("[History] Preserving existing automation_events subdirectory");
      }
      
      // Preserve existing automation event data if it exists
      FirebaseJsonData eventData;
      String existingEvent = "";
      String existingEventType = "";
      String existingEventTime = "";
      if (existingData.get(eventData, "automationEvent")) {
        existingEvent = eventData.to<String>();
      }
      if (existingData.get(eventData, "automationEventType")) {
        existingEventType = eventData.to<String>();
      }
      if (existingData.get(eventData, "automationEventTime")) {
        existingEventTime = eventData.to<String>();
      }
      if (existingEvent.length() > 0) {
        historyData.add("automationEvent", existingEvent);
        historyData.add("automationEventType", existingEventType);
        historyData.add("automationEventTime", existingEventTime);
      }
      
      Serial.print("[History] Writing to Firebase path: "); Serial.println(historyPath.c_str());
      bool historySuccess = Firebase.RTDB.setJSON(&fbdo, historyPath.c_str(), &historyData);
      Serial.print("[History] Firebase write: "); Serial.println(historySuccess ? "success" : "failed");
      if (!historySuccess) {
        Serial.print("[History] Error: "); Serial.println(fbdo.errorReason());
      }
    }
  }
}

// Shows a boot-stage message on the OLED so the screen isn't just blank while
// setup() works through WiFi/NTP/Firebase - previously the display showed
// nothing at all until loop() started, which looked frozen even after the
// serial monitor already confirmed WiFi was connected.
void showBootStatus(const char* line1, const char* line2) {
  display.clearDisplay();
  display.fillScreen(SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.print(line1);
  if (line2 && line2[0] != '\0') {
    display.setCursor(5, 40);
    display.print(line2);
  }
  display.display();
}

void updateOLED(float currentTemp) {
  display.clearDisplay();

  display.fillScreen(SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);

  display.setTextSize(1);
  display.setCursor(5, 5);
  display.printf("SET: %.0fC - %.0fC", minTemp, maxTemp);

  display.drawFastHLine(0, 15, 128, SSD1306_BLACK);

  display.setCursor(20, 30);
  display.setTextSize(2);
  if (isnan(currentTemp)) {
    display.print("SENSOR ERR");
  } else {
    display.print(currentTemp, 1);
    display.print(" C");
  }

  display.setTextSize(1);
  display.setCursor(5, 55);
  if (isnan(currentTemp)) {
    display.print("STATUS: NO READING");
  } else if (currentTemp < minTemp) {
    display.print("STATUS: TOO COLD");
  } else if (currentTemp > maxTemp) {
    display.print("STATUS: TOO HOT");
  } else {
    display.print("STATUS: NORMAL");
  }

  display.display();
}

// Copies the compiled-in codes into the active buffers, so there is always
// a known-good code to send even before Firebase/Preferences have anything.
void loadDefaultIRCodes() {
  memcpy_P(irCodeOn, rawOn, sizeof(rawOn));
  irCodeOnLen = rawOnLen;
  memcpy_P(irCodeOff, rawOff, sizeof(rawOff));
  irCodeOffLen = rawOffLen;
  memcpy_P(irCodeTempUp, rawTempUp, sizeof(rawTempUp));
  irCodeTempUpLen = rawTempUpLen;
  memcpy_P(irCodeTempDown, rawTempDown, sizeof(rawTempDown));
  irCodeTempDownLen = rawTempDownLen;
  Serial.println("[IR Library] Loaded compiled-in default codes");
}

void saveIRCodesToPreferences() {
  preferences.putBytes("irCodeOn", irCodeOn, irCodeOnLen * sizeof(uint16_t));
  preferences.putUInt("irCodeOnLen", irCodeOnLen);
  preferences.putBytes("irCodeOff", irCodeOff, irCodeOffLen * sizeof(uint16_t));
  preferences.putUInt("irCodeOffLen", irCodeOffLen);
  preferences.putBytes("irCodeTUp", irCodeTempUp, irCodeTempUpLen * sizeof(uint16_t));
  preferences.putUInt("irCodeTUpLen", irCodeTempUpLen);
  preferences.putBytes("irCodeTDn", irCodeTempDown, irCodeTempDownLen * sizeof(uint16_t));
  preferences.putUInt("irCodeTDnLen", irCodeTempDownLen);
  Serial.println("[IR Library] Cached current codes to flash");
}

// Overlays whatever was last successfully cached from Firebase, if any -
// used at boot before WiFi/Firebase are available yet.
void loadIRCodesFromPreferences() {
  size_t len = preferences.getUInt("irCodeOnLen", 0);
  if (len > 0 && len <= MAX_IR_CODE_LEN) {
    preferences.getBytes("irCodeOn", irCodeOn, len * sizeof(uint16_t));
    irCodeOnLen = len;
  }
  len = preferences.getUInt("irCodeOffLen", 0);
  if (len > 0 && len <= MAX_IR_CODE_LEN) {
    preferences.getBytes("irCodeOff", irCodeOff, len * sizeof(uint16_t));
    irCodeOffLen = len;
  }
  len = preferences.getUInt("irCodeTUpLen", 0);
  if (len > 0 && len <= MAX_IR_CODE_LEN) {
    preferences.getBytes("irCodeTUp", irCodeTempUp, len * sizeof(uint16_t));
    irCodeTempUpLen = len;
  }
  len = preferences.getUInt("irCodeTDnLen", 0);
  if (len > 0 && len <= MAX_IR_CODE_LEN) {
    preferences.getBytes("irCodeTDn", irCodeTempDown, len * sizeof(uint16_t));
    irCodeTempDownLen = len;
  }
  Serial.println("[IR Library] Overlaid cached codes from flash (if any)");
}

// Reads one command's rawData array out of an already-fetched library JSON
// object (e.g. "acOn/rawData") into outBuf/outLen. Returns false (leaving
// outBuf/outLen untouched) if the path is missing, empty, or too long.
bool loadIRCodeArrayFromJson(FirebaseJson &json, const char* path, uint16_t* outBuf, size_t &outLen) {
  FirebaseJsonData jsonData;
  if (!json.get(jsonData, path)) return false;

  FirebaseJsonArray arr;
  if (!jsonData.getArray(arr)) return false;

  size_t count = arr.size();
  if (count == 0 || count > MAX_IR_CODE_LEN) {
    Serial.println("[IR Library] " + String(path) + " has invalid length (" + String(count) + "), ignoring");
    return false;
  }

  FirebaseJsonData item;
  for (size_t i = 0; i < count; i++) {
    arr.get(item, i);
    outBuf[i] = (uint16_t)item.intValue;
  }
  outLen = count;
  return true;
}

// Fetches this room's 4 IR commands from the Remote Library in one request
// and overlays whatever's valid onto the active buffers. Anything missing
// or invalid just leaves the current code (Firebase/Preferences/compiled-in,
// whichever was already active) in place rather than clearing it.
void loadIRCodesFromLibrary(bool cacheOnSuccess) {
  Serial.println("[IR Library] Fetching IR codes for " + String(ROOM_ID) + " from Remote Library...");

  if (!Firebase.RTDB.getJSON(&fbdo, "/irRemoteLibrary/" + String(ROOM_ID))) {
    Serial.println("[IR Library] No library data for this room yet, keeping current codes. " + fbdo.errorReason());
    return;
  }

  FirebaseJson &libJson = fbdo.jsonObject();
  bool anyLoaded = false;

  if (loadIRCodeArrayFromJson(libJson, "acOn/rawData", irCodeOn, irCodeOnLen)) {
    Serial.println("[IR Library] Loaded acOn (" + String(irCodeOnLen) + " values)");
    anyLoaded = true;
  }
  if (loadIRCodeArrayFromJson(libJson, "acOff/rawData", irCodeOff, irCodeOffLen)) {
    Serial.println("[IR Library] Loaded acOff (" + String(irCodeOffLen) + " values)");
    anyLoaded = true;
  }
  if (loadIRCodeArrayFromJson(libJson, "temperatureUp/rawData", irCodeTempUp, irCodeTempUpLen)) {
    Serial.println("[IR Library] Loaded temperatureUp (" + String(irCodeTempUpLen) + " values)");
    anyLoaded = true;
  }
  if (loadIRCodeArrayFromJson(libJson, "temperatureDown/rawData", irCodeTempDown, irCodeTempDownLen)) {
    Serial.println("[IR Library] Loaded temperatureDown (" + String(irCodeTempDownLen) + " values)");
    anyLoaded = true;
  }

  if (anyLoaded && cacheOnSuccess) {
    saveIRCodesToPreferences();
  }
}

void handleACCommand(String cmd, int unit, bool isAutomation) {
  Serial.println("🎮 AC Command: " + cmd + " (Unit " + String(unit) + ") " + (isAutomation ? "[Automation]" : "[Manual]"));

  // Only log automation events if this is an automated command
  if (isAutomation) {
    lastAutomationEvent = "AC Power automation: " + cmd + " command sent to Unit " + String(unit);
    lastAutomationEventType = "power";
    lastAutomationEventTime = millis();
    Serial.print("[Automation] Event set: "); Serial.println(lastAutomationEvent);
  } else {
    Serial.println("[Manual] Manual command - not logging as automation event");
  }
  
  // unit==0 is single-unit mode (must use the primary sender/pin, same as
  // every automation branch's #else path); unit==2 is the only case that
  // should use the secondary sender/pin. The previous "unit==1 ? secondary
  // : primary" ternary meant manual commands for Unit 1 fired on pin 5 and
  // Unit 2 fired on pin 4 - backwards from every automation branch, which
  // consistently sends Unit 1 on irsend/pin 4 and Unit 2 on irsend2/pin 5.
  // Confirmed live via serial: a manual "Unit 1 OFF" was logged as sending
  // on pin 5, Unit 2's physical pin.
  IRsend *irSender = (unit == 2) ? &irsend2 : &irsend;
  uint16_t pin = (unit == 2) ? kIrLedPin2 : kIrLedPin;
  
  Serial.println("🎮 Using IRsend instance on pin " + String(pin));
  
  if (cmd == "ON") {
    if (irCodeOnLen == 0) {
      Serial.println("⚠️ No ON code loaded - skipping IR send");
    } else {
      Serial.println("🎮 Sending ON signal...");
      irSender->sendRaw(irCodeOn, irCodeOnLen, kFrequency);
      Serial.println("✓ ON sent on pin " + String(pin));
    }
  } else if (cmd == "OFF") {
    if (irCodeOffLen == 0) {
      Serial.println("⚠️ No OFF code loaded - skipping IR send");
    } else {
      Serial.println("🎮 Sending OFF signal...");
      irSender->sendRaw(irCodeOff, irCodeOffLen, kFrequency);
      Serial.println("✓ OFF sent on pin " + String(pin));
    }
  } else if (cmd == "TEMP_UP") {
    if (irCodeTempUpLen == 0) {
      Serial.println("⚠️ No TEMP_UP code loaded - skipping IR send");
    } else {
      Serial.println("🎮 Sending TEMP_UP signal...");
      irSender->sendRaw(irCodeTempUp, irCodeTempUpLen, kFrequency);
      Serial.println("✓ TEMP_UP sent on pin " + String(pin));
    }
  } else if (cmd == "TEMP_DOWN") {
    if (irCodeTempDownLen == 0) {
      Serial.println("⚠️ No TEMP_DOWN code loaded - skipping IR send");
    } else {
      Serial.println("🎮 Sending TEMP_DOWN signal...");
      irSender->sendRaw(irCodeTempDown, irCodeTempDownLen, kFrequency);
      Serial.println("✓ TEMP_DOWN sent on pin " + String(pin));
    }
  } else {
    Serial.println("⚠️ Unknown command: " + cmd);
  }

  // Only log automation event if this is an automated command
  if (isAutomation) {
    logAutomationEventToHistory();
  } else {
    // Manual override logic REMOVED (AC Power Automation disabled)
  }

  // Update Firebase based on unit
  if (unit == 0) {
    // Single unit mode
    Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/ac_command", "IDLE");
    Firebase.RTDB.setBool(&fbdo, "/" + String(ROOM_ID) + "/ac", (cmd == "ON"));
  } else {
    // Dual unit mode
    String unitPath = "/" + String(ROOM_ID) + "/units/unit_" + String(unit);
    Firebase.RTDB.setString(&fbdo, unitPath + "/ac_command", "IDLE");
    Firebase.RTDB.setBool(&fbdo, unitPath + "/ac", (cmd == "ON"));
  }
  
  Serial.println("✓ Firebase updated");
}

// Function to log automation event to Firebase history immediately
void logAutomationEventToHistory() {
  if (lastAutomationEvent.length() > 0) {
    Serial.print("[History] Logging automation event: "); Serial.println(lastAutomationEvent);
    Serial.print("[History] Event type: "); Serial.println(lastAutomationEventType);
    
    // Use local epoch time in seconds (to avoid 32-bit overflow with milliseconds)
    time_t now = time(nullptr);
    
    Serial.print("[History] Event time (epoch s): "); Serial.println(now);
    
    // Get current hour for Firebase path
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char dateStr[11];
    char hourStr[3];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    strftime(hourStr, sizeof(hourStr), "%H", &timeinfo);
    
    String historyPath = "/history/" + String(ROOM_ID) + "/" + String(dateStr) + "/" + String(hourStr) + "/automation_events";

    FirebaseJson historyData;
    historyData.add("automationEvent", lastAutomationEvent);
    historyData.add("automationEventType", lastAutomationEventType);
    historyData.add("automationEventTime", now);
    historyData.add("automationEventPastTemp", lastAutomationEventPastTemp);
    historyData.add("automationEventUpdatedTemp", lastAutomationEventUpdatedTemp);
    #if DUAL_UNIT_MODE
    // Previously only Unit 1's before/after setpoint was ever logged here,
    // even for events that changed both units - add Unit 2's alongside it
    // rather than silently dropping it.
    historyData.add("automationEventPastTemp2", lastAutomationEventPastTemp2);
    historyData.add("automationEventUpdatedTemp2", lastAutomationEventUpdatedTemp2);
    #endif

    Serial.print("[History] Writing to Firebase path: "); Serial.println(historyPath.c_str());
    // Use pushJSON to preserve all events (creates unique keys)
    bool historySuccess = Firebase.RTDB.pushJSON(&fbdo, historyPath.c_str(), &historyData);
    Serial.print("[History] Firebase write: "); Serial.println(historySuccess ? "success" : "failed");
    if (!historySuccess) {
      Serial.print("[History] Error: "); Serial.println(fbdo.errorReason());
    }

    // Clear the event after logging
    lastAutomationEvent = "";
    lastAutomationEventType = "";
    lastAutomationEventTime = 0;
    lastAutomationEventPastTemp = 0.0;
    lastAutomationEventUpdatedTemp = 0.0;
    #if DUAL_UNIT_MODE
    lastAutomationEventPastTemp2 = 0.0;
    lastAutomationEventUpdatedTemp2 = 0.0;
    #endif
  }
}

void handleIRReceiver() {
  if (irrecv.decode(&irResults)) {
    Serial.println("[IR Receiver] Signal detected!");
    String rawData = "";
    int len = irResults.rawlen;
    Serial.print("[IR Receiver] Raw length: "); Serial.println(len);

    if (len > 0 && len < 200) {
      for (int i = 0; i < len; i++) {
        if (i > 0) rawData += ",";
        rawData += String(irResults.rawbuf[i] * RAWTICK);
      }
      Serial.print("[IR Receiver] Raw data: "); Serial.println(rawData);
    }

    FirebaseJson signalJson;
    signalJson.add("rawData", rawData);
    signalJson.add("length", len);
    signalJson.add("protocol", typeToString(irResults.decode_type, irResults.repeat));
    signalJson.add("timestamp", String(millis()));

    Serial.println("[IR Receiver] Sending to Firebase...");
    bool success = Firebase.RTDB.setJSON(&fbdo, "/irReceiver/lastSignal", &signalJson);
    Serial.print("[IR Receiver] SetJSON success: "); Serial.println(success ? "true" : "false");
    if (!success) {
      Serial.print("[IR Receiver] Error: "); Serial.println(fbdo.errorReason());
    }

    success = Firebase.RTDB.setString(&fbdo, "/irReceiver/state", "received");
    Serial.print("[IR Receiver] SetState success: "); Serial.println(success ? "true" : "false");

    irrecv.resume();
    delay(100);
  }
}

void runAutomation(float temp, float humidity) {
  static unsigned long lastDebugLog = 0;
  if (millis() - lastDebugLog > 60000) {
    // lastAutomationCheckMillis is never updated (leftover from the removed
    // schedule-based automation), so it was only ever showing device uptime,
    // mislabeled as "time since last check". Report the real per-automation
    // countdowns instead - these are the actual gates in the code below.
    unsigned long tempElapsed = millis() - lastTempAutomationCheckMillis;
    unsigned long humElapsed = millis() - lastHumidityAutomationCheckMillis;
    unsigned long tempRemaining = (tempElapsed < AUTOMATION_INTERVAL_MS) ? (AUTOMATION_INTERVAL_MS - tempElapsed) / 1000 : 0;
    unsigned long humRemaining = (humElapsed < AUTOMATION_INTERVAL_MS) ? (AUTOMATION_INTERVAL_MS - humElapsed) / 1000 : 0;
    Serial.print("[Automation Debug] Humidity Enabled: "); Serial.print(automationEnabled);
    Serial.print(", Temp Enabled: "); Serial.print(temperatureAutomationEnabled);
    Serial.print(", Next temp check in: "); Serial.print(tempRemaining);
    Serial.print("s, Next humidity check in: "); Serial.print(humRemaining);
    Serial.println("s");
    lastDebugLog = millis();
  }

  // Temperature safety automation (always runs if enabled, independent of humidity automation)
  if (temperatureAutomationEnabled) {
    if (millis() - lastTempAutomationCheckMillis < AUTOMATION_INTERVAL_MS) {
      // Skip temperature check this time, but continue to humidity check
    } else {
      lastTempAutomationCheckMillis = millis();

      Serial.println("🚨 [Temperature Automation] Running safety check...");
      Serial.print("🌡️ [Temperature Automation] Current: "); Serial.print(temp, 1); Serial.print("°C | Min: "); Serial.print(minTemp, 1); Serial.print("°C | Max: "); Serial.print(maxTemp, 1); Serial.println("°C");

      if (temp > maxTemp) {
        Serial.println("⚠️ TEMPERATURE SAFETY: Current temp exceeds max threshold!");
        Serial.print("Current: "); Serial.print(temp, 1); Serial.print("°C > Max: "); Serial.print(maxTemp, 1); Serial.println("°C");
        Serial.println("Action: Sending TEMP_DOWN to cool down");

        unsigned long timeSinceLastIR = millis() - lastIRSendMillis;
        if (timeSinceLastIR < IR_SEND_COOLDOWN_MS) {
          unsigned long cooldownRemaining = (IR_SEND_COOLDOWN_MS - timeSinceLastIR) / 1000;
          Serial.print("Cooldown active, "); Serial.print(cooldownRemaining); Serial.println(" seconds remaining");
        } else if (irCodeTempDownLen == 0) {
          Serial.println("⚠️ No TEMP_DOWN code loaded - skipping automation IR send");
        } else {
          Serial.print("Sending TEMP_DOWN on pin "); Serial.println(kIrLedPin);
          #if DUAL_UNIT_MODE
          Serial.println("Sending to Unit 1 (pin 4)");
          irsend.sendRaw(irCodeTempDown, irCodeTempDownLen, kFrequency);
          delay(100);
          Serial.println("Sending to Unit 2 (pin 5)");
          irsend2.sendRaw(irCodeTempDown, irCodeTempDownLen, kFrequency);
          #else
          irsend.sendRaw(irCodeTempDown, irCodeTempDownLen, kFrequency);
          #endif
          Serial.println("TEMP_DOWN sent successfully");
          lastIRSendMillis = millis();
          // Step the AC's own target temp down by 1, not the room's
          // ambient sensor reading - a TEMP_DOWN button press moves the
          // AC's setpoint by one degree, it doesn't matter what the room
          // sensor happens to read. Clamped to the unit's real 16-30
          // range (same range Settings' +/- buttons use) so this can
          // never write a value the AC can't actually be set to - which
          // previously fed a bogus target into the cost calibration and
          // made "Past Temp"/"Automated Temp" show room-temperature
          // values instead of AC setpoints.
          // If a getFloat read fails (this device's WiFi drops routinely
          // enough that it already has its own reconnect watchdog), do
          // NOT fall back to writing a hardcoded 24.0-based guess back to
          // Firebase as if it were real - that would permanently desync
          // the tracked setpoint from the AC's real one, and every future
          // cycle would then step from the wrong baseline with no way to
          // self-correct. Skip the write instead; Firebase just stays one
          // step stale until a read succeeds, which is a far safer failure
          // mode than persisting a wrong value as fact.
          #if DUAL_UNIT_MODE
          float priorTarget1 = 24.0, priorTarget2 = 24.0;
          bool gotPrior1 = Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/targetTemp");
          if (gotPrior1) priorTarget1 = fbdo.floatData();
          bool gotPrior2 = Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/targetTemp");
          if (gotPrior2) priorTarget2 = fbdo.floatData();
          float newTargetTemp1 = constrain(priorTarget1 - 1.0, 16.0, 30.0);
          float newTargetTemp2 = constrain(priorTarget2 - 1.0, 16.0, 30.0);
          if (gotPrior1) {
            Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/targetTemp", newTargetTemp1);
            Serial.print("[Automation] Updated Firebase unit_1 targetTemp to: "); Serial.println(newTargetTemp1, 1);
          } else {
            Serial.println("[Automation] Skipped unit_1 targetTemp write - could not read current setpoint");
          }
          if (gotPrior2) {
            Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/targetTemp", newTargetTemp2);
            Serial.print("[Automation] Updated Firebase unit_2 targetTemp to: "); Serial.println(newTargetTemp2, 1);
          } else {
            Serial.println("[Automation] Skipped unit_2 targetTemp write - could not read current setpoint");
          }
          float priorTarget = priorTarget1;
          float newTargetTemp = newTargetTemp1;
          lastAutomationEventPastTemp2 = priorTarget2;
          lastAutomationEventUpdatedTemp2 = newTargetTemp2;
          #else
          float priorTarget = 24.0;
          bool gotPrior = Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/targetTemp");
          if (gotPrior) priorTarget = fbdo.floatData();
          float newTargetTemp = constrain(priorTarget - 1.0, 16.0, 30.0);
          if (gotPrior) {
            Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/targetTemp", newTargetTemp);
            Serial.print("[Automation] Updated Firebase targetTemp to: "); Serial.println(newTargetTemp, 1);
          } else {
            Serial.println("[Automation] Skipped targetTemp write - could not read current setpoint");
          }
          #endif
          // Log automation event with the AC's own target temp before/
          // after (both within 16-30, like the physical remote) rather
          // than the room's ambient sensor reading - "Room Temperature"
          // is already shown separately in the chart tooltip.
          lastAutomationEvent = "Temperature safety: TEMP_DOWN sent (temp exceeded max)";
          lastAutomationEventType = "temperature";
          lastAutomationEventTime = millis();
          lastAutomationEventPastTemp = priorTarget;
          lastAutomationEventUpdatedTemp = newTargetTemp;
          Serial.print("[Automation] Event set: "); Serial.println(lastAutomationEvent);
          Serial.print("[Automation] Past temp: "); Serial.print(lastAutomationEventPastTemp, 1); Serial.print("°C, Updated temp: "); Serial.print(lastAutomationEventUpdatedTemp, 1); Serial.println("°C");
          // Log to Firebase history
          logAutomationEventToHistory();
        }
      }

      if (temp < minTemp) {
        Serial.println("⚠️ TEMPERATURE SAFETY: Current temp below min threshold!");
        Serial.print("Current: "); Serial.print(temp, 1); Serial.print("°C < Min: "); Serial.print(minTemp, 1); Serial.println("°C");
        Serial.println("Action: Sending TEMP_UP to warm up");

        unsigned long timeSinceLastIR = millis() - lastIRSendMillis;
        if (timeSinceLastIR < IR_SEND_COOLDOWN_MS) {
          unsigned long cooldownRemaining = (IR_SEND_COOLDOWN_MS - timeSinceLastIR) / 1000;
          Serial.print("Cooldown active, "); Serial.print(cooldownRemaining); Serial.println(" seconds remaining");
        } else if (irCodeTempUpLen == 0) {
          Serial.println("⚠️ No TEMP_UP code loaded - skipping automation IR send");
        } else {
          Serial.print("Sending TEMP_UP on pin "); Serial.println(kIrLedPin);
          #if DUAL_UNIT_MODE
          Serial.println("Sending to Unit 1 (pin 4)");
          irsend.sendRaw(irCodeTempUp, irCodeTempUpLen, kFrequency);
          delay(100);
          Serial.println("Sending to Unit 2 (pin 5)");
          irsend2.sendRaw(irCodeTempUp, irCodeTempUpLen, kFrequency);
          #else
          irsend.sendRaw(irCodeTempUp, irCodeTempUpLen, kFrequency);
          #endif
          Serial.println("TEMP_UP sent successfully");
          lastIRSendMillis = millis();
          // Step the AC's own target temp up by 1 - see the matching
          // comment in the TEMP_DOWN branch above for why this reads the
          // current setpoint instead of the room's ambient sensor temp.
          // See the matching comment in the TEMP_DOWN branch above for why
          // a failed read skips the Firebase write instead of falling
          // back to a hardcoded guess.
          #if DUAL_UNIT_MODE
          float priorTarget1 = 24.0, priorTarget2 = 24.0;
          bool gotPrior1 = Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/targetTemp");
          if (gotPrior1) priorTarget1 = fbdo.floatData();
          bool gotPrior2 = Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/targetTemp");
          if (gotPrior2) priorTarget2 = fbdo.floatData();
          float newTargetTemp1 = constrain(priorTarget1 + 1.0, 16.0, 30.0);
          float newTargetTemp2 = constrain(priorTarget2 + 1.0, 16.0, 30.0);
          if (gotPrior1) {
            Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/targetTemp", newTargetTemp1);
            Serial.print("[Automation] Updated Firebase unit_1 targetTemp to: "); Serial.println(newTargetTemp1, 1);
          } else {
            Serial.println("[Automation] Skipped unit_1 targetTemp write - could not read current setpoint");
          }
          if (gotPrior2) {
            Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/targetTemp", newTargetTemp2);
            Serial.print("[Automation] Updated Firebase unit_2 targetTemp to: "); Serial.println(newTargetTemp2, 1);
          } else {
            Serial.println("[Automation] Skipped unit_2 targetTemp write - could not read current setpoint");
          }
          float priorTarget = priorTarget1;
          float newTargetTemp = newTargetTemp1;
          lastAutomationEventPastTemp2 = priorTarget2;
          lastAutomationEventUpdatedTemp2 = newTargetTemp2;
          #else
          float priorTarget = 24.0;
          bool gotPrior = Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/targetTemp");
          if (gotPrior) priorTarget = fbdo.floatData();
          float newTargetTemp = constrain(priorTarget + 1.0, 16.0, 30.0);
          if (gotPrior) {
            Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/targetTemp", newTargetTemp);
            Serial.print("[Automation] Updated Firebase targetTemp to: "); Serial.println(newTargetTemp, 1);
          } else {
            Serial.println("[Automation] Skipped targetTemp write - could not read current setpoint");
          }
          #endif
          // Log automation event with the AC's own target temp before/
          // after (both within 16-30), not the room's ambient reading.
          lastAutomationEvent = "Temperature safety: TEMP_UP sent (temp below min)";
          lastAutomationEventType = "temperature";
          lastAutomationEventTime = millis();
          lastAutomationEventPastTemp = priorTarget;
          lastAutomationEventUpdatedTemp = newTargetTemp;
          Serial.print("[Automation] Event set: "); Serial.println(lastAutomationEvent);
          Serial.print("[Automation] Past temp: "); Serial.print(lastAutomationEventPastTemp, 1); Serial.print("°C, Updated temp: "); Serial.print(lastAutomationEventUpdatedTemp, 1); Serial.println("°C");
          // Log to Firebase history
          logAutomationEventToHistory();
        }
      }
    }
  }

  // Humidity-based automation (only runs if humidity automation is enabled)
  if (!automationEnabled) {
    static unsigned long lastDisabledLog = 0;
    if (millis() - lastDisabledLog > 10000) {
      Serial.println("❌ [Humidity Automation] Skipped - automation disabled");
      lastDisabledLog = millis();
    }
    return;
  }

  if (millis() - lastHumidityAutomationCheckMillis < AUTOMATION_INTERVAL_MS) {
    static unsigned long lastIntervalLog = 0;
    if (millis() - lastIntervalLog > 10000) {
      Serial.println("⏱️ [Humidity Automation] Skipped - interval not reached");
      lastIntervalLog = millis();
    }
    return;
  }
  lastHumidityAutomationCheckMillis = millis();

  Serial.println("📊 [Humidity Automation] Running check...");
  Serial.print("🌡️ [Humidity Automation] Temp: "); Serial.print(temp, 1); Serial.print("°C | Humidity: "); Serial.print(humidity, 1); Serial.print("% | Thresholds: "); Serial.print(MIN_HUMIDITY, 1); Serial.print("% - "); Serial.print(MAX_HUMIDITY, 1); Serial.println("%");
  Serial.print("🔍 [Humidity Automation] Humidity check: "); Serial.print(humidity, 1); Serial.print("% > "); Serial.print(MAX_HUMIDITY, 1); Serial.print("? "); Serial.println(humidity > MAX_HUMIDITY ? "YES" : "NO");
  Serial.print("🔍 [Humidity Automation] Humidity check: "); Serial.print(humidity, 1); Serial.print("% < "); Serial.print(MIN_HUMIDITY, 1); Serial.print("? "); Serial.println(humidity < MIN_HUMIDITY ? "YES" : "NO");

  float targetTemp;
  // Hoisted out of the branches below (rather than declared fresh inside
  // each) so the IR step-count math further down can use the AC's real
  // prior setpoint instead of the room's ambient sensor temp.
  float pastTargetTemp = 24.0;
  String humidityEventName;
  #if DUAL_UNIT_MODE
  float pastTargetTemp2 = 24.0;
  #endif

  if (humidity > MAX_HUMIDITY) {
    // minTemp/maxTemp double as room-ambient safety thresholds elsewhere
    // in this file, which an admin could legitimately configure outside
    // the AC's real 16-30 range (e.g. as extra safety margin) - clamp
    // before writing it as a setpoint so that can never happen here.
    targetTemp = constrain(maxTemp, 16.0, 30.0);
    humidityEventName = "Humidity automation: Occupied detected, setting target to max temp";
    Serial.println("📊 HUMIDITY AUTOMATION: Occupied detected");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.print("% > "); Serial.print(MAX_HUMIDITY); Serial.print("%, setting target to max: "); Serial.println(targetTemp, 1);
  } else if (humidity < MIN_HUMIDITY) {
    targetTemp = constrain(minTemp, 16.0, 30.0);
    humidityEventName = "Humidity automation: Not occupied detected, setting target to min temp";
    Serial.println("📊 HUMIDITY AUTOMATION: Not occupied detected");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.print("% < "); Serial.print(MIN_HUMIDITY); Serial.print("%, setting target to min: "); Serial.println(targetTemp, 1);
  } else {
    Serial.println("✅ [Humidity Automation] Humidity within normal range (45-60%), no action needed");
    return;
  }

  // Read the AC's actual current setpoint(s) before deciding anything -
  // read-only, no Firebase writes here. The write of the new setpoint is
  // deferred until after an IR command is actually confirmed sent (see
  // below), matching the temperature-safety code above, so a skipped or
  // failed send can never make Firebase claim a setpoint change that
  // never reached the physical AC.
  bool gotPastTemp = false;
  #if DUAL_UNIT_MODE
  if (Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/targetTemp")) {
    pastTargetTemp = fbdo.floatData();
    gotPastTemp = true;
  }
  // unit_2's prior value is only used for event history display below,
  // never to decide whether/how much to send - see the note further down
  // where it feeds lastAutomationEventPastTemp2.
  bool gotPastTemp2 = Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/targetTemp");
  if (gotPastTemp2) {
    pastTargetTemp2 = fbdo.floatData();
  }
  #else
  if (Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/targetTemp")) {
    pastTargetTemp = fbdo.floatData();
    gotPastTemp = true;
  }
  #endif

  if (!gotPastTemp) {
    Serial.println("[Humidity Automation] Skipped - could not read current setpoint");
    return;
  }

  Serial.print("Target temp: "); Serial.print(targetTemp, 1); Serial.print("°C, Prior setpoint: "); Serial.print(pastTargetTemp, 1); Serial.println("°C");
  Serial.println("Checking if temp change needed...");

  if (targetTemp != pastTargetTemp) {
    unsigned long timeSinceLastIR = millis() - lastIRSendMillis;
    Serial.print("Time since last IR send: "); Serial.print(timeSinceLastIR / 1000); Serial.println(" seconds");
    if (timeSinceLastIR < IR_SEND_COOLDOWN_MS) {
      unsigned long cooldownRemaining = (IR_SEND_COOLDOWN_MS - timeSinceLastIR) / 1000;
      Serial.print("Cooldown active, "); Serial.print(cooldownRemaining); Serial.println(" seconds remaining");
      // Nothing was written to Firebase or lastAutomationEvent* yet (both
      // are only set after a confirmed send, below), so there is nothing
      // to roll back here - just skip this cycle.
      Serial.println("Skipping - no IR was actually sent");
      return;
    }
    Serial.println("No cooldown active, proceeding with IR commands");

    float difference1 = targetTemp - pastTargetTemp;
    int steps1 = (int)round(abs(difference1));
    bool up1 = difference1 > 0;
    Serial.print("Unit 1: adjusting by "); Serial.print(difference1, 1); Serial.print("°C ("); Serial.print(steps1); Serial.println(" steps)");

    #if DUAL_UNIT_MODE
    // Unit 2 may have drifted from Unit 1's setpoint (e.g. a manual
    // per-unit override) - its own step count and direction are computed
    // independently rather than assuming it needs Unit 1's commands,
    // which could otherwise land it at the wrong temperature.
    bool adjustUnit2 = gotPastTemp2 && (pastTargetTemp2 != targetTemp);
    float difference2 = adjustUnit2 ? (targetTemp - pastTargetTemp2) : 0.0;
    int steps2 = adjustUnit2 ? (int)round(abs(difference2)) : 0;
    bool up2 = difference2 > 0;
    if (adjustUnit2) {
      Serial.print("Unit 2: adjusting by "); Serial.print(difference2, 1); Serial.print("°C ("); Serial.print(steps2); Serial.println(" steps)");
    } else if (!gotPastTemp2) {
      Serial.println("Unit 2: prior setpoint unknown - leaving untouched this cycle");
    } else {
      Serial.println("Unit 2: already at target, no adjustment needed");
    }
    #endif

    bool needUp = up1;
    bool needDown = !up1;
    #if DUAL_UNIT_MODE
    if (adjustUnit2 && up2) needUp = true;
    if (adjustUnit2 && !up2) needDown = true;
    #endif

    if ((needUp && irCodeTempUpLen == 0) || (needDown && irCodeTempDownLen == 0)) {
      Serial.println("⚠️ Missing IR code for a needed direction - skipping automation IR send");
      return;
    }

    Serial.println("Starting IR transmission...");

    int maxSteps = steps1;
    #if DUAL_UNIT_MODE
    if (steps2 > maxSteps) maxSteps = steps2;
    #endif

    for (int i = 0; i < maxSteps; i++) {
      if (i < steps1) {
        Serial.print("Step "); Serial.print(i + 1); Serial.print("/"); Serial.print(steps1); Serial.print(": Unit 1 "); Serial.println(up1 ? "TEMP UP" : "TEMP DOWN");
        if (up1) irsend.sendRaw(irCodeTempUp, irCodeTempUpLen, kFrequency);
        else irsend.sendRaw(irCodeTempDown, irCodeTempDownLen, kFrequency);
      }
      #if DUAL_UNIT_MODE
      if (i < steps1 && i < steps2) delay(100); // gap between back-to-back sends this round
      if (i < steps2) {
        Serial.print("Step "); Serial.print(i + 1); Serial.print("/"); Serial.print(steps2); Serial.print(": Unit 2 "); Serial.println(up2 ? "TEMP UP" : "TEMP DOWN");
        if (up2) irsend2.sendRaw(irCodeTempUp, irCodeTempUpLen, kFrequency);
        else irsend2.sendRaw(irCodeTempDown, irCodeTempDownLen, kFrequency);
      }
      #endif

      if (i < maxSteps - 1) {
        delay(20000);
      }
    }

    lastIRSendMillis = millis();
    Serial.print("Humidity automation complete. Target temp: "); Serial.println(targetTemp, 1);

    // The IR command(s) are now confirmed sent - only now persist the new
    // setpoint(s) to Firebase and log the event, so a cooldown-skip or
    // failed-read above (both of which return before this point) can
    // never leave Firebase claiming a setpoint change that never reached
    // the physical AC. Unit 2's Firebase value (and event log entry) is
    // only touched if it was actually sent commands this cycle, or was
    // already confirmed at the target - never guessed.
    #if DUAL_UNIT_MODE
    Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/targetTemp", targetTemp);
    Serial.print("[Automation] Updated Firebase unit_1 targetTemp to: "); Serial.println(targetTemp, 1);
    if (adjustUnit2) {
      Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/targetTemp", targetTemp);
      Serial.print("[Automation] Updated Firebase unit_2 targetTemp to: "); Serial.println(targetTemp, 1);
      lastAutomationEventPastTemp2 = pastTargetTemp2;
      lastAutomationEventUpdatedTemp2 = targetTemp;
    } else if (gotPastTemp2) {
      lastAutomationEventPastTemp2 = pastTargetTemp2;
      lastAutomationEventUpdatedTemp2 = pastTargetTemp2;
    } else {
      lastAutomationEventPastTemp2 = pastTargetTemp;
      lastAutomationEventUpdatedTemp2 = pastTargetTemp;
    }
    #else
    Firebase.RTDB.setFloat(&fbdo, "/" + String(ROOM_ID) + "/targetTemp", targetTemp);
    Serial.print("[Automation] Updated Firebase targetTemp to: "); Serial.println(targetTemp, 1);
    #endif

    lastAutomationEvent = humidityEventName;
    lastAutomationEventType = "humidity";
    lastAutomationEventTime = millis();
    lastAutomationEventPastTemp = pastTargetTemp;
    lastAutomationEventUpdatedTemp = targetTemp;
    Serial.print("[Automation] Event set: "); Serial.println(lastAutomationEvent);
    Serial.print("[Automation] Past temp: "); Serial.print(lastAutomationEventPastTemp, 1); Serial.print("°C, Updated temp: "); Serial.print(lastAutomationEventUpdatedTemp, 1); Serial.println("°C");

    // Log humidity automation event to Firebase history
    logAutomationEventToHistory();
  } else {
    Serial.println("Target temp matches current temp, no action needed");
  }
}

// Schedule Efficiency Functions - REMOVED (AC Power Automation disabled)