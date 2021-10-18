#if (USE_E131 == 1)

void InitializeE131() {
  DisposeE131();
  if (workMode == SLAVE) {
    // По спецификации E1.31 DMX MULTICAST поддерживается до 7 вселенных. Одна группа синхронизации - 7 вселенныхб т.е Группа 0 - 1..7 вселенная, Группа 1 - 8..14 вселенные и т.д.
    START_UNIVERSE = (syncGroup * 7) + 1;
    // Одна вседенная содержит данные 170 RGB светодиодов - используется 170*3 = 510 каналов из доступных по спецификации E1.31 DMX 512 каналов на вселенную.
    UNIVERSE_COUNT = NUM_LEDS / 170 + 1;  
    if (UNIVERSE_COUNT > 7) UNIVERSE_COUNT = 7;    
    uint8_t END_UNIVERSE = START_UNIVERSE + UNIVERSE_COUNT - 1;

    IPAddress address  = IPAddress(239, 255, ((START_UNIVERSE >> 8) & 0xff), ((START_UNIVERSE >> 0) & 0xff));
    IPAddress address2 = IPAddress(239, 255, ((END_UNIVERSE >> 8) & 0xff), ((END_UNIVERSE >> 0) & 0xff));
    DEBUG(F("\nЗапуск слушателя пакетов E1.31\nГруппа: "));
    DEBUG(syncGroup);
    DEBUG(F(" на "));
    DEBUG(address.toString());
    DEBUG(F(" - "));
    DEBUGLN(address2.toString());

    last_fps_time = millis();
    
    // Инициализировать слушатель E1.31 пакетов
    e131 = new ESPAsyncE131(UNIVERSE_COUNT);
    if (e131 != NULL) {
      if (e131->begin(E131_MULTICAST, START_UNIVERSE, UNIVERSE_COUNT)) {  // Listen via Multicast
          DEBUGLN(F("Ожидание поступления данных E1.31..."));          
      } else { 
          DEBUGLN(F("Ошибка запуска слушателя E1.31"));        
          DisposeE131();
      }
    }

    flag_1 = false;
    flag_2 = false;
    
    DEBUGLN();
  }
} 

void DisposeE131() {
  if (e131 != NULL) {
    delete[] e131;
    e131 = NULL;
    DEBUGLN(F("Cлушатель E1.31 остановлен."));        
  }
}

void printE131packet(e131_packet_t *packet) {
  int16_t val;
  // Root Layer
  DEBUGLN("-------------------------------------------");
  DEBUG("preamble_size=");         DEBUGLN("0x" + IntToHex(htons(packet->preamble_size),4));
  DEBUG("postamble_size=");        DEBUGLN("0x" + IntToHex(htons(packet->postamble_size),4));
  DEBUG("acn_id=");                for(int i=0; i<12; i++) DEBUG("0x" + IntToHex(packet->acn_id[i],2) + ", "); DEBUGLN();
  DEBUG("root_flength=");          val = htons(packet->root_flength); DEBUG(val & 0x0FFF); DEBUG(", Flag="); DEBUGLN("0x" + IntToHex((val & 0xF000) >> 12, 2));
  DEBUG("root_vector=");           DEBUGLN("0x" + IntToHex(htonl(packet->root_vector),8));
  DEBUG("cid=");                   for(int i=0; i<16; i++) DEBUG((char)packet->cid[i]); DEBUGLN();
  DEBUGLN();
  // Frame Layer
  DEBUG("frame_flength=");         val = htons(packet->frame_flength); DEBUG(val & 0x0FFF); DEBUG(", Flag="); DEBUGLN("0x" + IntToHex((val & 0xF000) >> 12, 2));
  DEBUG("frame_vector=");          DEBUGLN("0x" + IntToHex(htonl(packet->frame_vector),8));
  DEBUG("source_name=");           DEBUGLN("'" + String((char*)packet->source_name) + "'");
  DEBUG("priority=");              DEBUGLN(packet->priority);
  DEBUG("reserved=");              DEBUGLN(packet->reserved);
  DEBUG("sequence_number=");       DEBUGLN(packet->sequence_number);
  DEBUG("options=");               DEBUGLN(packet->options);
  DEBUG("universe=");              DEBUGLN(htons(packet->universe));
  DEBUGLN();
  // DMP Layer
  DEBUG("dmp_flength=");           val = htons(packet->dmp_flength); DEBUG(val & 0x0FFF); DEBUG(", Flag="); DEBUGLN("0x" + IntToHex((val & 0xF000) >> 12, 2));
  DEBUG("dmp_vector=");            DEBUGLN("0x" + IntToHex(packet->dmp_vector,2));
  DEBUG("type=");                  DEBUGLN("0x" + IntToHex(packet->type,2));
  DEBUG("first_address=");         DEBUGLN("0x" + IntToHex(htons(packet->first_address),4));
  DEBUG("address_increment=");     DEBUGLN("0x" + IntToHex(htons(packet->address_increment),4));
  DEBUG("property_value_count=");  DEBUGLN(htons(packet->property_value_count));
  DEBUG("property_values[0]=");    DEBUGLN(packet->property_values[0]);
  DEBUGLN("-------------------------------------------");  
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

#endif
