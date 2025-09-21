/*
  ESP32-CAM with NEO-6M GPS - Server Mode v4
  ESP32-CAM as web server.
  It first waits for a valid GPS fix, printing detailed status (sats, time, HDOP) 
  to the Serial monitor while searching. Once a fix is acquired, it starts the web server. 
  When a client connects, it flashes the LED, captures a new image, reads GPS data,
  and sends the image back in an HTTP response with the GPS data in a custom header.

  Hardware Connections:
  ESP32-CAM      | NEO-6M GPS
  ----------------|-----------
  GND             | GND
  5V              | VCC
  GPIO 12 (U0TXD) | RX
  GPIO 13 (U0RXD) | TX
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>

// WiFi credentials
const char* ssid = "XXX";
const char* password = "XXXXXXXX";

// Define GPS pins
#define GPS_RX_PIN 13
#define GPS_TX_PIN 12

// Onboard flash LED 
#define LED_PIN 4

// Camera model configuration
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

HardwareSerial GPSSerial(1); 
TinyGPSPlus gps;

WiFiServer server(80);

// initialize the camera
bool setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 12; 
  config.fb_count = 2; 

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }
  return true;
}

// Function to connect to WiFi
void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to get GPS data
String getGPSData() {
  String gps_data = "no_gps_data";
  unsigned long start_time = millis();
  while (millis() - start_time < 2000) { 
    if (GPSSerial.available() > 0) {
      if (gps.encode(GPSSerial.read())) {
        if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
          gps_data = String(gps.location.lat(), 6) + "," +
                     String(gps.location.lng(), 6) + "_" +
                     String(gps.date.year()) + "-" +
                     String(gps.date.month()) + "-" +
                     String(gps.date.day()) + "_" +
                     String(gps.time.hour()) + "-" +
                     String(gps.time.minute()) + "-" +
                     String(gps.time.second());
          return gps_data;
        }
      }
    }
  }
  return gps_data; 
}

void waitForGPSFix() {
  Serial.println("Waiting for GPS fix... Please take the device outside with a clear view of the sky.");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  unsigned long lastDisplay = 0;

  while (!gps.location.isValid()) {
    while (GPSSerial.available() > 0) {
      gps.encode(GPSSerial.read());
    }

    if (millis() - lastDisplay > 1000) {
      lastDisplay = millis();
      Serial.print("Satellites: ");
      Serial.print(gps.satellites.isValid() ? String(gps.satellites.value()) : "N/A");

      Serial.print(" | HDOP: ");
      Serial.print(gps.hdop.isValid() ? String(gps.hdop.value() / 100.0) : "N/A");

      Serial.print(" | Time: ");
      if (gps.time.isValid()) {
        if (gps.time.hour() < 10) Serial.print("0");
        Serial.print(gps.time.hour());
        Serial.print(":");
        if (gps.time.minute() < 10) Serial.print("0");
        Serial.print(gps.time.minute());
        Serial.print(":");
        if (gps.time.second() < 10) Serial.print("0");
        Serial.print(gps.time.second());
      } else {
        Serial.print("N/A");
      }
      
      Serial.print(" | Location: ");
      if (gps.location.isValid()) {
        Serial.print(gps.location.lat(), 6);
        Serial.print(", ");
        Serial.print(gps.location.lng(), 6);
      } else {
        Serial.print("Not fixed");
      }
      Serial.println();
    }
  }

  Serial.println("\nGPS fix acquired!");
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  // Waiting for a valid GPS signal before proceeding
  waitForGPSFix();

  if (!setupCamera()) {
    Serial.println("Camera setup failed!");
    return;
  }

  setupWiFi();

  server.begin();
  Serial.println("Server started. Waiting for client requests.");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            digitalWrite(LED_PIN, HIGH);
            
            // first capture a frame and immediately return it. This "flushes"
            // the buffer of any old data.
            camera_fb_t* flush_fb = esp_camera_fb_get();
            if (flush_fb) {
              esp_camera_fb_return(flush_fb); // Return the buffer
            }
            
            // capture the frame 
            camera_fb_t* fb = esp_camera_fb_get();

            digitalWrite(LED_PIN, LOW);
            
            if (!fb) {
              Serial.println("Camera capture failed");
              client.println("HTTP/1.1 500 Internal Server Error");
              client.println("Content-Type: text/plain");
              client.println();
              client.println("Camera capture failed");
              break;
            }

            String gpsCoords = getGPSData();
            Serial.println("GPS Data: " + gpsCoords);

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: image/jpeg");
            client.println("Content-Disposition: inline; filename=capture.jpg");
            client.println("GPS-Data: " + gpsCoords);
            client.println("Content-Length: " + String(fb->len));
            client.println("Connection: close");
            client.println();
            
            client.write(fb->buf, fb->len);
            
            // returning the frame buffer,
            // "delete the image and empty memory for next request". This makes the memory available
            // for the camera driver to use for the next picture.
            esp_camera_fb_return(fb);
            
            break; 
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
}



