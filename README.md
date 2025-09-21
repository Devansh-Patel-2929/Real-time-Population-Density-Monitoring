# Real-time Population Density Monitoring with ESP32-CAM and AI

This project provides a complete solution for capturing images and GPS coordinates from a remote ESP32-CAM, performing person detection on the images, and visualizing the data in real-time on a web-based dashboard.

The ESP32-CAM acts as a mobile data collection unit. It hosts a web server that, upon request, captures a high-resolution image and its current GPS location. A Python backend server, running on a PC, periodically requests this data, uses an OpenCV AI model to count the number of people in the image, and then logs this data. The results are displayed on a live dashboard featuring a heatmap that visualizes population density in the surveyed areas.

## Features

* **Remote Data Capture:** ESP32-CAM captures and serves images and GPS data over Wi-Fi.
* **AI-Powered Person Detection:** The Python backend uses a pre-trained HOG (Histogram of Oriented Gradients) model to detect and count people in each image.
* **Live Web Dashboard:** A Flask-based web interface displays real-time statistics, including device status, total images received, and total people detected.
* **Heatmap Visualization:** An interactive map from OpenStreetMap shows the density of detected people using a color-coded heat layer.
* **Persistent Image Storage:** Every captured image, with bounding boxes drawn around detected people, is saved with a unique, geotagged filename for later analysis.

## Hardware Requirements

* ESP32-CAM AI-Thinker Module
* NEO-6M GPS Module
* FTDI Programmer (USB to Serial converter for uploading code)
* Jumper Wires
* A stable **5V, 2A** power supply (A standard USB port is often not sufficient and can cause errors).

## Software & Libraries

* **For ESP32-CAM:**
    * [Arduino IDE](https://www.arduino.cc/en/software)
    * ESP32 Board Manager for Arduino IDE
    * **TinyGPS++** Library by Mikal Hart
* **For Python Backend:**
    * Python 3.7+
    * Flask
    * Requests
    * OpenCV-Python
    * NumPy

## Installation and Setup

Follow these steps to get the project up and running.

### Part 1: Hardware Wiring

Connect the components as described below. It is crucial to have a stable 5V power supply.

| **ESP32-CAM Pin** | **Connects to** | **Purpose** |
| :---------------- | :------------------- | :------------------------- |
| `5V`              | Power Supply `+5V`   | Main Power                 |
| `GND`             | Power Supply `GND`   | Common Ground              |
| `U0TXD` (GPIO1)   | FTDI Programmer `RX` | Serial Communication (TX)  |
| `U0RXD` (GPIO3)   | FTDI Programmer `TX` | Serial Communication (RX)  |
| `GND`             | FTDI Programmer `GND`| Common Ground for flashing |
| `GPIO 12`         | GPS Module `RX`      | GPS Data In                |
| `GPIO 13`         | GPS Module `TX`      | GPS Data Out               |

**For Flashing Mode:** Before uploading the code, you must connect `GPIO 0` to `GND`. You will remove this connection for normal operation.

### Part 2: Programming the ESP32-CAM

1.  **Configure Arduino IDE:** Set up your Arduino IDE to work with ESP32 boards. You can follow [this guide](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/).
2.  **Install Library:** In the Arduino IDE, go to `Sketch` > `Include Library` > `Manage Libraries...` and search for and install `TinyGPS++`.
3.  **Load the Code:** Open the `esp32_cam_gps_server.ino` sketch in the Arduino IDE.
4.  **Update Credentials:** Change the following lines to match your Wi-Fi network:
    ```cpp
    const char* ssid = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    ```
5.  **Upload the Sketch:**
    * Select `AI Thinker ESP32-CAM` as your board.
    * Connect `GPIO 0` to `GND`.
    * Connect the FTDI programmer to your computer and the ESP32-CAM.
    * Press the "Upload" button.
6.  **Get the IP Address:**
    * **Disconnect the `GPIO 0` from `GND`.**
    * Press the reset button on the back of the ESP32-CAM.
    * Open the Serial Monitor (`Tools > Serial Monitor`) and set the baud rate to `115200`.
    * The ESP32 will print its IP address once it connects to your Wi-Fi. Note this IP down.

### Part 3: Running the Python Backend

1.  **Clone the Repository:** Download and unzip the project files to a folder on your computer.
2.  **File Structure:** Ensure your project folder is organized like this:
    ```
    project_folder/
    ├── image_receiver.py
    └── templates/
        └── index.html
    ```
3.  **Install Dependencies:** Open a terminal or command prompt, navigate to `project_folder`, and run:
    ```bash
    pip install Flask requests opencv-python numpy
    ```
4.  **Configure IP Address:** Open `image_receiver.py` and update the `ESP32_IP` variable with the address you noted in the previous step.
    ```python
    ESP32_IP = "YOUR_ESP32_IP_ADDRESS"
    ```
5.  **Run the Server:** In the same terminal, run the script:
    ```bash
    python image_receiver.py
    ```

## How to Use

1.  **Power On:** Power up your wired ESP32-CAM setup. For the best GPS results, place it in a location with a clear view of the sky.
2.  **Start Server:** Run the `image_receiver.py` script on your computer.
3.  **Open Dashboard:** Open your web browser and navigate to `http://12.0.0.1:5000`.
4.  **Observe:** The dashboard will begin updating automatically. The device status will change, and the map will populate with heatmap points as data is collected.
5.  **Check Images:** The processed images will be saved in the `captured_images_with_detection` folder.
