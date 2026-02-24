# wifi_manager_pro_v2 — ESP32 Advanced Wi-Fi Manager

A production-oriented, non-blocking state machine for ESP32 Wi-Fi management based on **ESP-IDF v5.5.1**. This project represents an evolutionary step beyond basic demos, structured for scalability, maintainability, and real-world IoT deployment patterns.

## 🚀 Key Features

* **Robust State Machine:** Centralized control flow through defined states (`BOOT`, `SCANNING`, `TRY_STA`, `PROVISIONING`, `CONNECTED`, etc.).
* **Smart Scanning (Network Album):** Before entering provisioning or connecting, the system generates a fresh "album" of available networks to ensure the captive portal displays real-time data.
* **Non-Blocking Wait Mode:** If no networks are found, the system enters a 30-second "cooldown" but remains responsive to hardware interrupts.
* **Instant Button Override (GPIO 0):** Immediate access to Configuration Mode at any time via the BOOT button.
* **NVS Optimization:** Flash memory protection logic that only commits credentials after successful validation, preventing unnecessary wear.

---

## 🔁 System Flow

The system follows a prioritized logic path:
1.  **BOOT:** Hardware and radio initialization.
2.  **SCANNING:** Environmental check.
3.  **DECISION:** * If **Button Pressed** → Move to **PROVISIONING**.
    * If **Known Network Found** → Move to **TRY_STA**.
    * If **No Networks** → Enter **Wait Mode** (30s) then retry scan.
4.  **CONNECTED:** System remains in a monitoring state, ready for disconnection or manual reset.

---

## 🧱 Layered Architecture

wifi_manager_pro_v2/
├── main/
│   ├── main.c              # Application entry point
│   ├── system_state.c      # Core state machine logic
│   ├── wifi_manager.c      # Wi-Fi driver & event handling
│   ├── wifi_provisioning.c # SoftAP & Captive Portal management
│   ├── wifi_scanner.c      # Scanning logic & SSID temporary storage
│   ├── led_status.c        # Independent FreeRTOS task for visual feedback
│   ├── storage_nvs.c       # Persistent credential storage
│   └── dns_server.c        # DNS redirect for Captive Portal
├── CMakeLists.txt          # Project configuration
├── sdkconfig               # Project hardware/software settings
└── README.md

---

💡 LED Status Patterns
The led_status task runs independently to provide real-time visual feedback:
State          LED Pattern         Meaning
-------------------------------------------------------------------
Booting        Slow Blink          System initializing
Scanning       Quick Pulse         Searching for networks / Waiting
Connecting     Fast Blink          Attempting STA connection
Provisioning   Breath Effect       SoftAP active, waiting for user
Connected      Solid ON            Internet access available
Error          SOS / Rapid         Critical failure

---

💾 NVS Credential Lifecycle
- Safe Validation:     New credentials from the captive portal are tested before being permanently saved.
- Corruption Recovery: Automatic fallback to Provisioning Mode if NVS data is invalid.
- Zero-Wear Logic:     Re-connecting to a known network does not trigger a Flash write cycle.

---

🧪 Build & Flash
Ensure your ESP-IDF environment is configured (v5.5.1).

One-step build and flash:
idf.py build flash monitor

Step-by-step:
1. Configure: idf.py menuconfig (Set GPIOs for LED and Button).
2. Build: idf.py build
3. Flash: idf.py flash
4. Monitor: idf.py monitor

---

⚙️ Key Configuration
Key settings available in menuconfig or Kconfig.projbuild:
- GPIO 0: Default manual override trigger.
- STA Timeout: 15 seconds before failing connection.
- Scan Cooldown: 30 seconds wait period when no networks are found
















