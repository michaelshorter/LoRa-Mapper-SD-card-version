# LoRa Coverage Mapper

A DIY LoRa/Meshtastic coverage mapping tool that plots signal success and failure on a real map. Built using an Arduino Nano ESP32, a Heltec LoRa V3 running Meshtastic firmware, a GPS module, and an SD card for field logging. Results are visualised using a standalone Processing sketch that runs without any Arduino connected.

---

## How It Works

The sender unit automatically pings a fixed target node every 20 seconds over a private Meshtastic channel. The target node replies with an ACK. Based on whether the ACK is received within a timeout window, the result is logged as **Direct** (green) or **Failed** (red) alongside the current GPS coordinates, written directly to an SD card. Back home, copy the log file to your PC and open it in the Processing sketch to see all results plotted as coloured dots on your map.

```
[Heltec V3 Sender] --LoRa Meshtastic--> [Heltec V3 Target]
        |                                        |
    UART Serial                             UART Serial
        |                                        |
[Arduino Nano ESP32]                     [Arduino (Target)]
        |
    SPI Bus
        |
  [SD Card Module]
        |
   [log.csv file]
        |
  (copy to PC)
        |
[Processing Sketch]
        |
  Map with dots
```

---

## Hardware Required

### Sender Unit
- Arduino Nano ESP32
- Heltec LoRa V3 (running Meshtastic firmware)
- GPS module (UART, 9600 baud)
- Micro SD card module (SPI, 3.3V)
- Micro SD card (FAT32 formatted, 32GB or less)
- 2x LEDs — GPS fix indicators (green/red)
- 1x LED — Status (waiting for ACK)
- 2x LEDs — Result indicators (green = Direct, red = Failed)
- Resistors (220–330Ω for each LED)

### Target Unit
- Any Arduino
- Heltec LoRa V3 (running Meshtastic firmware)
- 1x LED (ping received indicator)

### PC / Base Station
- Any computer running Processing 4

---

## Wiring — Sender (Arduino Nano ESP32)

| Arduino Pin | Function            |
|-------------|---------------------|
| 4           | SD CS               |
| 5           | Heltec TX           |
| 7           | Heltec RX           |
| 9           | LED — Direct (Green)|
| 10          | LED — Failed (Red)  |
| 11          | SD MOSI             |
| 12          | SD MISO             |
| 13          | SD SCK              |
| 18          | LED — GPS Fix (Green)|
| 19          | LED — No GPS Fix (Red)|
| 20          | LED — Status        |
| 21          | GPS RX              |
| 22          | GPS TX              |

### SD Card Module
| SD Module | Arduino Nano ESP32 |
|-----------|-------------------|
| VCC       | 3.3V              |
| GND       | GND               |
| MISO      | D12               |
| MOSI      | D11               |
| SCK       | D13               |
| CS        | D4                |

### Target — Arduino

| Arduino Pin | Function              |
|-------------|-----------------------|
| 4           | Heltec RX             |
| 5           | Heltec TX             |
| 13          | LED — Ping received   |

All LEDs wired with a 220–330Ω resistor between pin and GND.

---

## Meshtastic Configuration

Both Heltec V3 boards must be running Meshtastic firmware and configured as follows:

### Both Nodes
- **Channel 0** renamed to a private channel name known only to your two nodes
- Both nodes must share the same channel name and PSK (pre-shared key)

### Serial Module (both nodes)
- **Enabled:** Yes
- **Mode:** Text Message
- **Echo:** Off
- **RX/TX pins:** Match your wiring (see above)

Configure via the Meshtastic app under **Settings → Module Config → Serial**.

---

## SD Card Setup

- Format the card as **FAT32**
- Maximum card size **32GB**
- Cards larger than 32GB will not work with the Arduino SD library
- Use the official **SD Card Formatter** tool if Windows won't offer FAT32 as an option for larger cards

Each power cycle creates a new uniquely named log file (`log.csv`, `log1.csv`, `log2.csv` etc) so previous data is never overwritten.

---

## Software Setup

### Arduino Libraries Required
Install these via **Tools → Manage Libraries** in the Arduino IDE:
- `TinyGPSPlus`
- `SdFat` by Bill Greiman
- `ArduinoBLE` (BLE version only)

### Sketches
- Upload `sender_sd/sender_sd.ino` to the sender Arduino Nano ESP32
- Upload `target/target.ino` to the target Arduino

### Processing
1. Download and install [Processing 4](https://processing.org/)
2. Place your map image and log file in the sketch's `data/` folder:
```
coverage_mapper_standalone/
├── coverage_mapper_standalone.pde
└── data/
    ├── map.png
    └── log.csv
```
3. Update the map boundary coordinates in the sketch to match your map:
```java
float latMin = 53.377366;
float latMax = 53.389229;
float lonMin = -2.949590;
float lonMax = -2.916461;
```
4. Update the filename if needed:
```java
String dataFile = "log.csv";
```
5. Run the sketch — dots will appear automatically on startup

---

## Processing Sketch Controls

| Key       | Action                          |
|-----------|---------------------------------|
| L         | Reload the CSV file             |
| Spacebar  | Add a random test dot           |
| C         | Clear all dots from screen      |

---

## CSV Log Format

The SD card log file contains one entry per successful ping with a valid GPS fix:

```
Result,Latitude,Longitude
Direct,53.383100,-2.930200
Failed,53.381200,-2.928400
```

- `Direct` — ACK received from target within 15 second timeout
- `Failed` — No ACK received within timeout
- Entries without a GPS fix are not logged

The Processing sketch handles both comma and tab separated files automatically.

---

## Map Image

The map image should be a flat projection exported from a mapping tool with known boundary coordinates. The four boundary values (`latMin`, `latMax`, `lonMin`, `lonMax`) must correspond exactly to the edges of your image.

Recommended free tools for exporting map images with known coordinates:
- [JOSM](https://josm.openstreetmap.de/)
- [Overpass Turbo](https://overpass-turbo.eu/)

---

## File Structure

```
lora-coverage-mapper/
│
├── README.md
│
├── sender_sd/
│   └── sender_sd.ino           # Sender — GPS + ping + SD card logging
│
├── sender_ble/
│   └── sender_ble.ino          # Sender — GPS + ping + BLE to iPhone (alternative)
│
├── target/
│   └── target.ino              # Target — listens and ACKs
│
└── processing/
    ├── coverage_mapper/
    │   ├── coverage_mapper.pde      # Live version — requires Arduino connected via USB
    │   └── data/
    │       └── map.png
    │
    └── coverage_mapper_standalone/
        ├── coverage_mapper_standalone.pde   # Standalone — loads from CSV file, no Arduino needed
        └── data/
            ├── map.png
            └── log.csv                      # Copy from SD card after field session
```

---

## Known Limitations

- Hop detection is not currently implemented. The system detects Direct vs Failed only. Hopped detection would require switching the Meshtastic serial module to Proto mode and parsing protobuf packets on the Arduino.
- The sender must have a valid GPS fix before any result is logged — pings during GPS_NOT_FIXED are silently skipped.
- Dots are not persisted between Processing sessions in the standalone version — they are loaded fresh from the CSV each time.

---

## Future Improvements

- [ ] Proto mode serial parsing for hop count detection (Direct / Hopped / Failed)
- [ ] Variable dot size based on RSSI signal strength
- [ ] Multiple sender units logging to one map
- [ ] Web-based map viewer instead of Processing
- [ ] Export map as image from Processing

---

## License

MIT License — free to use, modify and share.
