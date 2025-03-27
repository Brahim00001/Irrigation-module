Overview
This repository contains a collection of embedded drivers and communication protocols developed for an STM32-based smart irrigation system. The system is designed to efficiently manage basin water levels, electrical pumps, and valves, while ensuring reliable data logging and remote control via a LoRa gateway.

Key Features
✅ Basin Water Level Monitoring – Detects and maintains optimal water levels using sensor feedback.
✅ Electrical Pump & Valve Control – Automated activation/deactivation for efficient water distribution.
✅ LoRa-based Gateway Communication – Handles data exchange between remote nodes and the central control system.
✅ Flash Memory Management – Stores system logs and configurations persistently.
✅ Real-time Data Logging – Monitors system performance and maintains historical records.

Technologies Used
🔹 Embedded C / C++
🔹 STM32 HAL / LL Drivers
🔹 LoRa Communication (E82G4M20S Module)
🔹 I²C, SPI, UART Protocols
🔹 RTOS (if applicable)

Project Structure
📂 Basin/ – Water level sensor drivers & logic
📂 Electrical_Pumps/ – Pump control drivers
📂 Selenoid_Valves/ – Valve control system
📂 Gateway/ – LoRa communication handlers
📂 Flash_Memory/ – EEPROM & persistent storage drivers
📂 Logs/ – Data logging & debugging utilities

Applications & Impact
🌱 Agriculture & Irrigation – Smart water distribution reduces waste and increases efficiency.
📡 IoT & Remote Monitoring – LoRa integration allows real-time control from remote locations.
⚙️ Scalable & Modular – Can be extended for additional sensors and actuators.
