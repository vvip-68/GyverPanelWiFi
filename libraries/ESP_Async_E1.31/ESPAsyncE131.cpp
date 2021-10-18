/*
* ESPAsyncE131.cpp
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

#include "ESPAsyncE131.h"
#include <string.h>

// E1.17 ACN Packet Identifier
const byte ESPAsyncE131::ACN_ID[12] = { 0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00 };

// Constructor
ESPAsyncE131::ESPAsyncE131(uint8_t buffers) {

    pbuff = nullptr;
    if (buffers)
    {
        pbuff = RingBuf_new (sizeof (e131_packet_t), buffers);
    }

    stats.num_packets = 0;
    stats.packet_errors = 0;
}

/////////////////////////////////////////////////////////
//
// Public begin() members
//
/////////////////////////////////////////////////////////

bool ESPAsyncE131::begin(e131_listen_t type, uint16_t universe, uint8_t n)
{
    return begin (type, E131_ListenPort, universe, n);
}

bool ESPAsyncE131::begin (e131_listen_t type, ESPAsyncE131PortId UdpPortId, uint16_t universe, uint8_t n)
{
    bool success = false;

    E131_ListenPort = UdpPortId;

    if (type == E131_UNICAST)
        success = initUnicast ();
    if (type == E131_MULTICAST)
        success = initMulticast (universe, n);

    return success;
}

/////////////////////////////////////////////////////////
//
// Private init() members
//
/////////////////////////////////////////////////////////

bool ESPAsyncE131::initUnicast() {
    bool success = false;
    delay(100);

    if (udp.listen(E131_ListenPort)) {
        udp.onPacket(std::bind(&ESPAsyncE131::parsePacket, this,
                std::placeholders::_1));
        success = true;
    }
    return success;
}

bool ESPAsyncE131::initMulticast(uint16_t universe, uint8_t n) {
    bool success = false;
    delay(100);

    IPAddress address = IPAddress(239, 255, ((universe >> 8) & 0xff),
        ((universe >> 0) & 0xff));

    if (udp.listenMulticast(address, E131_ListenPort)) {
        ip4_addr_t ifaddr;
        ip4_addr_t multicast_addr;

        ifaddr.addr = static_cast<uint32_t>(WiFi.localIP());
        for (uint8_t i = 1; i < n; i++) {
            multicast_addr.addr = static_cast<uint32_t>(IPAddress(239, 255,
                    (((universe + i) >> 8) & 0xff), (((universe + i) >> 0)
                    & 0xff)));
            igmp_joingroup(&ifaddr, &multicast_addr);
        }

        udp.onPacket(std::bind(&ESPAsyncE131::parsePacket, this,
                std::placeholders::_1));

        success = true;
    }
    return success;
}

/////////////////////////////////////////////////////////
//
// Packet parsing - Private
//
/////////////////////////////////////////////////////////

void ESPAsyncE131::parsePacket(AsyncUDPPacket _packet) {
    e131_error_t error = ERROR_NONE;

    sbuff = reinterpret_cast<e131_packet_t *>(_packet.data());
    if (memcmp(sbuff->acn_id, ESPAsyncE131::ACN_ID, sizeof(sbuff->acn_id)))
        error = ERROR_ACN_ID;
    if (htonl(sbuff->root_vector) != ESPAsyncE131::VECTOR_ROOT)
        error = ERROR_VECTOR_ROOT;
    if (htonl(sbuff->frame_vector) != ESPAsyncE131::VECTOR_FRAME)
        error = ERROR_VECTOR_FRAME;
    if (sbuff->dmp_vector != ESPAsyncE131::VECTOR_DMP)
        error = ERROR_VECTOR_DMP;
    if (sbuff->property_values[0] != 0)
        error = ERROR_IGNORE;


    if (!error) {
        if (PacketCallback) { (*PacketCallback) (sbuff, UserInfo); }
        if (pbuff) { pbuff->add (pbuff, sbuff); }

        stats.num_packets++;
        stats.last_clientIP = _packet.remoteIP();
        stats.last_clientPort = _packet.remotePort();
        stats.last_seen = millis();
    } else if (error == ERROR_IGNORE) {
        // Do nothing
    } else {
        if (Serial)
            dumpError(error);
        stats.packet_errors++;
    }
}

/////////////////////////////////////////////////////////
//
// Debugging functions - Public
//
/////////////////////////////////////////////////////////

void ESPAsyncE131::dumpError(e131_error_t error) {
    switch (error) {
        case ERROR_ACN_ID:
            Serial.print(F("INVALID PACKET ID: "));
            for (uint i = 0; i < sizeof(ACN_ID); i++)
                Serial.print(sbuff->acn_id[i], HEX);
            Serial.println("");
            break;
        case ERROR_PACKET_SIZE:
            Serial.println(F("INVALID PACKET SIZE: "));
            break;
        case ERROR_VECTOR_ROOT:
            Serial.print(F("INVALID ROOT VECTOR: 0x"));
            Serial.println(htonl(sbuff->root_vector), HEX);
            break;
        case ERROR_VECTOR_FRAME:
            Serial.print(F("INVALID FRAME VECTOR: 0x"));
            Serial.println(htonl(sbuff->frame_vector), HEX);
            break;
        case ERROR_VECTOR_DMP:
            Serial.print(F("INVALID DMP VECTOR: 0x"));
            Serial.println(sbuff->dmp_vector, HEX);
        case ERROR_NONE:
            break;
        case ERROR_IGNORE:
            break;
    }
}
