#  STM32 Wireless Bootloader (FOTA System)

A complete **Firmware Over-The-Air (FOTA)** solution using:

- STM32 Bootloader (UART + CRC)
- ESP32 (WiFi ↔ UART Bridge)
- Python GUI (TCP Server + Control Panel)

This project enables remote firmware updates over WiFi without physical access to the microcontroller.

---

##  System Architecture

```
+---------------------+        TCP/IP        +------------------+        UART        +----------------------+
|   Python GUI App    |  <-----------------> |      ESP32       |  <------------->   |   STM32 Bootloader   |
|  (FOTA Controller)  |                     | (WiFi Bridge)    |                    |  (Flash + Control)   |
+---------------------+                     +------------------+                    +----------------------+
```

---

##  Features

### 🔹 STM32 Bootloader
- UART Communication Protocol
- CRC32 Verification (Hardware CRC)
- Sector Erase / Mass Erase
- Flash Programming
- Jump to Application / Address
- Read Protection Level (RDP)
- Read Option Bytes
- Bootloader Versioning

### 🔹 ESP32 Bridge
- WiFi Station Mode
- TCP Client connection
- UART ↔ TCP Forwarding
- STM32 Reset Control (GPIO)
- Packet handling with state machine

### 🔹 Python GUI
- Login System
- Tkinter-based interface
- Real-time logs
- Supports:
  - Bootloader Version
  - Chip ID
  - Read Protection
  - Option Bytes
  - Sector Erase
  - Full Erase
  - Firmware Upload
  - Jump to Application

---

##  Communication Protocol

### Command Packet Format
```
[ Length (1B) ][ Command (1B) ][ Payload (N Bytes) ][ CRC32 (4 Bytes) ]
```

### Response Format
```
[ Status (1B) ][ Data (Optional) ][ 0xFF ]
```

### Status Codes

| Code | Meaning |
|------|--------|
| 0x01 | BL_OK |
| 0x00 | BL_NOK |

### End of Message
```
0xFF
```

---

## 🧠 CRC Details

- Polynomial: 0x04C11DB7
- Initial Value: 0xFFFFFFFF
- Same implementation as STM32 HAL CRC

---

##  Supported Commands

| Command | Code | Description |
|--------|------|------------|
| Get Version | 0x10 | Bootloader version |
| Get Help | 0x11 | Supported commands |
| Get Chip ID | 0x12 | MCU ID |
| Erase Sectors | 0x13 | Partial erase |
| Mass Erase | 0x14 | Full erase |
| Jump to Address | 0x16 | Jump execution |
| Write Flash | 0x17 | Upload firmware |
| Read Protection | 0x18 | RDP level |
| Read Option Bytes | 0x19 | Option bytes |

---

##  Memory Layout

```
0x08000000  → Bootloader Start
0x08008000  → Application Start
```

Bootloader occupies the first sectors and is protected.

---

##  Firmware Upload Flow

1. Select firmware file  
2. Split into chunks (~248 bytes)  
3. Send packets:  

```
[Length][Command][Address][Size][Data][CRC]
```

4. Wait for BL_OK  
5. Repeat until complete  
6. Send Jump to Application  

---

##  ESP32 Role

- Receive TCP data from GUI  
- Forward to STM32 via UART  
- Receive STM32 response  
- Send back to GUI  
- Control STM32 reset pin  

---

##  Safety Features

- CRC validation  
- Address validation  
- Flash boundary check  
- Sector validation  
- Data alignment check  

---

