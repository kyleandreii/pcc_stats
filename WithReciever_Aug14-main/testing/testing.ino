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
#include <time.h>
#include <Preferences.h>

#define OLED_SDA 21
#define OLED_SCL 22

#define WIFI_SSID "BA1D616R8"
#define WIFI_PASSWORD "GM6AA4X8rm128!"
#define DATABASE_SECRET "DvhZNBNmaFVHMFSup70ezVZlEVHWwO87OrGK4Td7"
#define DATABASE_URL "https://pcc-stats-default-rtdb.asia-southeast1.firebasedatabase.app/"

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

bool automationEnabled = false;  // Controls humidity-based automation
bool temperatureAutomationEnabled = false;  // Temperature safety disabled
unsigned long automationStartTime = 0;  // Timestamp when automation was last enabled
String lastAutomationEvent = "";  // Description of last automation action
String lastAutomationEventType = "";  // Type: "humidity", "temperature", "power"
unsigned long lastAutomationEventTime = 0;  // Timestamp of last automation action
float lastAutomationEventPastTemp = 0.0;  // Temperature before automation
float lastAutomationEventUpdatedTemp = 0.0;  // Temperature after automation

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

unsigned long lastAutomationCheckMillis = 0;
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== PCC S.T.A.T.S. System Starting ===");

  preferences.begin("pcc-automation", false);
  automationEnabled = preferences.getBool("automationEnabled", false);
  Serial.println("Loaded automation enabled state from Preferences: " + String(automationEnabled));

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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected");

  // Configure NTP time
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

  Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/automation/enabled");
  if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    automationEnabled = fbdo.boolData();
    preferences.putBool("automationEnabled", automationEnabled);
    Serial.println("Synced automation enabled state from Firebase: " + String(automationEnabled));
    if (automationEnabled) {
      Serial.println("✅ [Setup] Humidity automation is ENABLED");
    } else {
      Serial.println("❌ [Setup] Humidity automation is DISABLED");
    }
  } else {
    Serial.println("⚠️ [Setup] Failed to read automation enabled state from Firebase. HTTP code: " + String(fbdo.httpCode()));
  }

  Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/automation/humidityOccupiedThreshold");
  if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    float threshold = fbdo.floatData();
    if (threshold > 0) {
      humidityOccupiedThreshold = threshold;
      MAX_HUMIDITY = threshold;
      Serial.println("Synced humidity occupied threshold from Firebase: " + String(humidityOccupiedThreshold) + "%");
    } else {
      Serial.println("⚠️ [Setup] Firebase humidity occupied threshold is 0 or invalid, using default: " + String(MAX_HUMIDITY) + "%");
    }
  } else {
    Serial.println("⚠️ [Setup] Failed to read humidity occupied threshold from Firebase. HTTP code: " + String(fbdo.httpCode()) + ", using default: " + String(MAX_HUMIDITY) + "%");
  }

  Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/automation/humidityEmptyThreshold");
  if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    float threshold = fbdo.floatData();
    if (threshold > 0) {
      humidityEmptyThreshold = threshold;
      MIN_HUMIDITY = threshold;
      Serial.println("Synced humidity empty threshold from Firebase: " + String(humidityEmptyThreshold) + "%");
    } else {
      Serial.println("⚠️ [Setup] Firebase humidity empty threshold is 0 or invalid, using default: " + String(MIN_HUMIDITY) + "%");
    }
  } else {
    Serial.println("⚠️ [Setup] Failed to read humidity empty threshold from Firebase. HTTP code: " + String(fbdo.httpCode()) + ", using default: " + String(MIN_HUMIDITY) + "%");
  }

  Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/thresholds/minTemp");
  if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    float temp = fbdo.floatData();
    if (temp > 0) {
      minTemp = temp;
      Serial.println("Synced minTemp from Firebase: " + String(minTemp) + "°C");
    }
  }

  Firebase.RTDB.getFloat(&fbdo, "/" + String(ROOM_ID) + "/thresholds/maxTemp");
  if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    float temp = fbdo.floatData();
    if (temp > 0) {
      maxTemp = temp;
      Serial.println("Synced maxTemp from Firebase: " + String(maxTemp) + "°C");
    }
  }

  // Schedule configuration loading REMOVED (AC Power Automation disabled)

  Serial.println("[Setup] Temperature Safety Automation: DISABLED");
  Serial.println("[Setup] Automation Interval: " + String(AUTOMATION_INTERVAL_MS / 1000) + " seconds");
  Serial.println("[Setup] IR Send Cooldown: " + String(IR_SEND_COOLDOWN_MS / 1000) + " seconds");

  FirebaseJson json;
  json.set(".sv", "timestamp");
  Firebase.RTDB.setJSON(&fbdo, "/" + String(ROOM_ID) + "/last_seen", &json);
  Firebase.RTDB.setBool(&fbdo, "/" + String(ROOM_ID) + "/online", true);
  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/device_room_id", ROOM_ID);

  // Load online duration from Firebase
  Firebase.RTDB.getInt(&fbdo, "/" + String(ROOM_ID) + "/online_duration");
  if (fbdo.dataType() == "int") {
    onlineDurationSeconds = fbdo.intData();
    Serial.println("[Setup] Loaded online_duration from Firebase: " + String(onlineDurationSeconds) + " seconds");
  } else {
    Serial.println("[Setup] No online_duration found in Firebase, starting at 0");
  }

  #if DUAL_UNIT_MODE
  Firebase.RTDB.getInt(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/online_duration");
  if (fbdo.dataType() == "int") {
    onlineDurationSecondsUnit1 = fbdo.intData();
    Serial.println("[Setup] Loaded online_duration for unit 1 from Firebase: " + String(onlineDurationSecondsUnit1) + " seconds");
  } else {
    Serial.println("[Setup] No online_duration found for unit 1 in Firebase, starting at 0");
  }

  Firebase.RTDB.getInt(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/online_duration");
  if (fbdo.dataType() == "int") {
    onlineDurationSecondsUnit2 = fbdo.intData();
    Serial.println("[Setup] Loaded online_duration for unit 2 from Firebase: " + String(onlineDurationSecondsUnit2) + " seconds");
  } else {
    Serial.println("[Setup] No online_duration found for unit 2 in Firebase, starting at 0");
  }
  #endif

  #if DUAL_UNIT_MODE
  // Force AC to false on startup
  Firebase.RTDB.setBool(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/ac", false);
  Firebase.RTDB.setBool(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/ac", false);
  Serial.println("[Setup] Forcing AC to false on startup");
  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/units/unit_1/ac_command", "IDLE");
  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/units/unit_2/ac_command", "IDLE");
  #endif
}

void loop() {
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
    
    // Log important paths, ignore system paths like /last_seen and ac_command spam
    if (path != "/last_seen" && path.indexOf("last_seen") < 0 && !path.endsWith("ac_command")) {
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
        preferences.putBool("automationEnabled", automationEnabled);
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
          preferences.putBool("automationEnabled", automationEnabled);
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
  
  IRsend *irSender = (unit == 1) ? &irsend2 : &irsend;
  uint16_t pin = (unit == 1) ? kIrLedPin2 : kIrLedPin;
  
  Serial.println("🎮 Using IRsend instance on pin " + String(pin));
  
  if (cmd == "ON") {
    uint16_t on_buf[rawOnLen];
    memcpy_P(on_buf, rawOn, sizeof(rawOn));
    Serial.println("🎮 Sending ON signal...");
    irSender->sendRaw(on_buf, rawOnLen, kFrequency);
    Serial.println("✓ ON sent on pin " + String(pin));
  } else if (cmd == "OFF") {
    uint16_t off_buf[rawOffLen];
    memcpy_P(off_buf, rawOff, sizeof(rawOff));
    Serial.println("🎮 Sending OFF signal...");
    irSender->sendRaw(off_buf, rawOffLen, kFrequency);
    Serial.println("✓ OFF sent on pin " + String(pin));
  } else if (cmd == "TEMP_UP") {
    uint16_t up_buf[rawTempUpLen];
    memcpy_P(up_buf, rawTempUp, sizeof(rawTempUp));
    Serial.println("🎮 Sending TEMP_UP signal...");
    irSender->sendRaw(up_buf, rawTempUpLen, kFrequency);
    Serial.println("✓ TEMP_UP sent on pin " + String(pin));
  } else if (cmd == "TEMP_DOWN") {
    uint16_t down_buf[rawTempDownLen];
    memcpy_P(down_buf, rawTempDown, sizeof(rawTempDown));
    Serial.println("🎮 Sending TEMP_DOWN signal...");
    irSender->sendRaw(down_buf, rawTempDownLen, kFrequency);
    Serial.println("✓ TEMP_DOWN sent on pin " + String(pin));
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
    signalJson.add("protocol", "PANASONIC");
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
    Serial.print("[Automation Debug] Humidity Enabled: "); Serial.print(automationEnabled);
    Serial.print(", Temp Enabled: "); Serial.print(temperatureAutomationEnabled);
    Serial.print(", Time since last check: "); Serial.print((millis() - lastAutomationCheckMillis) / 1000);
    Serial.println(" seconds");
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
        } else {
          uint16_t down_buf[rawTempDownLen];
          memcpy_P(down_buf, rawTempDown, sizeof(rawTempDown));
          Serial.print("Sending TEMP_DOWN on pin "); Serial.println(kIrLedPin);
          #if DUAL_UNIT_MODE
          Serial.println("Sending to Unit 1 (pin 4)");
          irsend.sendRaw(down_buf, rawTempDownLen, kFrequency);
          delay(100);
          Serial.println("Sending to Unit 2 (pin 5)");
          irsend2.sendRaw(down_buf, rawTempDownLen, kFrequency);
          #else
          irsend.sendRaw(down_buf, rawTempDownLen, kFrequency);
          #endif
          Serial.println("TEMP_DOWN sent successfully");
          lastIRSendMillis = millis();
          // Update Firebase targetTemp
          float newTargetTemp = temp - 1.0;
          #if DUAL_UNIT_MODE
          Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_1/targetTemp", newTargetTemp);
          Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_2/targetTemp", newTargetTemp);
          Serial.print("[Automation] Updated Firebase unit_1 targetTemp to: "); Serial.println(newTargetTemp, 1);
          Serial.print("[Automation] Updated Firebase unit_2 targetTemp to: "); Serial.println(newTargetTemp, 1);
          #else
          Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/targetTemp", newTargetTemp);
          Serial.print("[Automation] Updated Firebase targetTemp to: "); Serial.println(newTargetTemp, 1);
          #endif
          // Log automation event with temperature data
          lastAutomationEvent = "Temperature safety: TEMP_DOWN sent (temp exceeded max)";
          lastAutomationEventType = "temperature";
          lastAutomationEventTime = millis();
          lastAutomationEventPastTemp = temp;
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
        } else {
          uint16_t up_buf[rawTempUpLen];
          memcpy_P(up_buf, rawTempUp, sizeof(rawTempUp));
          Serial.print("Sending TEMP_UP on pin "); Serial.println(kIrLedPin);
          #if DUAL_UNIT_MODE
          Serial.println("Sending to Unit 1 (pin 4)");
          irsend.sendRaw(up_buf, rawTempUpLen, kFrequency);
          delay(100);
          Serial.println("Sending to Unit 2 (pin 5)");
          irsend2.sendRaw(up_buf, rawTempUpLen, kFrequency);
          #else
          irsend.sendRaw(up_buf, rawTempUpLen, kFrequency);
          #endif
          Serial.println("TEMP_UP sent successfully");
          lastIRSendMillis = millis();
          // Update Firebase targetTemp
          float newTargetTemp = temp + 1.0;
          #if DUAL_UNIT_MODE
          Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_1/targetTemp", newTargetTemp);
          Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_2/targetTemp", newTargetTemp);
          Serial.print("[Automation] Updated Firebase unit_1 targetTemp to: "); Serial.println(newTargetTemp, 1);
          Serial.print("[Automation] Updated Firebase unit_2 targetTemp to: "); Serial.println(newTargetTemp, 1);
          #else
          Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/targetTemp", newTargetTemp);
          Serial.print("[Automation] Updated Firebase targetTemp to: "); Serial.println(newTargetTemp, 1);
          #endif
          // Log automation event with temperature data
          lastAutomationEvent = "Temperature safety: TEMP_UP sent (temp below min)";
          lastAutomationEventType = "temperature";
          lastAutomationEventTime = millis();
          lastAutomationEventPastTemp = temp;
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

  if (humidity > MAX_HUMIDITY) {
    targetTemp = maxTemp;
    Serial.println("📊 HUMIDITY AUTOMATION: Occupied detected");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.print("% > "); Serial.print(MAX_HUMIDITY); Serial.print("%, setting target to max: "); Serial.println(targetTemp, 1);
    // Update Firebase targetTemp
    #if DUAL_UNIT_MODE
    Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_1/targetTemp", targetTemp);
    Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_2/targetTemp", targetTemp);
    Serial.print("[Automation] Updated Firebase unit_1 targetTemp to: "); Serial.println(targetTemp, 1);
    Serial.print("[Automation] Updated Firebase unit_2 targetTemp to: "); Serial.println(targetTemp, 1);
    #else
    Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/targetTemp", targetTemp);
    Serial.print("[Automation] Updated Firebase targetTemp to: "); Serial.println(targetTemp, 1);
    #endif
    lastAutomationEvent = "Humidity automation: Occupied detected, setting target to max temp";
    lastAutomationEventType = "humidity";
    lastAutomationEventTime = millis();
    lastAutomationEventPastTemp = temp;
    lastAutomationEventUpdatedTemp = targetTemp;
    Serial.print("[Automation] Event set: "); Serial.println(lastAutomationEvent);
    Serial.print("[Automation] Past temp: "); Serial.print(lastAutomationEventPastTemp, 1); Serial.print("°C, Updated temp: "); Serial.print(lastAutomationEventUpdatedTemp, 1); Serial.println("°C");
  } else if (humidity < MIN_HUMIDITY) {
    targetTemp = minTemp;
    Serial.println("📊 HUMIDITY AUTOMATION: Not occupied detected");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.print("% < "); Serial.print(MIN_HUMIDITY); Serial.print("%, setting target to min: "); Serial.println(targetTemp, 1);
    // Update Firebase targetTemp
    #if DUAL_UNIT_MODE
    Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_1/targetTemp", targetTemp);
    Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/units/unit_2/targetTemp", targetTemp);
    Serial.print("[Automation] Updated Firebase unit_1 targetTemp to: "); Serial.println(targetTemp, 1);
    Serial.print("[Automation] Updated Firebase unit_2 targetTemp to: "); Serial.println(targetTemp, 1);
    #else
    Firebase.RTDB.setFloat(&fbdo, String(ROOM_ID) + "/targetTemp", targetTemp);
    Serial.print("[Automation] Updated Firebase targetTemp to: "); Serial.println(targetTemp, 1);
    #endif
    lastAutomationEvent = "Humidity automation: Not occupied detected, setting target to min temp";
    lastAutomationEventType = "humidity";
    lastAutomationEventTime = millis();
    lastAutomationEventPastTemp = temp;
    lastAutomationEventUpdatedTemp = targetTemp;
    Serial.print("[Automation] Event set: "); Serial.println(lastAutomationEvent);
    Serial.print("[Automation] Past temp: "); Serial.print(lastAutomationEventPastTemp, 1); Serial.print("°C, Updated temp: "); Serial.print(lastAutomationEventUpdatedTemp, 1); Serial.println("°C");
  } else {
    Serial.println("✅ [Humidity Automation] Humidity within normal range (45-60%), no action needed");
    return;
  }

  Serial.print("Target temp: "); Serial.print(targetTemp, 1); Serial.print("°C, Current sensor temp: "); Serial.print(temp, 1); Serial.println("°C");
  Serial.println("Checking if temp change needed...");

  if (targetTemp != temp) {
    unsigned long timeSinceLastIR = millis() - lastIRSendMillis;
    Serial.print("Time since last IR send: "); Serial.print(timeSinceLastIR / 1000); Serial.println(" seconds");
    if (timeSinceLastIR < IR_SEND_COOLDOWN_MS) {
      unsigned long cooldownRemaining = (IR_SEND_COOLDOWN_MS - timeSinceLastIR) / 1000;
      Serial.print("Cooldown active, "); Serial.print(cooldownRemaining); Serial.println(" seconds remaining");
      Serial.println("Logging automation event to history despite cooldown");
      logAutomationEventToHistory();
      return;
    }
    Serial.println("No cooldown active, proceeding with IR commands");

    float difference = targetTemp - temp;
    int steps = (int)round(abs(difference));
    Serial.print("Adjusting temp by "); Serial.print(difference, 1); Serial.print("°C ("); Serial.print(steps); Serial.println(" steps)");
    Serial.println("Starting IR transmission...");

    for (int i = 0; i < steps; i++) {
      if (difference > 0) {
        uint16_t up_buf[rawTempUpLen];
        memcpy_P(up_buf, rawTempUp, sizeof(rawTempUp));
        Serial.print("Step "); Serial.print(i + 1); Serial.print("/"); Serial.print(steps); Serial.print(": Sending TEMP UP on pin "); Serial.println(kIrLedPin);
        #if DUAL_UNIT_MODE
        Serial.println("  -> Unit 1 (pin 4)");
        irsend.sendRaw(up_buf, rawTempUpLen, kFrequency);
        delay(100);
        Serial.println("  -> Unit 2 (pin 5)");
        irsend2.sendRaw(up_buf, rawTempUpLen, kFrequency);
        #else
        irsend.sendRaw(up_buf, rawTempUpLen, kFrequency);
        #endif
        Serial.println("TEMP UP sent");
      } else {
        uint16_t down_buf[rawTempDownLen];
        memcpy_P(down_buf, rawTempDown, sizeof(rawTempDown));
        Serial.print("Step "); Serial.print(i + 1); Serial.print("/"); Serial.print(steps); Serial.print(": Sending TEMP DOWN on pin "); Serial.println(kIrLedPin);
        #if DUAL_UNIT_MODE
        Serial.println("  -> Unit 1 (pin 4)");
        irsend.sendRaw(down_buf, rawTempDownLen, kFrequency);
        delay(100);
        Serial.println("  -> Unit 2 (pin 5)");
        irsend2.sendRaw(down_buf, rawTempDownLen, kFrequency);
        #else
        irsend.sendRaw(down_buf, rawTempDownLen, kFrequency);
        #endif
        Serial.println("TEMP DOWN sent");
      }

      if (i < steps - 1) {
        delay(20000);
      }
    }

    lastIRSendMillis = millis();
    Serial.print("Humidity automation complete. Target temp: "); Serial.println(targetTemp, 1);
    // Log humidity automation event to Firebase history
    logAutomationEventToHistory();
  } else {
    Serial.println("Target temp matches current temp, no action needed");
  }
}

// Schedule Efficiency Functions - REMOVED (AC Power Automation disabled)