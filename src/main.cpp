#include "esp_camera.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "driver/temp_sensor.h"

#define CAMERA_MODEL_ESP32S3_EYE

#include "camera_pins.h"

#define PIN_PIXS 48
#define PIX_NUM 1

Adafruit_NeoPixel pixels(PIX_NUM, PIN_PIXS, NEO_GRB + NEO_KHZ800);

extern void showPixelColor(uint32_t c); // Declaration for use in app_httpd.cpp
extern bool isStreaming; // Add streaming flag declaration

void startCameraServer();

void showPixelColor(uint32_t c) {
  pixels.setPixelColor(0, c);
  pixels.show();
}

void inline startWifiConfig() {
  const char* ssid = "cam";
  const char* password = "esp32cam"; // password must be at least 8 characters for WPA2
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("Hotspot IP: ");
  Serial.println(WiFi.softAPIP());
  showPixelColor(0x00FF00);
}

void inline initCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  if (psramFound()) {
    Serial.printf("PS RAM Found [%d]\n", ESP.getPsramSize());
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);      // flip it back
  s->set_brightness(s, 1); // up the brightness just a bit
  s->set_saturation(s, 0); // lower the saturation
}

void initTempSensor(){
  temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
  temp_sensor.dac_offset = TSENS_DAC_L1;  // TSENS_DAC_L2 is default; L4(-40°C ~ 20°C), L2(-10°C ~ 80°C), L1(20°C ~ 100°C), L0(50°C ~ 125°C)
  temp_sensor_set_config(temp_sensor);
  temp_sensor_start();
}

unsigned long lastTempRead = 0;
const unsigned long TEMP_READ_INTERVAL = 1000; // every 1 second

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(8);
  showPixelColor(0x0);
  initTempSensor();
  initCamera();
  startWifiConfig();
  startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("' to connect");
}

void loop() { 
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastTempRead >= TEMP_READ_INTERVAL) {
    if (!isStreaming) { // Only when streaming is inactive
      float temp = 0;
      temp_sensor_read_celsius(&temp);
      Serial.printf("Temperature: %g°C\n", round(temp));
    }
    lastTempRead = currentMillis;
  }
}