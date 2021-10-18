/*
* E131_Test.ino - Simple sketch to listen for E1.31 data on an ESP32 
*                  and print some statistics.
*
* Project: ESPAsyncE131 - Asynchronous E.131 (sACN) library for Arduino ESP8266 and ESP32
* Copyright (c) 2019 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include <ESPAsyncE131.h>

#define UNIVERSE 1                      // First DMX Universe to listen for
#define UNIVERSE_COUNT 2                // Total number of Universes to listen for, starting at UNIVERSE

const char ssid[] = "........";         // Replace with your SSID
const char passphrase[] = "........";   // Replace with your WPA2 passphrase

// ESPAsyncE131 instance with UNIVERSE_COUNT buffer slots
ESPAsyncE131 e131(UNIVERSE_COUNT);

void setup() {
    Serial.begin(115200);
    delay(10);
    
    // Make sure you're in station mode    
    WiFi.mode(WIFI_STA);

    Serial.println("");
    Serial.print(F("Connecting to "));
    Serial.print(ssid);

    if (passphrase != NULL)
        WiFi.begin(ssid, passphrase);
    else
        WiFi.begin(ssid);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print(F("Connected with IP: "));
    Serial.println(WiFi.localIP());
    
    // Choose one to begin listening for E1.31 data
    //if (e131.begin(E131_UNICAST))                               // Listen via Unicast
    if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT))   // Listen via Multicast
        Serial.println(F("Listening for data..."));
    else 
        Serial.println(F("*** e131.begin failed ***"));
}

void loop() {
    if (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);     // Pull packet from ring buffer
        
        Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
                htons(packet.universe),                 // The Universe for this packet
                htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                e131.stats.num_packets,                 // Packet counter
                e131.stats.packet_errors,               // Packet error counter
                packet.property_values[1]);             // Dimmer data for Channel 1
    }
}
