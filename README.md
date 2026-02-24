# wifi_manager_pro_v2 — ESP32 Wi-Fi Manager (Reliability Layer)

A modular, production-oriented ESP32 Wi-Fi manager implementing:
- Auto-reconnect & resilience
- NVS credential storage
- LED status signaling
- Clean layered architecture
- Upgrade path to captive portal provisioning (Option D)

This project represents an **evolutionary step** beyond a basic Wi-Fi demo — structured for scalability, maintainability, and real IoT deployment patterns.

---

## 🎯 Project Goals

- Reliable Wi-Fi STA connection
- Automatic reconnection on drop
- Persistent credentials stored in NVS
- Real-time LED system status indicator
- Modular architecture for future provisioning & HTTP portal
- Foundation for MQTT / OTA / Home Assistant integration

---

## 🧱 Architecture Overview

main.c
├── wifi_manager.c # Wi-Fi connection lifecycle
├── storage_nvs.c # Persistent credential storage
├── led_status.c # Visual system state feedback
└── system_state.c # (Optional Option D upgrade)

---

## 🔁 Boot Flow

BOOT
↓
Initialize NVS
↓
Initialize LED Status
↓
Load Wi-Fi Credentials
↓
Attempt STA Connection
├─ Success → Normal Mode
└─ Fail → Reconnect Loop


(Provisioning Mode will be added in Option D)

---

## 📡 Wi-Fi Features

- Station Mode (STA)
- Auto-reconnect on disconnect
- Backoff retry logic
- Default fallback SSID if NVS empty
- IP acquisition logging
- Connection state API

---

## 💾 NVS Credential Storage

Stores:
- SSID
- Password

Features:
- Safe validation
- Corruption recovery
- Clear / reset support
- Secure commit lifecycle

---

## 💡 LED Status Patterns

| State | Pattern |
|------|--------|
| Boot | Slow blink |
| Connecting | Fast blink |
| Connected | Solid ON |
| Disconnected | Double blink |
| Error | SOS-style alert |

LED task runs independently in FreeRTOS.

---

## 🧪 Build & Flash

idf.py fullclean build flash monitor

or step-by-step:

idf.py build
idf.py flash
idf.py monitor

## 🗂️ Key Files
File			Purpose
main.c			System entrypoint
wifi_manager.c		Wi-Fi lifecycle controller
storage_nvs.c		Credential persistence
led_status.c		Visual state feedback
CMakeLists.txt		Component registration
Kconfig.projbuild	GPIO / LED configuration
idf_component.yml	External dependencies

## ⚙️ Configuration

Configured via menuconfig:
idf.py menuconfig

Key settings:
- LED GPIO
- Blink timing
- Wi-Fi parameters

