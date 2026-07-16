#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>

// ==========================================
// 1. CẤU HÌNH PINOUT & MAPPING
// ==========================================
#define RXp2 16
#define TXp2 17

// --- BẢNG MAPPING PIN CẢM BIẾN ---
const int PIN_TEMP = 1;
const int PIN_PH = 2;
const int PIN_TDS = 3;
const int PIN_WATER = 5;
// Các chân Flow, Relay, PZEM đã được loại bỏ

// ==========================================
// 2. CẤU HÌNH MQTT MẶC ĐỊNH (HiveMQ TLS)
// ==========================================

char mqtt_server[60] = "8263ee975ee9413ca344b39c068b3dbc.s1.eu.hivemq.cloud";
char mqtt_port[6] = "8883";
char mqtt_user[20] = "amazing_iot";
char mqtt_pass[30] = "Amazing_iot2025";
char topic_data[50] = "iras-rag/telemetry/+";
char topic_control[50] = "iras-rag/command/+";
char topic_system_config[50] = "reaper_iot/system/config";

// Biến toàn cục
bool shouldSaveConfig = false;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiTimeout = 60000;

WiFiClientSecure espClient; // Sử dụng WiFiClientSecure cho TLS/SSL
PubSubClient client(espClient);
WiFiManager wm;
WebSocketsServer webSocket = WebSocketsServer(81);

// ==========================================
// 3. HÀM HỖ TRỢ (FILESYSTEM & LƯU CONFIG)
// ==========================================
void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void loadConfig()
{
  if (SPIFFS.begin(true))
  {
    // FORCE RESET: Xóa cấu hình cũ của đồ án trước lưu trong bộ nhớ mạch
    if (SPIFFS.exists("/config.json")) {
      SPIFFS.remove("/config.json");
      Serial.println(">>> Deleted old config.json!");
    }
    
    if (SPIFFS.exists("/config.json"))
    {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        JsonDocument json;
        if (!deserializeJson(json, buf.get()))
        {
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
        }
      }
    }
  }
}

void saveConfig()
{
  JsonDocument json;
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_user"] = mqtt_user;
  json["mqtt_pass"] = mqtt_pass;
  File configFile = SPIFFS.open("/config.json", "w");
  if (configFile)
  {
    serializeJson(json, configFile);
    configFile.close();
  }
}

// ==========================================
// 5. MQTT LOGIC & ĐÓNG GÓI JSON
// ==========================================
void publishSystemConfig()
{
  if (!client.connected())
    return;
  delay(200);

  String mac = WiFi.macAddress();
  mac.toUpperCase();

  JsonDocument doc;
  doc["type"] = "system_config";
  doc["mqtt_server"] = mqtt_server;
  doc["mqtt_port"] = mqtt_port;
  // BẢO MẬT: KHÔNG GỬI USER/PASS LÊN MQTT
  doc["topic_data"] = "iras-rag/telemetry/" + mac;
  doc["topic_control"] = "iras-rag/command/" + mac;
  doc["mac_address"] = mac;
  doc["timestamp"] = millis();

  String output;
  serializeJson(doc, output);
  client.publish(topic_system_config, output.c_str(), true);
}

void sendTelemetryToBackend(String rawJson)
{
  StaticJsonDocument<512> stmDoc;
  if (!deserializeJson(stmDoc, rawJson))
  {
    StaticJsonDocument<1024> beDoc;
    String mac = WiFi.macAddress();
    mac.toUpperCase();
    beDoc["mac"] = mac;

    JsonArray readings = beDoc.createNestedArray("readings");

    if (stmDoc.containsKey("temp"))
    {
      JsonObject r = readings.createNestedObject();
      r["pin"] = PIN_TEMP;
      r["val"] = stmDoc["temp"];
    }
    if (stmDoc.containsKey("pH"))
    {
      JsonObject r = readings.createNestedObject();
      r["pin"] = PIN_PH;
      r["val"] = stmDoc["pH"];
    }
    if (stmDoc.containsKey("tds"))
    {
      JsonObject r = readings.createNestedObject();
      r["pin"] = PIN_TDS;
      r["val"] = stmDoc["tds"];
    }
    if (stmDoc.containsKey("water"))
    {
      JsonObject r = readings.createNestedObject();
      r["pin"] = PIN_WATER;
      r["val"] = stmDoc["water"];
    }

    String finalJson;
    serializeJson(beDoc, finalJson);

    String topic = "iras-rag/telemetry/" + mac;
    client.publish(topic.c_str(), finalJson.c_str());
    webSocket.broadcastTXT(finalJson);
    Serial.println("Sent BE: " + finalJson);
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  for (int i = 0; i < length; i++)
    message += (char)payload[i];
  Serial.println("Recv BE cmd: " + message);

  if (message == "RESET_CONFIG")
  {
    wm.resetSettings();
    if (SPIFFS.exists("/config.json"))
      SPIFFS.remove("/config.json");
    ESP.restart();
  }

  // --- XỬ LÝ LỆNH JSON ---
  StaticJsonDocument<256> cmdDoc;
  if (!deserializeJson(cmdDoc, message))
  {
    if (cmdDoc.containsKey("cmd") && cmdDoc.containsKey("pin"))
    {
      int pin = cmdDoc["pin"];
      String cmd = cmdDoc["cmd"];

      String stmCommand = "CMD:" + String(pin) + ":" + cmd;
      Serial2.println(stmCommand);
      Serial.println("Forward STM32: " + stmCommand);
    }
  }
  else
  {
    // Nếu không phải JSON (ví dụ lệnh Calibration), cứ đẩy thẳng xuống STM32
    Serial2.println(message);
  }
}

void reconnect()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  if (!client.connected())
  {
    // BUG FIX QUAN TRỌNG: Phải gọi espClient.stop() trước khi kết nối lại.
    // Nếu không, WiFiClientSecure sẽ cố tái sử dụng phiên TLS cũ đã chết
    // khiến TLS handshake thất bại âm thầm và ESP32 không gửi được data nào lên HiveMQ.
    espClient.stop();
    delay(100); // Chờ socket đóng hẳn

    String mac = WiFi.macAddress();
    mac.toUpperCase();
    String cleanMac = mac;
    cleanMac.replace(":", "");
    String clientId = "ESP32Gateway-" + cleanMac;

    Serial.println("Connecting to MQTT Server: " + String(mqtt_server) + " at port " + String(mqtt_port) + "...");
    Serial.println("Using TLS/SSL (WiFiClientSecure)");
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println(">>> MQTT CONNECTED SUCCESSFULLY!");
      String topic_control_dynamic = "iras-rag/command/" + mac;
      client.subscribe(topic_control_dynamic.c_str());
      publishSystemConfig();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ==========================================
// 6. SETUP & LOOP CHÍNH
// ==========================================
void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXp2, TXp2);
  loadConfig();

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 60);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT User", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Pass", mqtt_pass, 30);

  wm.setSaveConfigCallback(saveConfigCallback);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);

  if (!wm.autoConnect("ESP32-SmartGarden", "12345678"))
    ESP.restart();

  if (shouldSaveConfig)
  {
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_pass, custom_mqtt_pass.getValue());
    saveConfig();
    ESP.restart();
  }

  if (MDNS.begin("esp32-gateway"))
    MDNS.addService("ws", "tcp", 81);

  espClient.setInsecure(); // Bỏ qua kiểm tra chứng chỉ SSL/TLS (Bắt buộc cho HiveMQ TLS)
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
  client.setBufferSize(1024);

  webSocket.begin();
  Serial.println("System Ready - Connected to HiveMQ (TLS)");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    unsigned long now = millis();
    if (now - lastWiFiCheck > wifiTimeout)
    {
      wm.setConfigPortalTimeout(180);
      if (!wm.startConfigPortal("ESP32-ReconnectWifi", "12345678"))
        ESP.restart();
      lastWiFiCheck = now;
    }
  }
  else
  {
    lastWiFiCheck = millis();
    if (!client.connected())
      reconnect();
    client.loop();
    webSocket.loop();

    if (Serial2.available())
    {
      String inputString = Serial2.readStringUntil('\n');
      inputString.trim();
      
      if (inputString.length() > 0) 
      {
        Serial.println(">>> RX from STM32: " + inputString);
      }

      if (inputString.length() > 0 && inputString.charAt(0) == '{')
      {
        sendTelemetryToBackend(inputString);
      }
    }
  }
}