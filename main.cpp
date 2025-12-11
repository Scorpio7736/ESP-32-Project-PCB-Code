#include "esp_camera.h"
#include "WiFi.h"
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const int INPUT_PIN  = 13; 
const int OUTPUT_PIN = 12; 
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

WiFiServer server(80);

void startCamera() {
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

    if(psramFound()) {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera init failed");
        return;
    }
    Serial.println("Camera initialized.");
}

void setup() {
    Serial.begin(115200);

    pinMode(INPUT_PIN, INPUT_PULLUP);
    pinMode(OUTPUT_PIN, OUTPUT);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Connected!");
    Serial.println(WiFi.localIP());

    startCamera();

    server.begin();
    Serial.println("Server started.");
}

void loop() {
    WiFiClient client = server.available();

    if (client) {
        Serial.println("Client connected.");

        while (client.connected()) {

            // Read input pin
            bool inputState = digitalRead(INPUT_PIN);
            digitalWrite(OUTPUT_PIN, inputState); // Mirror state

            // Grab frame
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera frame failed");
                continue;
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
            client.println();
            client.println("--frame");
            client.println("Content-Type: image/jpeg");
            client.println("Content-Length: " + String(fb->len));
            client.println();
            client.write(fb->buf, fb->len);
            client.println();

            esp_camera_fb_return(fb);

            delay(30);
        }

        client.stop();
        Serial.println("Client disconnected.");
    }
}
