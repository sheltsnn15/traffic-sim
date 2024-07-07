# RTOS-Based Traffic Light Simulation with ESP32 Integration

## Overview

This project develops a traffic light simulation using the ESP32 microcontroller and FreeRTOS. The system controls traffic lights and allows remote monitoring and management through an embedded web server.

### Components

- **RTOS:** FreeRTOS
- **Microcontroller:** ESP32
- **Networking:** HTTP for communication
- **Web Interface:** For remote control and monitoring

### Architecture

- **ESP32 Microcontroller:** Manages RTOS tasks and controls the traffic light simulation.
- **Web Interface:** Allows users to interact with and monitor the traffic lights remotely.

### Key Features

1. **Adaptive Traffic Management:** Adjusts light timings based on real-time traffic conditions.
2. **Intersection Coordination:** Synchronizes traffic lights to optimize traffic flow across multiple intersections.
3. **Web Interface Interaction:** Handles HTTP commands for remote control and updates the web interface with current traffic light statuses.
4. **Button Handling:** Implements debouncing to handle button presses reliably.
