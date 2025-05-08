
# RP2040 W Proximity Sensor HTTP Client

## Overview

This project utilizes a Raspberry Pi Pico W (RP2040 W) board to read data from a digital proximity sensor. When the sensor state changes (detecting or not detecting proximity), the Pico W connects to a pre-defined WiFi network (expected to be an Access Point hosted by another device, like an Arduino R4 WiFi) and sends a simple HTTP GET request to control a remote device (e.g., turn an LED on or off). This demonstrates basic IoT communication using the Pico W as an HTTP client.

## Features

*   Reads a digital proximity sensor connected to a GPIO pin.
*   Connects to a specified WiFi network (Station Mode).
*   Sends HTTP GET requests (`/ON` or `/OFF`) to a fixed IP address and port upon sensor state changes.
*   Uses the Pico SDK's `pico_cyw43_arch` library for WiFi and lwIP for basic TCP communication.
*   Provides status messages via serial output (USB).

## Hardware Requirements

*   **Raspberry Pi Pico W**
*   **Digital Proximity Sensor:** A sensor with a digital HIGH/LOW output (e.g., HC-SR501 PIR, basic IR proximity switch). Connect its output to GPIO 24, VCC to 3.3V or 5V (check sensor specs), and GND.
*   **WiFi Access Point:** Another device (like the Arduino R4 WiFi running the companion server sketch) configured as an Access Point with known SSID and password.
*   Micro-USB cable for programming and serial monitoring.

## Installation

1.  **Set up Pico SDK:** Ensure you have the Raspberry Pi Pico C/C++ SDK correctly installed and configured for your development environment. [Pico Getting Started Guide](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
2.  **Clone this Repository:**
    ```sh
    git clone [LINK_PARA_SEU_REPOSITORIO_GIT]
    cd [NOME_DO_SEU_DIRETORIO]
    ```
3.  **Configure WiFi Credentials:** Open `main.c` and verify/edit the following lines with the details of the target Access Point:
    ```c
    #define WIFI_SSID "R4WIFI"
    #define WIFI_PASSWORD "12345678"
    #define SERVER_IP_STR "192.168.4.1" // IP address of the device hosting the server
    ```
4.  **Configure Sensor Pin:** Verify `PROXIMITY_SENSOR_PIN` in `main.c` matches the GPIO pin you connected the sensor's output to (default is 24).
5.  **Build the Project:**
    *   Create a build directory: `mkdir build && cd build`
    *   Run CMake: `cmake ..`
    *   Run Make: `make -j$(nproc)` (or `make -jX` where X is number of cores)
6.  **Flash the Pico W:**
    *   Hold the BOOTSEL button on the Pico W while connecting it via USB.
    *   It will appear as a mass storage device (RPI-RP2).
    *   Drag and drop the generated `.uf2` file (found in the `build` directory, e.g., `rp2040w_prox_sensor_client.uf2`) onto the RPI-RP2 drive. The board will automatically reboot.

## Usage

1.  Ensure the target WiFi Access Point ("R4WIFI" by default) is active and running the companion web server (like the Arduino sketch provided).
2.  Power on the RP2040 W.
3.  Open a Serial Monitor connected to the Pico W's USB serial port (baud rate doesn't strictly matter for USB CDC, but standard tools often default to 115200 or 9600).
4.  The Pico W will attempt to connect to the WiFi network. Connection status will be printed to the serial monitor.
5.  Once connected, trigger the proximity sensor (e.g., move your hand in front of it).
6.  Observe the serial monitor output from the Pico W. It should indicate "Proximity DETECTED" or "Proximity NOT Detected" and attempt to send an HTTP GET request (`/ON` or `/OFF`) to the server IP.
7.  Observe the device being controlled by the server (e.g., the LED on the Arduino R4 WiFi) to confirm it turns on/off accordingly.

## Target Server HTTP Endpoints (Assumed)

The Pico W sends requests to these endpoints on the target server (IP defined in `SERVER_IP_STR`):

| Method | Endpoint | Expected Action on Server    | Triggered By Pico W When...       |
| :----- | :------- | :------------------------- | :-------------------------------- |
| GET    | `/ON`    | Turns the controlled device ON  | Proximity sensor output goes HIGH |
| GET    | `/OFF`   | Turns the controlled device OFF | Proximity sensor output goes LOW  |

## Tags

*   **Raspberry Pi Pico W**
*   **RP2040W**
*   **WiFi Client**
*   **HTTP Client**
*   **IoT**
*   **Proximity Sensor**
*   **Pico SDK**
*   **lwIP**
*   **CYW43**
*   **Embedded Systems**
*   **GPIO**

## Author

GitHub: [github.com/luisbaiano](https://github.com/luisbaiano)
