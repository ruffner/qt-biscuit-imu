#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <String>
#include <WiFiUdp.h>
#include <Wire.h>

#define COMMAND_ACK           "GOTCHA"
#define COMMAND_CONFIGURE     "CONFIGURE"
#define COMMAND_STREAM_START  "STREAM_START"
#define COMMAND_STREAM_STOP   "STREAM_STOP"

#define WIFI_SSID "wiltj"
#define WIFI_PASS "82417451"

#define SERVER_IP   "192.168.1.22"
#define SERVER_PORT 1980

#define PERIOD_REC 200
#define PERIOD_IMU 100    // PERIOD IN MS TO SEND IMU DATA
#define PERIOD_LED 1000   // PERIOD IN MS OF HEARTBEAT LED

#define PIN_LED (12)
#define PIN_BNO055_RESET (13)

#define EEPROM_ADDR_IMU_PERIOD 1 // 2 BYTES LONG

#define EEPROM_ADDR_CONFIG 3

#define OPTION_QUAT   1
#define OPTION_EULER  2
#define OPTION_ACCEL  4
#define OPTION_GYRO   8
#define OPTION_MAG    16

uint16_t imuPeriod = PERIOD_IMU;

uint8_t streamConfig = OPTION_QUAT;

unsigned long lastImu, lastBlink, lastCheck;

// UDP CONNECTION
WiFiUDP udp;

// BNO055 ABSOLUTE ORIENTATION SENSPR
Adafruit_BNO055 bno055 = Adafruit_BNO055();

// FOR DATA RECEPTION
char packet[64];

// FOR IMU DATA SENDING
StaticJsonBuffer<512> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();
JsonObject& quatData = root.createNestedObject("quaternion");

String targetIp = SERVER_IP;

bool isAssociated = 0;

void setup() {
  // SERIAL INIT
  Serial.begin(115200);

  // PIN CONFIG
  pinMode(PIN_LED, OUTPUT);

  EEPROM.begin(512);
  // CHECK IF EEPROM HAS BEEN USED
  if ( EEPROM.read(0) != 0xAA ) {
    // STORE INITIAL STATES (FIRST RUN ONLY)
    EEPROM.write(0, 0xAA);
    saveImuPeriod();
    saveConfig();
  } else {
    // LOAD IN SAVED STATES
    loadImuPeriod();
    loadConfig();
  }
  EEPROM.end();

  sensorInit();

  wifiInit();

  // START UDP
  udp.begin(SERVER_PORT);
  Serial.print("UDP STARTED ON PORT ");
  Serial.println(udp.localPort());

  lastImu = millis();
  lastBlink = millis();
}

void loop() {

  if ( millis() - lastBlink > PERIOD_LED ) {
    lastBlink = millis();
    digitalWrite( PIN_LED, !digitalRead(PIN_LED) );
  }

  if ( millis() - lastImu > imuPeriod  && isAssociated) {
    lastImu = millis();
    streamData();
  }

  if ( millis() - lastCheck > PERIOD_REC ) {
    lastCheck = millis();
    handlePacket();
  }
}

void streamData() {

//  if ( streamConfig & OPTION_QUAT ){
//    JsonObject& quatData = root.createNestedObject("quaternion");
//    imu::Quaternion quat = bno055.getQuat();

//  }

//  if( streamConfig & OPTION_EULER ){
//    JsonObject& eulerData = root.createNestedObject("euler");
//    imu::Vector<3> euler = bno055.getVector(Adafruit_BNO055::VECTOR_EULER);
//    eulerData["x"] = (float)euler.x();
//    eulerData["y"] = (float)euler.y();
//    eulerData["z"] = (float)euler.z();
//  }

    root["time"] = lastImu;

    imu::Quaternion quat = bno055.getQuat();
    quatData["w"] = (float)quat.w();
    quatData["x"] = (float)quat.x();
    quatData["y"] = (float)quat.y();
    quatData["z"] = (float)quat.z();
  
  String s = "";
  root.printTo(s);

  // SEND OVER UP CONNECTION
  udpSend(s);
}

void handlePacket() {
  // CHECK FOR RECEPTION
  int cb = udp.parsePacket();
  if (!cb) {
    return;
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packet, cb); // read the packet into the buffer
    String cmd(packet);

    Serial.print("packet data: ");

    if ( cmd.substring(0,strlen(COMMAND_CONFIGURE)).equals(String(COMMAND_CONFIGURE)) ) {
      Serial.println("SENDING IP ADDRESS FOR CONFIGURATION");
      isAssociated = 0;
      String newIp = cmd.substring(strlen(COMMAND_CONFIGURE), cb);
      Serial.print("ASSIGNING NEW IP ADDR FOR STREAM TARGET: ");
      Serial.println(newIp);
      targetIp = newIp;
      udpSend(COMMAND_ACK);
    } else if ( cmd.substring(0, strlen(COMMAND_STREAM_START)).equals(String(COMMAND_STREAM_START)) ) {
      Serial.print("STARTING STREAM");
      isAssociated = 1;
    } else if ( cmd.substring(0, strlen(COMMAND_STREAM_STOP)).equals(String(COMMAND_STREAM_STOP)) ) {
      Serial.println("STOPPING_STREAM");
      isAssociated = 0;
    }
  }
}

void sensorInit() {
  // BNO055 INIT
  Serial.print("INITIALIZING BNO055");
  if ( !bno055.begin() ) {
    do {
      Serial.print(".");
      delay(200);
      digitalWrite( PIN_LED, !digitalRead(PIN_LED) );
    } while (!bno055.begin());
  }
  Serial.println();
}

void wifiInit() {
  // WIFI INIT
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("INITIALIZING WIFI");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(400);
    digitalWrite( PIN_LED, !digitalRead(PIN_LED) );
  }

  Serial.print("\nDONE. IP ADDRESS: ");
  Serial.println(WiFi.localIP());

  // START AVAHI SERVICE
  if (!MDNS.begin("biscuit")) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS STARTED");
}

void saveImuPeriod() {
  EEPROM.write((uint8_t)(0xFF & (imuPeriod >> 8)), EEPROM_ADDR_IMU_PERIOD);
  EEPROM.write((uint8_t)(0xFF & (imuPeriod)), EEPROM_ADDR_IMU_PERIOD + 1);
  EEPROM.commit();
}

void saveConfig() {
  EEPROM.write(streamConfig, EEPROM_ADDR_CONFIG);
  EEPROM.commit();
}

void loadImuPeriod() {
  uint16_t p = EEPROM.read(EEPROM_ADDR_IMU_PERIOD) << 8;
  p |= EEPROM.read(EEPROM_ADDR_IMU_PERIOD + 1);
  imuPeriod = p;
}

void loadConfig() {
  streamConfig = EEPROM.read(EEPROM_ADDR_CONFIG);
}

void enableStream(uint8_t s) {
  if ( streamConfig & s ) {
    return;
  } else {
    streamConfig |= s;
  }
}

void disableStream(uint8_t s) {
  if ( streamConfig & ~s ) {
    streamConfig &= ~s;
  } else {
    return;
  }
}

void udpSend(String &s) {
  udp.beginPacket(targetIp.c_str(), SERVER_PORT);
  udp.write(s.c_str(), s.length());
  udp.endPacket();
}

void udpSend(const char *s) {
  udp.beginPacket(targetIp.c_str(), SERVER_PORT);
  udp.write(s, strlen(s));
  udp.endPacket();
}

