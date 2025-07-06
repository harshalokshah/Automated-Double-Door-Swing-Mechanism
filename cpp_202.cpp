#include <Wire.h>

#define OV7670_ADDR 0x21
#define VS_PIN 2
#define HS_PIN 3
#define PCLK_PIN 4
const int dataPins[8] = {5, 6, 7, 8, 9, 10, 11, 12}; // D0-D7

// Camera config (QQVGA 80x60 YUV)
const uint8_t reg_config[][2] = {
  {0x12, 0x80}, // Reset
  {0x0C, 0x08},  // COM3 (Enable scaling)
  {0x3E, 0x00},  // COM14 (No PCLK scaling)
  {0x40, 0xD0},  // COM15 (RGB565)
  {0x12, 0x0C},  // COM7 (QQVGA + color bar)
  {0x11, 0xC0},  // CLKRC (Internal clock)
  {0x42, 0x00},  // COM17 (No DSP color bar)
  {0x14, 0x18},  // COM9 (AGC ceiling)
  {0x4F, 0xB3},  // MTX1
  {0x50, 0xB3},  // MTX2
  {0x51, 0x00},  // MTX3
  {0x52, 0x3D},  // MTX4
  {0x53, 0xA7},  // MTX5
  {0x54, 0xE4},  // MTX6
  {0x3D, 0xC0},  // COM13 (UV sat auto adj)
  {0x00, 0x00}   // Terminator
};

void wrReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(OV7670_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
  delay(1);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Configure pins
  pinMode(VS_PIN, INPUT);
  pinMode(HS_PIN, INPUT);
  pinMode(PCLK_PIN, INPUT);
  for (int i = 0; i < 8; i++) pinMode(dataPins[i], INPUT);
  
  // Apply camera config
  for (int i = 0; reg_config[i][0]; i++) 
    wrReg(reg_config[i][0], reg_config[i][1]);
  
  Serial.println("CAM_READY");
}

void captureFrame() {
  // Wait for new frame (VSYNC falling edge)
  while (digitalRead(VS_PIN) == HIGH);
  while (digitalRead(VS_PIN) == LOW);
  
  Serial.println("FRAME_START");
  
  for (int y = 0; y < 60; y++) {
    // Wait for line start (HSYNC falling edge)
    while (digitalRead(HS_PIN) == HIGH);
    
    for (int x = 0; x < 80; x++) {
      // Wait for pixel clock (rising edge)
      while (digitalRead(PCLK_PIN) == LOW);
      
      // Read Y component (luminance)
      uint8_t pixel = 0;
      for (int b = 0; b < 8; b++) {
        pixel |= (digitalRead(dataPins[b]) << b);
      }
      Serial.write(pixel);
      
      // Skip UV components
      while (digitalRead(PCLK_PIN) == HIGH);
      while (digitalRead(PCLK_PIN) == LOW);
      while (digitalRead(PCLK_PIN) == HIGH);
    }
  }
  Serial.println("FRAME_END");
}

void loop() {
  if (Serial.available() && Serial.read() == 'C') {
    captureFrame();
  }
}