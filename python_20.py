import serial
import numpy as np
import cv2
import torch
import time

# Serial setup
cam_ser = serial.Serial('COM7', 115200, timeout=2)
motor_ser = serial.Serial('COM3', 9600, timeout=1)

# Load YOLOv5 model
model = torch.hub.load('ultralytics/yolov5', 'custom', path='best.pt')

def capture_frame():
    cam_ser.write(b'C')  # Trigger capture
    buffer = bytearray()
    
    while True:
        line = cam_ser.readline().decode('ascii', 'ignore').strip()
        if "FRAME_START" in line:
            buffer = bytearray()
        elif "FRAME_END" in line:
            if len(buffer) == 4800:  # 80x60
                return np.frombuffer(buffer, dtype=np.uint8).reshape((60, 80))
            else:
                print(f"Frame error: {len(buffer)}/4800 bytes")
                return None
        else:
            buffer.extend(line.encode('ascii'))

def process_frame(img):
    # Upscale for better detection
    img_rgb = cv2.cvtColor(cv2.resize(img, (320, 240)), cv2.COLOR_GRAY2RGB)
    
    # Run inference
    results = model(img_rgb)
    detections = results.pandas().xyxy[0]
    
    if detections.empty:
        return "OFF"
    
    # Get largest detection
    largest = detections.loc[detections['confidence'].idxmax()]
    w = largest['xmax'] - largest['xmin']
    h = largest['ymax'] - largest['ymin']
    size = max(w, h)
    
    return "SINGLE" if size < 100 else "DOUBLE"

while True:
    frame = capture_frame()
    if frame is not None:
        command = process_frame(frame)
        motor_ser.write(command.encode() + b'\n')
        print(f"Sent: {command}")
    time.sleep(0.5)