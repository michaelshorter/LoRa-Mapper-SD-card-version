// LoRa Coverage Mapper — Sender (SD Card version)
// Arduino Nano ESP32 + Heltec V3 (Meshtastic) + GPS + SD Card
//
// Automatically sends "ping" every 20 seconds.
// Logs result + GPS coordinates to SD card as CSV.
// Only logs when GPS fix is valid.
//
// Wiring:
//   Pin 5  -> Heltec TX
//   Pin 7  -> Heltec RX
//   Pin 9  -> Green LED (Direct result)
//   Pin 10 -> Red LED (Failed result)
//   Pin 11 -> SD MOSI
//   Pin 12 -> SD MISO
//   Pin 13 -> SD SCK
//   Pin 4  -> SD CS
//   Pin 18 -> Green LED (GPS fix)
//   Pin 19 -> Red LED (no GPS fix)
//   Pin 20 -> Status LED (waiting for ACK)
//   Pin 21 -> GPS RX
//   Pin 22 -> GPS TX

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <SdFat.h>
#include <SPI.h>

// GPS
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
const int RXPin = 21;
const int TXPin = 22;

// Heltec serial
HardwareSerial HeltecSerial(2);
const int HELTEC_RX = 7;
const int HELTEC_TX = 5;

// SD
SdFat sd;
SdFile logFile;
const int CS_PIN  = 4;
String filename   = "log.csv";
bool sdReady      = false;

// LEDs
const int ledFix    = 18;
const int ledNoFix  = 19;
const int ledStatus = 20;
const int ledDirect = 9;
const int ledFailed = 10;

// Timing
const unsigned long ACK_TIMEOUT   = 15000;
const unsigned long PING_INTERVAL = 20000;

// State
bool waitingForAck    = false;
unsigned long pingSentTime = 0;
unsigned long lastPingTime = 0;
bool lastFixState     = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  GPSSerial.begin(9600, SERIAL_8N1, RXPin, TXPin);
  HeltecSerial.begin(9600, SERIAL_8N1, HELTEC_RX, HELTEC_TX);

  pinMode(ledFix,    OUTPUT);
  pinMode(ledNoFix,  OUTPUT);
  pinMode(ledStatus, OUTPUT);
  pinMode(ledDirect, OUTPUT);
  pinMode(ledFailed, OUTPUT);

  digitalWrite(ledFix,    LOW);
  digitalWrite(ledNoFix,  HIGH);
  digitalWrite(ledStatus, LOW);
  digitalWrite(ledDirect, LOW);
  digitalWrite(ledFailed, LOW);

  // Initialise SD
  Serial.println("Initialising SD card...");
  if (!sd.begin(CS_PIN, SD_SCK_MHZ(4))) {
    Serial.println("SD card FAILED. Check wiring.");
    digitalWrite(ledFailed, HIGH); // solid red = SD error
    sdReady = false;
  } else {
    Serial.println("SD card OK.");
    sdReady = true;

    // Find unique filename — log.csv, log1.csv, log2.csv etc
    int i = 0;
    while (sd.exists(filename.c_str())) {
      i++;
      filename = "log" + String(i) + ".csv";
    }
    Serial.println("Logging to: " + filename);

    // Write header
    writeToSD("Result,Latitude,Longitude");
  }

  Serial.println("Ready. First ping in 20 seconds.");
}

void loop() {
  // Feed GPS
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  updateFixLED();

  // Auto ping every 20 seconds
  unsigned long now = millis();
  if (!waitingForAck && (now - lastPingTime >= PING_INTERVAL)) {
    sendPing();
    lastPingTime = now;
  }

  // Waiting for ACK
  if (waitingForAck) {

    // Timeout = Failed
    if (millis() - pingSentTime > ACK_TIMEOUT) {
      logResult("Failed");
      digitalWrite(ledFailed, HIGH);
      digitalWrite(ledStatus, LOW);
      waitingForAck = false;
    }

    // Read Heltec serial
    if (HeltecSerial.available()) {
      String reply = HeltecSerial.readStringUntil('\n');
      reply.trim();

      if (reply.length() == 0) return;
      if (reply.indexOf("ping") >= 0) return;

      if (reply.indexOf("ACK") >= 0) {
        logResult("Direct");
        digitalWrite(ledDirect, HIGH);
        digitalWrite(ledStatus, LOW);
        waitingForAck = false;
      }
    }
  }
}

void sendPing() {
  HeltecSerial.println("ping");
  Serial.println("Ping sent.");
  waitingForAck = true;
  pingSentTime  = millis();
  digitalWrite(ledDirect, LOW);
  digitalWrite(ledFailed, LOW);
  digitalWrite(ledStatus, HIGH);
}

void logResult(const char* result) {
  // Only log if GPS has a valid fix
  if (!gps.location.isValid()) {
    Serial.println(String(result) + " — no GPS fix, skipping.");
    return;
  }

  String lat = String(gps.location.lat(), 6);
  String lon = String(gps.location.lng(), 6);
  String csv = String(result) + "," + lat + "," + lon;

  Serial.println(csv);
  writeToSD(csv);
}

void writeToSD(String data) {
  if (!sdReady) return;

  if (logFile.open(filename.c_str(), O_WRITE | O_CREAT | O_AT_END)) {
    logFile.println(data);
    logFile.close();
  } else {
    Serial.println("SD write error!");
  }
}

void updateFixLED() {
  bool hasFix = gps.location.isValid();
  if (hasFix != lastFixState) {
    digitalWrite(ledFix,   hasFix ? HIGH : LOW);
    digitalWrite(ledNoFix, hasFix ? LOW  : HIGH);
    lastFixState = hasFix;
  }
}