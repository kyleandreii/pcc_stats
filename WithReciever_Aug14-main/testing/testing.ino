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

const uint16_t kIrLedPin = 4;
const uint16_t kIrLedPin2 = 5;
const uint16_t kIrRecvPin = 14;
IRsend irsend(kIrLedPin);
IRsend irsend2(kIrLedPin2);
IRrecv irrecv(kIrRecvPin);
decode_results irResults;
const uint16_t kFrequency = 38;

// --- Koppel AC RAW Data (ON/OFF only) ---
const uint16_t PROGMEM rawOn[149] = {4850, 2500, 382, 428, 358, 406, 378, 424, 362, 956, 358, 924, 380, 426, 360, 404, 406, 388, 384, 422, 360, 946, 384, 402, 356, 442, 356, 426, 358, 948, 358, 404, 382, 404, 392, 424, 360, 426, 360, 426, 384, 930, 384, 400, 362, 944, 358, 426, 360, 416, 380, 948, 358, 428, 356, 428, 360, 436, 358, 424, 360, 924, 406, 400, 362, 416, 380, 426, 360, 424, 360, 428, 358, 438, 356, 430, 358, 426, 358, 406, 404, 414, 384, 400, 360, 426, 360, 424, 358, 438, 360, 426, 360, 428, 358, 402, 382, 436, 362, 424, 382, 898, 382, 426, 360, 436, 360, 426, 360, 924, 382, 390, 400, 428, 358, 924, 406, 404, 358, 404, 380, 956, 358, 428, 360, 402, 382, 428, 358, 436, 360, 424, 360, 426, 384, 404, 358, 436, 360, 940, 364, 924, 380, 946, 358, 428, 358, 21474, 4852};
const uint16_t PROGMEM rawOff[149] = {4908, 2466, 416, 334, 452, 372, 412, 372, 414, 924, 392, 914, 390, 372, 414, 372, 414, 404, 392, 394, 390, 914, 390, 298, 488, 404, 392, 378, 408, 914, 390, 394, 390, 406, 390, 396, 390, 372, 414, 300, 486, 404, 390, 914, 390, 914, 390, 370, 416, 404, 390, 914, 392, 394, 390, 394, 392, 404, 392, 394, 390, 914, 390, 394, 392, 404, 392, 394, 390, 396, 390, 396, 390, 406, 390, 396, 390, 396, 390, 396, 390, 406, 392, 396, 390, 396, 390, 396, 390, 406, 390, 396, 390, 396, 390, 396, 390, 406, 390, 394, 392, 914, 390, 396, 370, 426, 388, 396, 366, 938, 390, 396, 390, 384, 390, 936, 390, 372, 412, 372, 412, 926, 390, 372, 414, 370, 414, 370, 416, 380, 416, 370, 414, 370, 416, 370, 416, 380, 416, 370, 416, 370, 416, 370, 416, 370, 416, 21416, 4908};
const uint16_t PROGMEM rawTempUp[75] = {0, 2, 8994, 4488, 556, 546, 558, 1678, 558, 546, 556, 546, 556, 1680, 556, 1676, 556, 546, 556, 1680, 558, 1676, 556, 546, 556, 1678, 556, 1678, 556, 548, 554, 550, 554, 1678, 556, 546, 554, 550, 530, 1702, 554, 550, 530, 1704, 556, 548, 552, 548, 558, 1678, 556, 1678, 556, 1676, 556, 548, 554, 1680, 554, 548, 556, 1678, 556, 1680, 554, 548, 532, 572, 558, 39932, 8968, 2254, 556, 0};
const uint16_t PROGMEM rawTempDown[75] = {0, 2, 8968, 4514, 554, 550, 554, 1680, 556, 548, 530, 572, 554, 1680, 556, 1676, 556, 548, 554, 1680, 530, 1704, 554, 548, 556, 1680, 530, 1704, 554, 548, 554, 548, 556, 1678, 554, 550, 554, 548, 554, 1680, 554, 550, 554, 550, 552, 1680, 556, 550, 528, 1704, 556, 1680, 554, 1680, 554, 550, 556, 1680, 552, 1680, 558, 546, 528, 1704, 554, 550, 554, 548, 554, 39934, 8968, 2258, 554, 0};

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

Preferences preferences;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 26
#define DHTTYPE DHT11
#define LED_PIN 27
#define RELAY_PIN 25
DHT dht(DHTPIN, DHTTYPE);

float minTemp = 20.0;
float maxTemp = 30.0;
unsigned long sendDataPrevMillis = 0;

bool automationEnabled = false;

float MAX_HUMIDITY = 65.0;
float MIN_HUMIDITY = 45.0;
const unsigned long AUTOMATION_INTERVAL_MS = 300000; // 5 minutes between automation checks
const unsigned long STEP_PRESS_DELAY_MS = 400;
const unsigned long IR_SEND_COOLDOWN_MS = 60000; // 1 minute cooldown between IR sends
const unsigned long SERIAL_LOG_THROTTLE_MS = 5000; // 5 seconds between serial logs

unsigned long lastAutomationCheckMillis = 0;
unsigned long lastIRSendMillis = 0;
unsigned long lastSerialLogMillis = 0;

void handleACCommand(String cmd);
void updateOLED(float currentTemp);
void runAutomation(float temp, float humidity);
void handleIRReceiver();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== PCC S.T.A.T.S. System Starting ===");

  // Initialize Preferences for persistent storage
  preferences.begin("pcc-automation", false);
  // Load from Preferences as fallback, but will sync with Firebase
  automationEnabled = preferences.getBool("automationEnabled", false);
  Serial.println("Loaded automation enabled state from Preferences: " + String(automationEnabled));
  
  Wire.begin(OLED_SDA, OLED_SCL);
  irsend.begin();
  irsend2.begin();
  irrecv.enableIRIn();
  Serial.println("[IR Receiver] IR receiver enabled on pin " + String(kIrRecvPin));

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

  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  config.signer.tokens.legacy_token = DATABASE_SECRET;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.RTDB.beginStream(&stream, "/" + String(ROOM_ID))) {
  } else {
  }

  // Sync automation enabled state from Firebase on startup
  Firebase.RTDB.getBool(&fbdo, "/" + String(ROOM_ID) + "/automation/enabled");
  if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    automationEnabled = fbdo.boolData();
    preferences.putBool("automationEnabled", automationEnabled);
    Serial.println("Synced automation enabled state from Firebase: " + String(automationEnabled));
  }


  FirebaseJson json;
  json.set(".sv", "timestamp");
  Firebase.RTDB.setJSON(&fbdo, "/" + String(ROOM_ID) + "/last_seen", &json);
  Firebase.RTDB.setBool(&fbdo, "/" + String(ROOM_ID) + "/online", true);
  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/device_room_id", ROOM_ID);
  // Don't overwrite Firebase currentKnownTemp on startup - use Firebase as source of truth
}

void loop() {
  static unsigned long lastLoopLog = 0;
  if (millis() - lastLoopLog > 5000) {
    Serial.println("[Loop] ESP32 is running, millis: " + String(millis()));
    lastLoopLog = millis();
  }

  if (Firebase.ready() && Firebase.RTDB.readStream(&stream)) {
    if (stream.streamAvailable()) {
      String path = stream.dataPath();
      
      if (path == "/thresholds/minTemp" || path == "/minTemp") {
        minTemp = stream.floatData();
      } else if (path == "/thresholds/maxTemp" || path == "/maxTemp") {
        maxTemp = stream.floatData();
      } else if (path == "/automation/enabled" || path.indexOf("automation/enabled") >= 0) {
        bool newEnabledState = stream.boolData();
        if (newEnabledState != automationEnabled) {
          automationEnabled = newEnabledState;
          preferences.putBool("automationEnabled", automationEnabled);
          Serial.println("Automation " + String(automationEnabled ? "ENABLED" : "DISABLED"));
        }
      } else if (path == "/automation" || path.indexOf("automation") >= 0) {
        FirebaseJson &json = stream.jsonObject();
        FirebaseJsonData jsonData;
        if (json.get(jsonData, "enabled")) {
          bool newEnabledState = jsonData.boolValue;
          if (newEnabledState != automationEnabled) {
            automationEnabled = newEnabledState;
            preferences.putBool("automationEnabled", automationEnabled);
            Serial.println("Automation " + String(automationEnabled ? "ENABLED" : "DISABLED"));
          }
        }
      } else if (path == "/ac_command" || path.endsWith("ac_command")) {
        handleACCommand(stream.stringData());
      }
    }
  }

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Live temperature and humidity display every 5 seconds
  static unsigned long lastSensorLogMillis = 0;
  if (!isnan(t) && !isnan(h) && millis() - lastSensorLogMillis > 5000) {
    Serial.print("Live - Temp: "); Serial.print(t, 1); Serial.print("°C, Humidity: "); Serial.print(h, 1); Serial.println("%");
    lastSensorLogMillis = millis();
  }

  // Safety check for DHT sensor
  if (!isnan(t)) {
    bool isAlarm = (t < minTemp || t > maxTemp);
    digitalWrite(LED_PIN, isAlarm ? HIGH : LOW);
    digitalWrite(RELAY_PIN, (t < minTemp) ? LOW : HIGH);
  }

  updateOLED(t);

  handleIRReceiver();

  if (!isnan(t) && !isnan(h)) {
    runAutomation(t, h);
  }

  // Send data to Firebase every 10 seconds
  if (millis() - sendDataPrevMillis > 10000) {
    sendDataPrevMillis = millis();
    FirebaseJson updateData;
    updateData.add("temperature", t);
    updateData.add("humidity", h);
    updateData.add("status", (!isnan(t) && (t < minTemp || t > maxTemp)) ? "ALARM" : "NORMAL");
    Firebase.RTDB.updateNode(&fbdo, "/" + String(ROOM_ID), &updateData);

    // Heartbeat
    FirebaseJson json;
    json.set(".sv", "timestamp");
    Firebase.RTDB.setJSON(&fbdo, "/" + String(ROOM_ID) + "/last_seen", &json);

    // Store historical data for analytics
    if (!isnan(t) && !isnan(h)) {
      // Get current date and hour
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
      char dateStr[11]; // YYYY-MM-DD
      char hourStr[3];  // HH
      strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);
      strftime(hourStr, sizeof(hourStr), "%H", timeinfo);
      
      String historyPath = "/history/" + String(ROOM_ID) + "/" + String(dateStr) + "/" + String(hourStr);
      FirebaseJson historyData;
      historyData.add("temperature", t);
      historyData.add("humidity", h);
      Firebase.RTDB.setJSON(&fbdo, historyPath.c_str(), &historyData);
    }
  }
}

void updateOLED(float currentTemp) {
  display.clearDisplay();

  // Design: Inverted Header for PCC S.T.A.T.S. look
  display.fillScreen(SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);

  // Top bar: Thresholds
  display.setTextSize(1);
  display.setCursor(5, 5);
  display.printf("SET: %.0fC - %.0fC", minTemp, maxTemp);

  // Divider line
  display.drawFastHLine(0, 15, 128, SSD1306_BLACK);

  // Main Temperature
  display.setCursor(20, 30);
  display.setTextSize(2);
  if (isnan(currentTemp)) {
    display.print("SENSOR ERR");
  } else {
    display.print(currentTemp, 1);
    display.print(" C");
  }

  // Footer status
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

void handleACCommand(String cmd) {
  if (cmd == "IDLE" || cmd == "") {
    return;
  }

  Serial.println("AC Command: " + cmd);

  if (cmd == "ON") {
    Serial.println("AC Command: POWER ON");
    uint16_t on_buf[149];
    memcpy_P(on_buf, rawOn, sizeof(rawOn));
    irsend.sendRaw(on_buf, 149, kFrequency);
  } else if (cmd == "OFF") {
    Serial.println("AC Command: POWER OFF");
    uint16_t off_buf[149];
    memcpy_P(off_buf, rawOff, sizeof(rawOff));
    irsend.sendRaw(off_buf, 149, kFrequency);
  } else if (cmd == "TEMP_UP") {
    Serial.println("AC Command: TEMPERATURE UP");
    uint16_t up_buf[75];
    memcpy_P(up_buf, rawTempUp, sizeof(rawTempUp));
    irsend.sendRaw(up_buf, 75, kFrequency);
  } else if (cmd == "TEMP_DOWN") {
    Serial.println("AC Command: TEMPERATURE DOWN");
    uint16_t down_buf[75];
    memcpy_P(down_buf, rawTempDown, sizeof(rawTempDown));
    irsend.sendRaw(down_buf, 75, kFrequency);
  }

  Firebase.RTDB.setString(&fbdo, "/" + String(ROOM_ID) + "/ac_command", "IDLE");
  Firebase.RTDB.setBool(&fbdo, "/" + String(ROOM_ID) + "/ac", (cmd == "ON"));
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
  // Check if automation is enabled - early exit if disabled
  if (!automationEnabled) {
    return;
  }

  // Check if it's time for automation check
  if (millis() - lastAutomationCheckMillis < AUTOMATION_INTERVAL_MS) {
    return;
  }
  lastAutomationCheckMillis = millis();
  
  Serial.print("Automation Check - Humidity: "); Serial.print(humidity, 1); Serial.print("%, Min Temp: "); Serial.print(minTemp, 1); Serial.print("°C, Max Temp: "); Serial.print(maxTemp, 1); Serial.println("°C");

  // Determine target temperature based on humidity using Firebase thresholds
  // High humidity = occupied, raise temp to max threshold
  // Low humidity = not occupied, lower temp to min threshold
  float targetTemp;
  
  if (humidity > MAX_HUMIDITY) {
    targetTemp = maxTemp;
    Serial.println("⚠️ OCCUPIED: Humidity is above maximum!");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.print("% > "); Serial.print(MAX_HUMIDITY); Serial.print("%, raising temp to max threshold: "); Serial.println(targetTemp, 1);
  } else if (humidity < MIN_HUMIDITY) {
    targetTemp = minTemp;
    Serial.println("⚠️ NOT OCCUPIED: Humidity is below minimum!");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.print("% < "); Serial.print(MIN_HUMIDITY); Serial.print("%, lowering temp to min threshold: "); Serial.println(targetTemp, 1);
  } else {
    Serial.println("Humidity within normal range, no action needed");
    return;
  }

  Serial.print("Target temp: "); Serial.print(targetTemp, 1); Serial.print("°C, Current sensor temp: "); Serial.print(temp, 1); Serial.println("°C");

  // If target is different from current sensor temp, send step commands
  if (targetTemp != temp) {
    // Check cooldown to prevent repetitive IR sends
    unsigned long timeSinceLastIR = millis() - lastIRSendMillis;
    if (timeSinceLastIR < IR_SEND_COOLDOWN_MS) {
      unsigned long cooldownRemaining = (IR_SEND_COOLDOWN_MS - timeSinceLastIR) / 1000;
      Serial.print("Cooldown active, "); Serial.print(cooldownRemaining); Serial.println(" seconds remaining");
      return;
    }
    
    float difference = targetTemp - temp;
    int steps = (int)round(abs(difference));
    Serial.print("Adjusting temp by "); Serial.print(difference, 1); Serial.print("°C ("); Serial.print(steps); Serial.println(" steps)");
    Serial.println("Starting IR transmission...");
    
    for (int i = 0; i < steps; i++) {
      if (difference > 0) {
        uint16_t up_buf[75];
        memcpy_P(up_buf, rawTempUp, sizeof(rawTempUp));
        Serial.print("Step "); Serial.print(i + 1); Serial.print("/"); Serial.print(steps); Serial.print(": Sending TEMP UP on pin "); Serial.println(kIrLedPin);
        irsend.sendRaw(up_buf, 75, kFrequency);
        Serial.println("TEMP UP sent");
      } else {
        uint16_t down_buf[75];
        memcpy_P(down_buf, rawTempDown, sizeof(rawTempDown));
        Serial.print("Step "); Serial.print(i + 1); Serial.print("/"); Serial.print(steps); Serial.print(": Sending TEMP DOWN on pin "); Serial.println(kIrLedPin);
        irsend.sendRaw(down_buf, 75, kFrequency);
        Serial.println("TEMP DOWN sent");
      }
      
      if (i < steps - 1) {
        delay(20000); // 20 seconds between each IR transmission
      }
    }
    
    lastIRSendMillis = millis();
    Serial.print("Automation complete. Target temp: "); Serial.println(targetTemp, 1);
  } else {
    Serial.println("Target temp matches current temp, no action needed");
  }
}