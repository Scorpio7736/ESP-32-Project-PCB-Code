#include <WiFi.h>
#include "esp_camera.h"

// Select the AI-Thinker pin definition for the ESP32-CAM module.
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Update these with your Wi-Fi credentials.
constexpr const char* WIFI_SSID = "REPLACE_WITH_YOUR_SSID";
constexpr const char* WIFI_PASSWORD = "REPLACE_WITH_YOUR_PASSWORD";

// HTTP server that will serve the MJPEG stream.
WiFiServer server(80);

// Configure and initialize the camera hardware.
bool initCamera() {
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
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t* sensor = esp_camera_sensor_get();
    sensor->set_brightness(sensor, 0);
    sensor->set_contrast(sensor, 0);
    sensor->set_saturation(sensor, 0);

    Serial.println("Camera initialized.");
    return true;
}

// Wait for an HTTP request header terminator (a blank line).
void readClientRequest(WiFiClient& client) {
    while (client.connected()) {
        if (!client.available()) {
            delay(1);
            continue;
        }

        String line = client.readStringUntil('\n');
        if (line.length() <= 1) {
            break;  // Empty line marks end of header.
        }
    }
}

// Stream MJPEG data to the connected client until they disconnect.
void streamCamera(WiFiClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Cache-Control: no-cache");
    client.println();

    while (client.connected()) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Failed to grab frame");
            break;
        }

        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.print("\r\n");

        esp_camera_fb_return(fb);
        delay(30);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    if (!initCamera()) {
        return;
    }

    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }

    Serial.println();
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());

    server.begin();
    Serial.println("HTTP stream server started on port 80.");
}

void loop() {
    WiFiClient client = server.available();
    if (!client) {
        delay(10);
        return;
    }

    Serial.println("Client connected.");
    client.setTimeout(1000);

    readClientRequest(client);
    streamCamera(client);

    client.stop();
    Serial.println("Client disconnected.");
}
