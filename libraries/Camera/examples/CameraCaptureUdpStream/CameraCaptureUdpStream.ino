#include <WiFi.h>
#include <WiFiUdp.h>
#include "camera.h"

// -----------------------------------------------------------------------------
// WiFi UDP camera stream configuration
// Update these values to match your WiFi network and the PC/server listening on UDP.
// -----------------------------------------------------------------------------
// const char *WIFI_SSID = "Arduinoboards";
// const char *WIFI_PASSWORD = "arduinotest";
const char *WIFI_SSID = "iPhone Dany";
const char *WIFI_PASSWORD = "danidani";
const IPAddress REMOTE_IP(172, 20, 10, 12);
const uint16_t REMOTE_PORT = 5005;
const uint16_t LOCAL_UDP_PORT = 5006;
const uint16_t UDP_CHUNK_SIZE = 1024;

Camera cam;
WiFiUDP udp;

void fatal_error(const char *msg) {
  Serial.println(msg);
  pinMode(LED_BUILTIN, OUTPUT);
  while (1) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

bool connect_to_wifi() {
  Serial.print("Connecting to WiFi ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  for (int attempt = 0; attempt < 60; ++attempt) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connection failed");
  return false;
}

void write_udp_frame_header(uint32_t frame_id, uint32_t payload_size, uint16_t chunk_index,
                            uint16_t chunk_count) {
  uint8_t header[16];

  header[0] = 'F';
  header[1] = 'R';
  header[2] = 'M';
  header[3] = 'E';

  header[4] = (payload_size >> 24) & 0xFF;
  header[5] = (payload_size >> 16) & 0xFF;
  header[6] = (payload_size >> 8) & 0xFF;
  header[7] = payload_size & 0xFF;

  header[8] = (frame_id >> 24) & 0xFF;
  header[9] = (frame_id >> 16) & 0xFF;
  header[10] = (frame_id >> 8) & 0xFF;
  header[11] = frame_id & 0xFF;

  header[12] = (chunk_index >> 8) & 0xFF;
  header[13] = chunk_index & 0xFF;

  header[14] = (chunk_count >> 8) & 0xFF;
  header[15] = chunk_count & 0xFF;

  if (udp.write(header, sizeof(header)) != sizeof(header)) {
    Serial.println("Failed to write UDP header");
  }
}

void send_frame_over_udp(const uint8_t *frame_data, size_t frame_size, uint32_t frame_id) {
  const uint16_t chunk_count = (frame_size + UDP_CHUNK_SIZE - 1) / UDP_CHUNK_SIZE;

  for (uint16_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
    const uint32_t offset = static_cast<uint32_t>(chunk_index) * UDP_CHUNK_SIZE;
    const uint16_t chunk_len = min<uint16_t>(UDP_CHUNK_SIZE, frame_size - offset);

    udp.beginPacket(REMOTE_IP, REMOTE_PORT);
    write_udp_frame_header(frame_id, frame_size, chunk_index, chunk_count);
    Serial.println("Sending UDP chunk number: " + String(chunk_index));
    if (chunk_len > 0) {
      if (udp.write(frame_data + offset, chunk_len) != chunk_len) {
        Serial.println("Failed to write UDP chunk number: " + String(chunk_index) + " of size: " + String(chunk_len));
      }
    }
    udp.endPacket();
  }
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  if (!connect_to_wifi()) {
    fatal_error("WiFi connection failed");
  }

  udp.begin(LOCAL_UDP_PORT);

  if (!cam.begin(320, 240, CAMERA_RGB565)) {
    fatal_error("Camera begin failed");
  }
  cam.setVerticalFlip(false);
  cam.setHorizontalMirror(false);

  Serial.println("UDP camera stream ready");
}

void loop() {
  FrameBuffer fb;
  static uint32_t frame_id = 0;

  if (cam.grabFrame(fb)) {
    if (WiFi.status() == WL_CONNECTED) {
      send_frame_over_udp(fb.getBuffer(), fb.getBufferSize(), frame_id++);
    }
    cam.releaseFrame(fb);
  }

  delay(10);
}
