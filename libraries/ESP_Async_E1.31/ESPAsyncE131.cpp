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

const uint8_t ESPAsyncE131::ACN_ID[12] = { 0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00 };

// Constructor
ESPAsyncE131::ESPAsyncE131(uint8_t buffers) {

    pbuff = nullptr;
    if (buffers)
    {
        pbuff = RingBuf_new (sizeof (e131_packet_t), buffers);
    }

    stats.num_packets = 0;
    stats.packet_errors = 0;
    send_sequence_number = 0;
}

ESPAsyncE131::~ESPAsyncE131() {
  if (listen_type == E131_MULTICAST) {
    ip4_addr_t ifaddr;
    ip4_addr_t multicast_addr;

    ifaddr.addr = static_cast<uint32_t>(WiFi.localIP());
    for (uint8_t i = 0; i < count_universe; i++) {
        multicast_addr.addr = static_cast<uint32_t>(IPAddress(239, 255,
                (((start_universe + i) >> 8) & 0xff), (((start_universe + i) >> 0)
                & 0xff)));
        igmp_leavegroup(&ifaddr, &multicast_addr);
    }
    listen_type = E131_NOT_INITIALIZED;
  }
  if (pbuff) {
    RingBuf_delete(pbuff);
    pbuff = nullptr;
  }
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
    listen_type = type;

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

    start_universe = universe;
    count_universe = n;

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

        udp.onPacket(std::bind(&ESPAsyncE131::parsePacket, this, std::placeholders::_1));

        success = true;
    }
    return success;
}

/////////////////////////////////////////////////////////
//
// Packet creation and set values - Public
//
/////////////////////////////////////////////////////////

/*
e131_packet_t* ESPAsyncE131::createPacket(const uint16_t universe, const uint16_t num_channels) {
  if (universe < 1 || universe > 63999 || num_channels < 1 || num_channels > 512) {
    return NULL;
  }
*/
e131_packet_t* ESPAsyncE131::createPacket(const char* src_name, uint8_t* uuid) {

  e131_packet_t *packet = (e131_packet_t *)malloc(sizeof(e131_packet_t));
  uint8_t src_len = strlen (src_name);
  if (src_len > sizeof (packet->source_name)) src_len = sizeof (packet->source_name);

  // clear packet
  memset(packet, 0, sizeof *packet);

  // set Root Layer values
  packet->preamble_size        = htons(PREAMBLE_SIZE);
  packet->postamble_size       = htons(POSTAMBLE_SIZE);
  memcpy(packet->acn_id, ACN_ID, sizeof(packet->acn_id));
  packet->root_flength 	       = htons((0x7 << 12) | (E131_PACKET_LENGTH - 16));
  packet->root_vector 	       = htonl(ROOT_VECTOR);
  memcpy(packet->cid,  uuid, 16);

  // set Framing Layer values
  packet->frame_flength        = htons((0x7 << 12) | (E131_PACKET_LENGTH - 38));
  packet->frame_vector 	       = htonl(FRAME_VECTOR);
  memcpy(packet->source_name,  src_name, src_len);
  packet->priority 	       = DEFAULT_PRIORITY;
  //packet->reserved             = 0x0000;
  //packet->sequence_number      = 0x00;
  packet->options 	       = FRAME_OPTIONS;
  //packet->universe 	       = htons(universe);

  // set Device Management Protocol (DMP) Layer values
  packet->dmp_flength 	       = htons((0x7 << 12) | (E131_PACKET_LENGTH - 115));
  packet->dmp_vector 	       = DMP_VECTOR;
  packet->type                 = DMP_TYPE;
  packet->first_address	       = htons(DMP_FIRST_ADDR);
  packet->address_increment    = htons(DMP_ADDR_INC);
  //packet->property_value_count = htons(num_channels + 1);

  return packet;
}

void ESPAsyncE131::setRGB(e131_packet_t *packet, uint8_t cnt, uint8_t r, uint8_t g, uint8_t b) {
  // first channel on [1]; [0] has to be 0
  uint16_t idx = cnt * 3;
  packet->property_values[idx+1] = r;
  packet->property_values[idx+2] = g;
  packet->property_values[idx+3] = b;
}

size_t ESPAsyncE131::sendPacket(e131_packet_t *packet, uint16_t universe, uint8_t cnt) {
  
  packet->universe = htons(universe);
  packet->property_value_count = htons(cnt * 3 + 1);
  packet->sequence_number = send_sequence_number++;

  IPAddress addr = IPAddress(239, 255, (((universe) >> 8) & 0xff), (((universe) >> 0) & 0xff));

  return udp.writeTo(reinterpret_cast<uint8_t *>(packet), E131_PACKET_LENGTH, addr, E131_DEFAULT_PORT);
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
    if (htonl(sbuff->root_vector) != ESPAsyncE131::ROOT_VECTOR)
        error = ERROR_VECTOR_ROOT;
    if (htonl(sbuff->frame_vector) != ESPAsyncE131::FRAME_VECTOR)
        error = ERROR_VECTOR_FRAME;
    if (sbuff->dmp_vector != ESPAsyncE131::DMP_VECTOR)
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
