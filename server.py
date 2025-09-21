import requests
import time
import os
import cv2
import numpy as np
import threading
from flask import Flask, jsonify, render_template
from datetime import datetime


ESP32_IP = "xxx.xxx.xxx.xxx"  #ip address of esp32-cam
ESP32_URL = f"http://{ESP32_IP}/"
REQUEST_INTERVAL = 30  
SAVE_FOLDER = "captured_images_with_detection"


app = Flask(__name__)


data_lock = threading.Lock()

heatmap_points = [] 
device_status = {
    "status": "WAITING",
    "last_seen": "Never",
    "image_count": 0
}


def fetch_from_esp32():
    """
    This function runs in a background thread, continuously fetching data
    from the ESP32-CAM.
    """
    global heatmap_points, device_status

    hog = cv2.HOGDescriptor()
    hog.setSVMDetector(cv2.HOGDescriptor_getDefaultPeopleDetector())

    while True:
        try:
            print(f"\n[Worker] Requesting image from {ESP32_URL}...")
            response = requests.get(ESP32_URL, timeout=10)
            response.raise_for_status()

            # --- Update Status ---
            with data_lock:
                device_status["status"] = "CONNECTED"
                device_status["last_seen"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                device_status["image_count"] += 1
            
            print("[Worker] Connection successful.")

            # --- Image Processing ---
            image_bytes = np.frombuffer(response.content, np.uint8)
            image = cv2.imdecode(image_bytes, cv2.IMREAD_COLOR)

            if image is None:
                print("[Worker] Error: Failed to decode image.")
                time.sleep(REQUEST_INTERVAL)
                continue

            # --- Person Detection ---
            boxes, weights = hog.detectMultiScale(image, winStride=(8, 8), padding=(16, 16), scale=1.05)
            person_count = len(boxes)
            print(f"[Worker] Found {person_count} person(s) in the image.")

            # --- File Saving (with detection boxes) ---
            for (x, y, w, h) in boxes:
                cv2.rectangle(image, (x, y), (x + w, y + h), (0, 255, 0), 2)
                cv2.putText(image, 'Person', (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            
            gps_data_str = response.headers.get("GPS-Data", "no_gps_data_received")
            
            if "no_gps" not in gps_data_str:
                # 1. Create the base filename without the extension
                base_filename = gps_data_str.replace(":", "-").replace(",", "_")
                
                # 2. Check for existence and find a unique name by appending a counter
                counter = 1
                filename_to_try = f"{base_filename}.jpg"
                filepath = os.path.join(SAVE_FOLDER, filename_to_try)

                while os.path.exists(filepath):
                    filename_to_try = f"{base_filename}_{counter}.jpg"
                    filepath = os.path.join(SAVE_FOLDER, filename_to_try)
                    counter += 1
                
                # 3. Save the image 
                cv2.imwrite(filepath, image)
                print(f"[Worker] Saved image with detections to: {filepath}")

                # --- Update Heatmap Data ---
                try:
                    coords_part = gps_data_str.split('_')[0]
                    lat_str, lng_str = coords_part.split(',')
                    lat, lng = float(lat_str), float(lng_str)
                    
                    with data_lock:
                        heatmap_points.append({'lat': lat, 'lng': lng, 'count': person_count})
                except (ValueError, IndexError) as e:
                    print(f"[Worker] Could not parse GPS data '{gps_data_str}': {e}")
            else:
                 print("[Worker] Warning: No GPS data received.")


        except requests.exceptions.RequestException as e:
            print(f"[Worker] Error connecting to ESP32-CAM: {e}")
            with data_lock:
                device_status["status"] = "DISCONNECTED"
        
        except Exception as e:
            print(f"[Worker] An unexpected error occurred: {e}")
            with data_lock:
                device_status["status"] = "ERROR"

        time.sleep(REQUEST_INTERVAL)


@app.route('/')
def index():
    """Serves the main dashboard HTML page."""
    return render_template('index.html')

@app.route('/data')
def get_data():
    """Provides heatmap data to the dashboard."""
    with data_lock:
        return jsonify(heatmap_points)

@app.route('/status')
def get_status():
    """Provides device status to the dashboard."""
    with data_lock:
        return jsonify(device_status)


if __name__ == '__main__':
    if not os.path.exists(SAVE_FOLDER):
        os.makedirs(SAVE_FOLDER)
        print(f"Created directory: {SAVE_FOLDER}")

    worker_thread = threading.Thread(target=fetch_from_esp32, daemon=True)
    worker_thread.start()

    # Start the Flask web server
    print("\n--- Real-time Population Density Dashboard ---")
    print("Open your web browser and go to http://127.0.0.1:5000")
    app.run(host='0.0.0.0', port=5000)

