#include <Arduino.h>
#include "display_tile_driver.h"
#include "image_960x240.h"

// ==================== CONFIGURATION ====================
//#define DEVICE_TYPE_MASTER   // uncomment for master board
#define DEVICE_TYPE_SLAVE   // uncomment for slave board

// UART pins
#define UART_TX_PIN  10
#define UART_RX_PIN  11

// Baud rate
#define UART_BAUD    460800

// Commands
#define CMD_REQUEST_SEND    0xAA  // Master asks if slave is ready
#define CMD_ACK             0xA2  // Acknowledge - ready/success
#define CMD_NACK            0xA3  // Not acknowledge - busy/error
#define CMD_START_IMAGE     0xAB  // Master starts sending image
#define CMD_IMAGE_COMPLETE  0xAC  // Slave confirms complete reception
#define CMD_TEST_PATTERN    0xEE

// Image dimensions
#define IMAGE_WIDTH  960
#define IMAGE_HEIGHT 240
#define IMAGE_SIZE   (IMAGE_WIDTH * IMAGE_HEIGHT)

// Timeouts
#define ACK_TIMEOUT_MS  3000
#define RX_TIMEOUT_MS   10000  // Much longer timeout for full image reception

// ==================== COMMON CODE ====================
HardwareSerial ImageSerial(1);

// ==================== MASTER CODE ====================
#ifdef DEVICE_TYPE_MASTER

bool waitForResponse(uint8_t expectedCmd, uint32_t timeout_ms) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout_ms) {
        if (ImageSerial.available()) {
            uint8_t response = ImageSerial.read();
            
            if (response == expectedCmd) {
                return true;
            } else if (response == CMD_NACK) {
                Serial.println("Received NACK from slave");
                return false;
            } else {
                Serial.printf("Unexpected response: 0x%02X\n", response);
            }
        }
        delay(10);
    }
    
    Serial.println("Timeout waiting for response");
    return false;
}

void sendImage() {
    Serial.println("\n=== Starting UART image transfer ===");
    
    pinMode(48, OUTPUT);
    digitalWrite(48, LOW);

    // Step 1: Request to send
    Serial.println("Step 1: Requesting permission to send...");
    ImageSerial.write(CMD_REQUEST_SEND);
    ImageSerial.flush();
    
    // Step 2: Wait for ACK from slave
    Serial.println("Step 2: Waiting for ACK from slave...");
    if (!waitForResponse(CMD_ACK, ACK_TIMEOUT_MS)) {
        Serial.println("ERROR: Slave not ready or no response!");
        Serial.println("Transfer aborted.\n");
        return;
    }
    Serial.println("✓ Slave is ready!");
    
    delay(100);  // Brief pause before starting
    
    // Step 3: Send START command
    Serial.println("Step 3: Sending START command...");
    ImageSerial.write(CMD_START_IMAGE);
    ImageSerial.flush();
    delay(100);
    
    // Step 4: Send image data
    Serial.println("Step 4: Sending image data...");
    unsigned long startTime = millis();
    
    const uint32_t totalPixels = IMAGE_SIZE;
    const uint32_t totalBytes = IMAGE_SIZE * 2;
    const uint32_t chunkSize = 1024;  // Use uint32_t to avoid overflow
    uint8_t buffer[chunkSize * 2];
    uint32_t bytesSent = 0;
    uint32_t pixelsSent = 0;

    digitalWrite(48, HIGH);
    
    Serial.printf("Total pixels to send: %u, chunk size: %u\n", totalPixels, chunkSize);
    
    for (uint32_t i = 0; i < totalPixels; i += chunkSize) {
        uint32_t pixelsInChunk = min(chunkSize, totalPixels - i);  // Use uint32_t

        // Pack RGB565 pixels into bytes
        for (uint32_t j = 0; j < pixelsInChunk; j++) {
            uint16_t pixel = pgm_read_word(&image[i + j]);
            buffer[j * 2]     = (pixel >> 8) & 0xFF;
            buffer[j * 2 + 1] = pixel & 0xFF;
        }

        // Send chunk
        size_t bytesToSend = pixelsInChunk * 2;
        size_t written = ImageSerial.write(buffer, bytesToSend);
        
        bytesSent += written;
        pixelsSent += pixelsInChunk;

        if (written != bytesToSend) {
            Serial.printf("ERROR: Only wrote %d of %d bytes at pixel %u\n", 
                         written, bytesToSend, i);
        }

        // Progress (less frequent to avoid slowing down)
        if (i % 50000 == 0) {
            Serial.printf("Sent: %lu/%u pixels, %lu/%u bytes (%.1f%%)\n",
                         pixelsSent, totalPixels, bytesSent, totalBytes,
                         (float)pixelsSent * 100.0 / totalPixels);
        }

        // No delay - send as fast as possible
    }
    
    Serial.printf("Loop completed. Final count: %u pixels sent\n", pixelsSent);

    ImageSerial.flush();
    digitalWrite(48, LOW);

    unsigned long elapsed = millis() - startTime;
    Serial.printf("Data transmission complete: %u pixels, %lu bytes in %lu ms\n", 
                  totalPixels, bytesSent, elapsed);
    Serial.printf("Speed: %.2f KB/s\n", (bytesSent / 1024.0) / (elapsed / 1000.0));
    
    // Step 5: Wait for completion confirmation from slave
    Serial.println("Step 5: Waiting for completion confirmation from slave...");
    if (waitForResponse(CMD_IMAGE_COMPLETE, ACK_TIMEOUT_MS)) {
        Serial.println("✓ Slave confirmed complete reception!");
        Serial.println("\n=== Transfer successful! ===\n");
    } else {
        Serial.println("✗ No confirmation from slave - transfer may be incomplete!");
        Serial.println("\n=== Transfer failed! ===\n");
    }
}

void sendTestPattern() {
    Serial.println("Sending test pattern...");
    ImageSerial.write(CMD_TEST_PATTERN);
    ImageSerial.flush();
    delay(100);
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println("\n========================================");
    Serial.println("    MASTER ESP32-S3 - UART Sender");
    Serial.println("========================================");

    // Initialize UART with both TX and RX for bidirectional communication
    ImageSerial.setRxBufferSize(1024);
    ImageSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    Serial.println("Commands:");
    Serial.println("  s - Send full image (with handshaking)");
    Serial.println("  t - Send test pattern");
    Serial.println("========================================\n");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        while (Serial.available()) Serial.read();

        if (cmd == 's' || cmd == 'S') sendImage();
        else if (cmd == 't' || cmd == 'T') sendTestPattern();
    }
    delay(50);
}

#endif  // MASTER

// ==================== SLAVE CODE ====================
#ifdef DEVICE_TYPE_SLAVE

DisplayTileDriver display(4, 5, 6, 7, 21);

// Image buffer in PSRAM
uint16_t *imageBuffer = nullptr;
bool imageReady = false;
bool receivingImage = false;
uint32_t receivedPixels = 0;
uint32_t receivedBytes = 0;

// Persistent state variables
bool waitingForCommand = true;
uint8_t pendingHighByte = 0;
bool haveHigh = false;

// Timeout handling
unsigned long lastByteTime = 0;

void sendResponse(uint8_t cmd) {
    ImageSerial.write(cmd);
    ImageSerial.flush();
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println("\n========================================");
    Serial.println("    SLAVE ESP32-S3 - UART Receiver");
    Serial.println("========================================\n");

    // PSRAM check & allocation
    if (psramFound()) {
        Serial.printf("PSRAM found: %d bytes\n", ESP.getPsramSize());
        imageBuffer = (uint16_t*)ps_malloc(IMAGE_SIZE * sizeof(uint16_t));
        if (!imageBuffer) {
            Serial.println("ERROR: PSRAM allocation failed!");
            while(1) delay(1000);
        }
        Serial.println("Image buffer allocated in PSRAM");
        memset(imageBuffer, 0, IMAGE_SIZE * sizeof(uint16_t));
    } else {
        Serial.println("ERROR: No PSRAM detected!");
        while(1) delay(1000);
    }

    // Initialize UART with both RX and TX for bidirectional communication
    ImageSerial.setRxBufferSize(32768);  // Much larger RX buffer
    ImageSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    // Init displays
    Serial.println("Initializing displays...");
    display.begin();
    display.clearAll(TFT_BLACK);
    Serial.println("Displays initialized");

    // Ready blink
    pinMode(48, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(48, HIGH); delay(200);
        digitalWrite(48, LOW);  delay(200);
    }

    Serial.println("\n========================================");
    Serial.println("Ready to receive UART data");
    Serial.println("========================================\n");
}

void loop() {
    // Check for timeout during image reception
    if (receivingImage && (millis() - lastByteTime > RX_TIMEOUT_MS)) {
        Serial.printf("\n!!! TIMEOUT - Received %u/%u pixels, %u/%u bytes !!!\n",
                     receivedPixels, IMAGE_SIZE, receivedBytes, IMAGE_SIZE * 2);
        Serial.println("Transfer incomplete, sending NACK...\n");
        
        sendResponse(CMD_NACK);
        
        // Don't reset immediately - display what we have
        imageReady = true;
        receivingImage = false;
        waitingForCommand = true;
        haveHigh = false;
        digitalWrite(48, LOW);
    }
    
    while (ImageSerial.available()) {  // Process ALL available bytes
        lastByteTime = millis();
        
        uint8_t b = ImageSerial.read();

        if (waitingForCommand) {
            if (b == CMD_REQUEST_SEND) {
                Serial.println("=== Received REQUEST_SEND from master ===");
                
                // Check if we're ready (not currently processing)
                if (!receivingImage && !imageReady) {
                    Serial.println("Slave is ready - sending ACK");
                    sendResponse(CMD_ACK);
                } else {
                    Serial.println("Slave is busy - sending NACK");
                    sendResponse(CMD_NACK);
                }
            }
            else if (b == CMD_START_IMAGE) {
                Serial.println("=== Starting image receive ===");
                Serial.printf("Expecting %u pixels (%u bytes)\n", IMAGE_SIZE, IMAGE_SIZE * 2);
                receivingImage = true;
                waitingForCommand = false;
                receivedPixels = 0;
                receivedBytes = 0;
                haveHigh = false;
                lastByteTime = millis();
                digitalWrite(48, HIGH);
            }
            else if (b == CMD_TEST_PATTERN) {
                Serial.println("Test pattern command received");
                display.displayTestPattern();
            }
        }
        else {
            // Receiving image pixels
            receivedBytes++;
            
            if (!haveHigh) {
                pendingHighByte = b;
                haveHigh = true;
            } else {
                uint16_t pixel = (pendingHighByte << 8) | b;
                if (receivedPixels < IMAGE_SIZE) {
                    imageBuffer[receivedPixels++] = pixel;

                    if (receivedPixels % 50000 == 0) {
                        Serial.printf("Received: %u/%u pixels, %u/%u bytes (%.1f%%)\n",
                                      receivedPixels, IMAGE_SIZE,
                                      receivedBytes, IMAGE_SIZE * 2,
                                      (float)receivedPixels * 100.0 / IMAGE_SIZE);
                    }

                    if (receivedPixels >= IMAGE_SIZE) {
                        Serial.printf("\n=== Image receive complete! ===\n");
                        Serial.printf("Total: %u pixels, %u bytes\n", 
                                     receivedPixels, receivedBytes);
                        
                        // Send completion confirmation to master
                        Serial.println("Sending IMAGE_COMPLETE to master");
                        sendResponse(CMD_IMAGE_COMPLETE);
                        
                        imageReady = true;
                        receivingImage = false;
                        waitingForCommand = true;
                        digitalWrite(48, LOW);
                    }
                }
                haveHigh = false;
            }
        }
    }

    // Display when complete
    if (imageReady) {
        Serial.println("Displaying image...");
        unsigned long t = millis();
        display.displayImage(imageBuffer, IMAGE_WIDTH, IMAGE_HEIGHT);
        Serial.printf("Display took %lu ms\n\n", millis() - t);

        imageReady = false;
        // Don't reset pixel counters here - keep for debugging
    }
}

#endif  // SLAVE