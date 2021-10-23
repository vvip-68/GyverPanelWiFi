// Спецификация протокола E1.31 - https://tsp.esta.org/tsp/documents/docs/ANSI_E1-31-2018.pdf
#if (USE_E131 == 1)

void InitializeE131() {
  printWorkMode();
  if (workMode == MASTER) {    
    // E1.31 пакет требует уникальный идентификатор передающего устройства - сформировать его из MAC адреса и ChipId
    memset(e131_cid, '\0', 16);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    memmove(&e131_cid[10], mac, 6);
    #if defined(ESP8266)
      uint8_t raw[4];
      uint32_t num;
      num = htonl(ESP.getChipId());
      (uint32_t&)raw = num;
      memmove(&e131_cid[6], raw, 4);
      num = htonl(ESP.getFlashChipId());
      (uint32_t&)raw = num;
      memmove(&e131_cid[2], raw, 4);
    #elif defined(ESP32)    
      uint64_t num = ESP.getEfuseMac();
      uint8_t raw[8];
      (uint64_t&)raw = num;
      memmove(&e131_cid[2], raw, 8);
    #endif    
    // Инициализировать передатчик E1.31 пакетов
    e131 = new ESPAsyncE131();
    if (!e131) {  // Listen via Multicast
      DEBUGLN(F("Ошибка запуска вещателя E1.31 потока"));        
      DisposeE131();
    }
  } else
  if (workMode == SLAVE) {
    // Поддерживается до 7 вселенных. Одна группа синхронизации - 7 вселенных, т.е Группа 0 - 1..7 вселенная, Группа 1 - 8..14 вселенные и т.д.
    // 7 вселенных по 170 диодов = 1190 диодов
    START_UNIVERSE = (syncGroup * 7) + 1;
    // Одна вседенная содержит данные 170 RGB светодиодов - используется 170*3 = 510 каналов из доступных по спецификации E1.31 DMX 512 каналов на вселенную.
    UNIVERSE_COUNT = NUM_LEDS / 170;  
    if ((NUM_LEDS % 170) > 0) UNIVERSE_COUNT++;
    if (UNIVERSE_COUNT > 7) UNIVERSE_COUNT = 7;    

    last_fps_time = millis();
    
    // Инициализировать слушатель E1.31 пакетов
    e131 = new ESPAsyncE131(UNIVERSE_COUNT);
    if (!(e131 && e131->begin(E131_MULTICAST, START_UNIVERSE, UNIVERSE_COUNT))) {  // Listen via Multicast
      DEBUGLN(F("Ошибка запуска слушателя потока E1.31"));        
      DisposeE131();
    }

    flag_1 = false;
    flag_2 = false;
    
    DEBUGLN();
  }
} 

void DisposeE131() {
  if (e131) {
    delete e131;
    e131 = NULL;
    if (workMode == MASTER)
      DEBUGLN(F("Вещатель потока E1.31 остановлен."));
    else if (workMode == SLAVE)
      DEBUGLN(F("Cлушатель потока E1.31 остановлен."));
  }
}

void printE131packet(e131_packet_t *packet) {
  int16_t val;
  DEBUGLN(F("-------------------------------------------"));
  // Root Layer
  DEBUG(F("preamble_size="));         DEBUGLN("0x" + IntToHex(htons(packet->preamble_size),4));
  DEBUG(F("postamble_size="));        DEBUGLN("0x" + IntToHex(htons(packet->postamble_size),4));
  DEBUG(F("acn_id="));                for(int i=0; i<12; i++) DEBUG("0x" + IntToHex(packet->acn_id[i],2) + ", "); DEBUGLN();
  DEBUG(F("root_flength="));          val = htons(packet->root_flength); DEBUG(val & 0x0FFF); DEBUG(", Flag="); DEBUGLN("0x" + IntToHex((val & 0xF000) >> 12, 2));
  DEBUG(F("root_vector="));           DEBUGLN("0x" + IntToHex(htonl(packet->root_vector),8));
  DEBUG(F("cid="));                   for(int i=0; i<16; i++) DEBUG(IntToHex(packet->cid[i],2)); DEBUGLN();
  DEBUGLN();
  // Frame Layer
  DEBUG(F("frame_flength="));         val = htons(packet->frame_flength); DEBUG(val & 0x0FFF); DEBUG(", Flag="); DEBUGLN("0x" + IntToHex((val & 0xF000) >> 12, 2));
  DEBUG(F("frame_vector="));          DEBUGLN("0x" + IntToHex(htonl(packet->frame_vector),8));
  DEBUG(F("source_name="));           DEBUGLN("'" + String((char*)packet->source_name) + "'");
  DEBUG(F("priority="));              DEBUGLN(packet->priority);
  DEBUG(F("reserved="));              DEBUGLN(packet->reserved);
  DEBUG(F("sequence_number="));       DEBUGLN(packet->sequence_number);
  DEBUG(F("options="));               DEBUGLN(packet->options);
  DEBUG(F("universe="));              DEBUGLN(htons(packet->universe));
  DEBUGLN();
  // DMP Layer
  DEBUG(F("dmp_flength="));           val = htons(packet->dmp_flength); DEBUG(val & 0x0FFF); DEBUG(", Flag="); DEBUGLN("0x" + IntToHex((val & 0xF000) >> 12, 2));
  DEBUG(F("dmp_vector="));            DEBUGLN("0x" + IntToHex(packet->dmp_vector,2));
  DEBUG(F("type="));                  DEBUGLN("0x" + IntToHex(packet->type,2));
  DEBUG(F("first_address="));         DEBUGLN("0x" + IntToHex(htons(packet->first_address),4));
  DEBUG(F("address_increment="));     DEBUGLN("0x" + IntToHex(htons(packet->address_increment),4));
  DEBUG(F("property_value_count="));  DEBUGLN(htons(packet->property_value_count));
  DEBUG(F("property_values[0]="));    DEBUGLN(packet->property_values[0]);
  DEBUGLN(F("-------------------------------------------"));  
}

bool drawE131frame(e131_packet_t *packet, eSyncModes syncMode) {
  uint16_t CURRENT_UNIVERSE = htons(packet->universe);      
  uint16_t offset = (CURRENT_UNIVERSE - START_UNIVERSE) * 170; // if more than 170 LEDs (510 channels), client will send in next higher universe    
  if (offset >= NUM_LEDS) return false;
  uint8_t *data = packet->property_values + 1;
  if (syncMode == PHYSIC) {
    // Режим вывода PHYSIC - просто передаем весь полученный массив данных на ленту
    // Порядок следования светодиодов на матрице должен соответствовать порядку следования пикселей на MASTER-устройстве
    uint16_t len = (170 + offset > NUM_LEDS) ? (NUM_LEDS - offset) * 3 : 510;  
    memmove(&leds[offset], data, len);
  } else {
    // Режим вывода LOGIC - начало картинки X,Y = 0,0; Далее по строке слева направо, затем строки сверху вниз
    // Порядок следования светодиодов на матрице должен соответствовать порядку следования пикселей на MASTER-устройстве
    uint16_t len = (170 + offset > NUM_LEDS) ? (NUM_LEDS - offset) : 170;  
    for (int i = 0; i<len; i++) {
      uint16_t idx = offset + i;
      uint8_t x = idx % pWIDTH;
      uint8_t y = pHEIGHT - idx / pWIDTH - 1;
      uint16_t n = i * 3;
      CRGB color = CRGB(data[n], data[n+1], data[n+2]);
      drawPixelXY(x,y,color);
    }              
  }
  return true;
}

void printWorkMode() {
  if (workMode == STANDALONE) {
    DEBUGLN(F("\nРежим работы: АВТОНОМНЫЙ, синхронизация E1.31 отключена\n"));
  } else {
    START_UNIVERSE = (syncGroup * 7) + 1;
    UNIVERSE_COUNT = NUM_LEDS / 170;  
    if ((NUM_LEDS % 170) > 0) UNIVERSE_COUNT++;
    if (UNIVERSE_COUNT > 7) UNIVERSE_COUNT = 7;    
    uint8_t END_UNIVERSE = START_UNIVERSE + UNIVERSE_COUNT - 1;
    IPAddress address  = IPAddress(239, 255, ((START_UNIVERSE >> 8) & 0xff), ((START_UNIVERSE >> 0) & 0xff));
    IPAddress address2 = IPAddress(239, 255, ((END_UNIVERSE >> 8) & 0xff), ((END_UNIVERSE >> 0) & 0xff));

    if (workMode == MASTER) {
      DEBUG(F("\nРежим работы: ИСТОЧНИК; Тип потока - "));
      if (syncMode == PHYSIC)
        DEBUGLN(F("светодиоды в порядке подключения"));
      else if (syncMode == LOGIC)
        DEBUGLN(F("светодиоды в логическом порядке"));
      else
        DEBUGLN(F("команды"));
    } else

    if (workMode == SLAVE) {      
      DEBUG(F("\nРежим работы: ПРИЕМНИК; Тип потока - "));
      if (syncMode == PHYSIC)
        DEBUGLN(F("светодиоды в порядке подключения"));
      else if (syncMode == LOGIC)
        DEBUGLN(F("светодиоды в логическом порядке"));
      else
        DEBUGLN(F("команды"));
    }  

    DEBUG(F("Группа: "));
    DEBUG(syncGroup);
    DEBUG(F(" на "));
    DEBUG(address.toString());
    DEBUG(F(" - "));
    DEBUGLN(address2.toString());
  }
}

void sendE131Screen() {  
  if (!e131) return;
  if (!(syncMode == PHYSIC || syncMode == LOGIC)) return;

  e131_packet_t *packet = e131->createPacket(host_name.c_str(), e131_cid);
  if (!packet) return;
  
  if (syncMode == PHYSIC) {
    uint16_t cnt = 0, universe = START_UNIVERSE, num_leds = 0;
    for (uint16_t idx = 0; idx < NUM_LEDS; idx++) {
      CRGB color = leds[idx];
      e131->setRGB(packet,cnt,color.r,color.g,color.b);      
      cnt++;
      if (cnt == 170 || idx == NUM_LEDS-1) {
        // отправить пакет
        uint16_t len = e131->sendPacket(packet, universe, cnt);
        cnt = 0;
        universe++;
      }
    }
  } else

  if (syncMode == LOGIC) {
    uint16_t idx = 0, cnt = 0, universe = START_UNIVERSE;
    for (int8_t y = pHEIGHT-1; y >= 0; y--) {
      for (int8_t x = 0; x < pWIDTH; x++) {
        CRGB color = getPixColorXY(x,y);  
        e131->setRGB(packet,cnt,color.r,color.g,color.b);      
        idx++; cnt++;
        if (cnt == 170 || idx == NUM_LEDS) {
          // отправить пакет
          uint16_t len = e131->sendPacket(packet, universe, cnt);
          cnt = 0;
          universe++;
        }
      }
    }
  }

  // Освободить память, выделенную под пакет
  if (packet) free(packet);  
}
#endif
