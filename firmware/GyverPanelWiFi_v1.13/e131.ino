// Спецификация протокола E1.31 - https://tsp.esta.org/tsp/documents/docs/ANSI_E1-31-2018.pdf
#if (USE_E131 == 1)

#define CMD_TURNONOFF     0
#define CMD_BRIGHTNESS    1
#define CMD_SPCBRIGHTNESS 2
#define CMD_EFFECT        3
#define CMD_SPCEFFECT     4
#define CMD_RUNTEXT       5
#define CMD_STOPTEXT      6
#define CMD_TIME          7
#define CMD_SPEED         8
#define CMD_CONTRAST      9
#define CMD_PARAM1       10
#define CMD_PARAM2       11
#define CMD_COLOR        12
#define CMD_DIMENSION    13
#define CMD_TEXTSPEED    14
#define CMD_CLOCKSPEED   15

// ---------------------------------------------------
// Инициализация протокола E1.31 для устройства в роли приемника или передатчика
// ---------------------------------------------------
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
    if (!e131) {
      DEBUGLN(F("Ошибка запуска вещателя E1.31 потока"));        
      DisposeE131();
    }
  } else
  if (workMode == SLAVE) {
    // Поддерживается до MAX_UNIVERCE_COUNT = 7 вселенных. Одна группа синхронизации - 7 вселенных, т.е Группа 0 - 1..7 вселенная, Группа 1 - 8..14 вселенные и т.д.
    // 7 вселенных по 170 диодов = 1190 диодов
    START_UNIVERSE = (syncGroup * MAX_UNIVERCE_COUNT) + 1;
    // ------------
    // Одна вселенная содержит данные 170 RGB светодиодов - используется 170*3 = 510 каналов из доступных по спецификации E1.31 DMX 512 каналов на вселенную.
    UNIVERSE_COUNT = NUM_LEDS / 170;  
    if ((NUM_LEDS % 170) > 0) UNIVERSE_COUNT++;
    if (UNIVERSE_COUNT > MAX_UNIVERCE_COUNT) UNIVERSE_COUNT = MAX_UNIVERCE_COUNT;    
    // На маленькую матрицу может приходить картинка с большой, поэтому инициализировать слушатель нужно на максимально поддерживаемое 
    // количество вселенных в одной группе
    UNIVERSE_COUNT = MAX_UNIVERCE_COUNT; // !!!
    // ------------
    END_UNIVERSE = START_UNIVERSE + UNIVERSE_COUNT - 1;

    last_fps_time = millis();
    
    // Инициализировать слушатель E1.31 пакетов
    e131 = new ESPAsyncE131(UNIVERSE_COUNT);
    if (!(e131 && e131->begin(E131_MULTICAST, START_UNIVERSE, UNIVERSE_COUNT))) {  // Listen via Multicast
      DEBUGLN(F("Ошибка запуска слушателя потока E1.31"));        
      DisposeE131();
    }

    flag_1 = false;
    flag_2 = false;
    e131_wait_command = syncMode == COMMAND;
  }
} 

// ---------------------------------------------------
// Отключение протокола E1.31 с освобождением ресурсов
// ---------------------------------------------------
void DisposeE131() {
  if (e131) {
    delete e131;
    e131 = NULL;
    if (workMode == MASTER)
      DEBUGLN(F("Вещатель потока E1.31 остановлен."));
    else if (workMode == SLAVE)
      DEBUGLN(F("Слушатель потока E1.31 остановлен."));
  }
  e131_wait_command = false;
}

// ---------------------------------------------------
// Печать отладочной информации - структуры и содержимого пакета E1.31
// ---------------------------------------------------
void printE131packet(e131_packet_t *packet) {
  int16_t val;
  DEBUGLN(F("-------------------------------------------"));
  // Root Layer
  DEBUG(F("preamble_size="));         DEBUGLN("0x" + IntToHex(htons(packet->preamble_size),4));
  DEBUG(F("postamble_size="));        DEBUGLN("0x" + IntToHex(htons(packet->postamble_size),4));
  DEBUG(F("acn_id="));                for(uint8_t i=0; i<12; i++) DEBUG("0x" + IntToHex(packet->acn_id[i],2) + ", "); DEBUGLN();
  DEBUG(F("root_flength="));          val = htons(packet->root_flength); DEBUG(val & 0x0FFF); DEBUG(", Flag="); DEBUGLN("0x" + IntToHex((val & 0xF000) >> 12, 2));
  DEBUG(F("root_vector="));           DEBUGLN("0x" + IntToHex(htonl(packet->root_vector),8));
  DEBUG(F("cid="));                   for(uint8_t i=0; i<16; i++) DEBUG(IntToHex(packet->cid[i],2)); DEBUGLN();
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
  DEBUG(F("property_values="));       for(uint8_t i=0; i<32; i++) DEBUG("0x" + IntToHex(packet->property_values[i],2) + ", "); DEBUGLN();
  DEBUGLN(F("-------------------------------------------"));  
}

// ---------------------------------------------------
// Отображение информации из полученного пакета E1.31 на матрицу
// ---------------------------------------------------
int cnt = 0;  
bool drawE131frame(e131_packet_t *packet, eSyncModes syncMode) {
  uint16_t CURRENT_UNIVERSE = htons(packet->universe);      
  /*
    Serial.print(CURRENT_UNIVERSE);
    Serial.print(",");
    cnt++; 
    if (cnt>60) {
      Serial.println();
      cnt=0;
    }
  */  
  uint16_t offset = (CURRENT_UNIVERSE - START_UNIVERSE) * 170; // if more than 170 LEDs (510 channels), client will send in next higher universe    
  uint8_t *data = packet->property_values + 1;
  if (syncMode == PHYSIC) {
    if (offset >= NUM_LEDS) return false;
    // Режим вывода PHYSIC - просто передаем весь полученный массив данных на ленту
    // Порядок следования светодиодов на матрице должен соответствовать порядку следования пикселей на MASTER-устройстве
    uint16_t len = (170 + offset > NUM_LEDS) ? (NUM_LEDS - offset) * 3 : 510;  
    memmove(&leds[offset], data, len);
  } else {
    // Режим вывода LOGIC - начало картинки X,Y = 0,0; Далее по строке слева направо, затем строки сверху вниз
    // Порядок следования светодиодов на матрице должен соответствовать порядку следования пикселей на MASTER-устройстве
    uint8_t w = masterWidth == 0 ? pWIDTH : masterWidth;
    uint8_t h = masterHeight == 0 ? pHEIGHT : masterHeight;
    uint16_t numLeds = w * h;
    uint16_t len = (170 + offset > numLeds) ? (numLeds - offset) : 170;  
    int8_t offset_x = (pWIDTH - w) / 2;
    int8_t offset_y = (pHEIGHT - h) / 2;
    for (uint16_t i = 0; i < len; i++) {
      uint16_t idx = offset + i;
      int8_t  x = idx % w;
      int8_t  y = h - idx / w - 1;
      uint16_t n = i * 3;
      CRGB color = CRGB(data[n], data[n+1], data[n+2]);
      drawPixelXY(offset_x + x, offset_y + y, color);
    } 
  }
  return true;
}

// ---------------------------------------------------
// Вывод информации о режиме работы устройства / использовании протокола E1.31 в монитор порта
// ---------------------------------------------------
void printWorkMode() {
  DEBUG(F("\nГрупповая синхронизация по протоколу E1.31"));
  if (workMode == STANDALONE) {
    DEBUGLN(F("\nРежим работы: АВТОНОМНЫЙ, синхронизация E1.31 отключена\n"));
  } else {
    START_UNIVERSE = (syncGroup * MAX_UNIVERCE_COUNT) + 1;
    UNIVERSE_COUNT = NUM_LEDS / 170;  
    if ((NUM_LEDS % 170) > 0) UNIVERSE_COUNT++;
    if (UNIVERSE_COUNT > MAX_UNIVERCE_COUNT) UNIVERSE_COUNT = MAX_UNIVERCE_COUNT;
    
    // В режиме SLAVE на маленькую матрицу может приходить сигнал с большой, поэтому задействовать нужно все доступное для группы количество вселенных
    if (workMode == SLAVE) UNIVERSE_COUNT = MAX_UNIVERCE_COUNT;

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

// ---------------------------------------------------
// Отправка изображения с матрицы на устройства-получатели
// ---------------------------------------------------
void sendE131Screen() {  
  if (!e131) return;
  if (!(syncMode == PHYSIC || syncMode == LOGIC)) return;

  e131_packet_t* packet = NULL;
  
  if (syncMode == PHYSIC) {
    packet = e131->createPacket(host_name.c_str(), e131_cid);
    if (packet) {
      uint16_t cnt = 0, universe = START_UNIVERSE;
      for (uint16_t idx = 0; idx < NUM_LEDS; idx++) {
        CRGB color = leds[idx];
        e131->setRGB(packet,cnt,color.r,color.g,color.b);      
        cnt++;
        if (cnt == 170 || idx == NUM_LEDS-1) {
          // отправить пакет
          e131->sendPacket(packet, universe, cnt);
          cnt = 0;
          universe++;
        }
      }
    }
  } else

  if (syncMode == LOGIC) {
    // Отправить пакет с размерами MASTER-матрицы
    packet = makeCommandPacket(CMD_DIMENSION);
    if (packet) {
      packet->property_values[4] = pWIDTH;
      packet->property_values[5] = pHEIGHT;
      e131->sendPacket(packet, 1, 170);
      // Отправить экран MASTER-матрицы клиентам SLAVE
      uint16_t idx = 0, cnt = 0, universe = START_UNIVERSE;
      for (int8_t y = pHEIGHT-1; y >= 0; y--) {
        for (int8_t x = 0; x < pWIDTH; x++) {
          CRGB color = getPixColorXY(x,y);  
          e131->setRGB(packet,cnt,color.r,color.g,color.b);      
          idx++; cnt++;
          if (cnt == 170 || idx == NUM_LEDS) {
            // отправить пакет
            e131->sendPacket(packet, universe, cnt);
            cnt = 0;
            universe++;
          }
        }
      }
    }
  }

  // Освободить память, выделенную под пакет
  if (packet) free(packet);  
}

// ---------------------------------------------------
// Проверка, что пакет E1.31 содержит команду к исполнению
// ---------------------------------------------------
bool isCommandPacket(e131_packet_t *packet) {
  // Команда всегда приходит в universe == 1
  // 1 - 0xAA  - сигнатура 1
  // 2 - 0x55  - сигнатура 2
  // 3 -       - ID команды
  // 4 и далее - параметры команды (зависит от команды)
  return packet != NULL && htons(packet->universe) == 1 && packet->property_values[1] == 0xAA && packet->property_values[2] == 0x55;
}

// ---------------------------------------------------
// Обработать команду, полученную в пакете E1.31
// ---------------------------------------------------
void processCommandPacket(e131_packet_t *packet) {
  if (!packet) return;
  /*  
   Команда содержится в каналах:
     3 -       - ID команды
     4 и далее - параметры команды (зависит от команды)
  
   Команды:
     Канал 3 = 0 - Вкл/выкл устройства
     Канал 4 = x - 0 - выключить 1 - включить
     ---
     Канал 3 = 1 - Установить текущую яркость
     Канал 4 = x - Значение яркости
     ---
     Канал 3 = 1 - Установить текущую яркость специальных эффектов
     Канал 4 = x - Значение яркости
     ---
     Канал 3  = 3 - Включить эффект
     Канал 4  = x - ID эффекта
     Канал 5  = x - скорость эффекта
     Канал 6  = x - контраст эффекта
     Канал 7  = x - спец.параметр №1
     Канал 8  = x - спец.параметр №2
     Канал 9  = r  - значение R от globalColor
     Канал 10 = g  - значение G от globalColor
     Канал 11 = b  - значение B от globalColor
     Канал 12 = W  - ширина матрицы Мастера
     Канал 13 = H  - высота матрицы Мастера
     ---
     Канал 3 = 4 - Включить указанный специальный эффект
     Канал 4 = x - ID специального эффекта
     ---
     Канал 3 = 5 - Включить отображение текста, переданного в параметре
     Канал 4 = x - char* null-terminated текст
     ---
     Канал 3 = 6 - Остановить отображение бегущей строки, если оно выполняется
     ---
     Канал 3 = 7 - Установить время
     Канал 4 = x - год (без века) -  20xx
     Канал 5 = x - месяц
     Канал 6 = x - день
     Канал 7 = x - часы
     Канал 8 = x - минуты
     Канал 9 = x - секунды
     ---
     Канал 3 = 8 - Установить скорость эффекта
     Канал 4 = x - скорость
     ---
     Канал 3 = 9 - Установить контрастность эффекта
     Канал 4 = x - контрастность
     ---
     Канал 3 = 10 - Установить параметр 1 эффекта
     Канал 4 = x  - значение
     ---
     Канал 3 = 11 - Установить параметр 2 эффекта
     Канал 4 = x  - значение
     ---
     Канал 3 = 12 - Установить globalColor
     Канал 4 = r  - значение R
     Канал 5 = g  - значение G
     Канал 6 = b  - значение B
     ---
     Канал 3 = 13 - Ширина и высота панели Мастера
     Канал 4 = W  - ширина
     Канал 5 = H  - высота
  */  

  switch (packet->property_values[3]) {

    // вкл/выкл устройство
    case CMD_TURNONOFF:
      set_isTurnedOff(packet->property_values[4] == 0);
      //DEBUGLN("GOT CMD_TURNONOFF");
      break;

    // Установить яркость
    case CMD_BRIGHTNESS:
      set_globalBrightness(packet->property_values[4]);
      //DEBUGLN("GOT CMD_BRIGHTNESS");
      break;

    // Установить яркость
    case CMD_SPCBRIGHTNESS:
      set_specialBrightness(packet->property_values[4]);
      //DEBUGLN("GOT CMD_SPCBRIGHTNESS");
      break;

    // Включить эффект  
    case CMD_EFFECT:
      syncEffectSpeed    = packet->property_values[5];              // Скорость эффекта
      syncEffectContrast = packet->property_values[6];              // Контраст эффекта 
      syncEffectParam1   = packet->property_values[7];              // Параметр эффекта 1
      syncEffectParam2   = packet->property_values[8];              // Параметр эффекта 2
      masterWidth        = packet->property_values[12];
      masterHeight       = packet->property_values[13];
      globalColor = CRGB(packet->property_values[9],packet->property_values[10],packet->property_values[11]);
      setEffect(packet->property_values[4]);        
      //DEBUGLN("GOT CMD_EFFECT");
      break;

    // Включить специальный эффект  
    case CMD_SPCEFFECT:
      setSpecialMode(packet->property_values[4]);
      //DEBUGLN("GOT CMD_SPCEFFECT");
      break;

    // Включить отображение текста, переданного в параметре 4 и далее
    case CMD_RUNTEXT: {
      // property_values[4] - char* null terminated string 
      char* buf = (char*)(&packet->property_values[4]);
      String text = String(buf);
      setImmediateText(text);
      //DEBUGLN("GOT CMD_RUNTEXT");
      break;
     }

    // Остановить бегущую строку, если она отображается
    case CMD_STOPTEXT:
      // Если ширина матрицы на мастере отличается от ширины устройства -
      // текст принудительно останавливать не нужно - на мастере он уже может пробежать,
      // а на более широкой матрице - все еще отображаться      
      mandatoryStopText = masterWidth > 0 && pWIDTH <= masterWidth;
      //DEBUGLN("GOT CMD_STOPTEXT");
      break;

    case CMD_TIME:
      // Установка текущего времени
      //   Канал 4 = x - год (без века) -  20xx
      //   Канал 5 = x - месяц
      //   Канал 6 = x - день
      //   Канал 7 = x - часы
      //   Канал 8 = x - минуты
      //   Канал 9 = x - секунды
      setCurrentTime(packet->property_values[7],packet->property_values[8],packet->property_values[9],
                     packet->property_values[6],packet->property_values[5],2000+packet->property_values[4]);
      //DEBUGLN("GOT CMD_TIME");
      break;      

    case CMD_SPEED:
      // Скорость текущего эффекта
      syncEffectSpeed    = packet->property_values[4];              // Скорость эффекта
      setTimersForMode(thisMode);
      //DEBUGLN("GOT CMD_SPEED");
      break;      

    case CMD_CONTRAST:
      // Контрастность текущего эффекта
      syncEffectContrast = packet->property_values[4];              // Контраст эффекта 
      //DEBUGLN("GOT CMD_CONTRAST");
      break;      

    case CMD_PARAM1:
      // Параметр-1 текущего эффекта
      syncEffectParam1   = packet->property_values[4];              // Параметр эффекта 1
      //DEBUGLN("GOT CMD_PARAM1");
      break;      

    case CMD_PARAM2:
      // Параметр-2 текущего эффекта
      syncEffectParam2   = packet->property_values[4];              // Параметр эффекта 2
      //DEBUGLN("GOT CMD_PARAM2");
      break;      

    case CMD_COLOR:
      // globalColor
      globalColor = CRGB(packet->property_values[4],packet->property_values[5],packet->property_values[6]);
      //DEBUGLN("GOT CMD_COLOR");
      break;      

    case CMD_DIMENSION:
    {
      // Ширина и высота MASTER - матрицы
      uint8_t ww = packet->property_values[4];
      uint8_t hh = packet->property_values[5];
      if (masterWidth != ww || masterHeight != hh) {
        masterWidth  = ww;
        masterHeight = hh;
        FastLED.clear();
      }
      //DEBUGLN("GOT CMD_DIMENSION");
      break;      
    }
    
    case CMD_TEXTSPEED:
      // Скорость прокрутки текста
      textScrollSpeed = packet->property_values[4];
      textTimer.setInterval(textScrollSpeed);
      //DEBUGLN("GOT CMD_TEXTSPEED");
      break;      

    case CMD_CLOCKSPEED:
      // Скорость прокрутки текста
      clockScrollSpeed = packet->property_values[4];
      if (clockScrollSpeed >= 240) {
        clockTimer.stopTimer();
        checkClockOrigin();
      } else {
        clockTimer.setInterval(clockScrollSpeed);
      }
      //DEBUGLN("GOT CMD_CLOCKSPEED");
      break;      
  }
}

// ---------------------------------------------------
// Отправка команд в пакете E1.31 клиентам
// ---------------------------------------------------

e131_packet_t* makeCommandPacket(uint8_t cmd) {
  e131_packet_t *packet = e131->createPacket(host_name.c_str(), e131_cid);
  if (packet) {
    packet->property_values[1] = 0xAA;
    packet->property_values[2] = 0x55;
    packet->property_values[3] = cmd;
  }
  return packet;
}

void commandTurnOnOff(bool value) {
  if (workMode != MASTER) return;
  e131_packet_t *packet = makeCommandPacket(CMD_TURNONOFF);
  if (!packet) return;
  packet->property_values[4] = value ? 1 :0;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);  
  //DEBUGLN("SEND CMD_TURNONOFF");
}

void commandSetBrightness(uint8_t value) {
  if (workMode != MASTER) return;
  e131_packet_t *packet = makeCommandPacket(CMD_BRIGHTNESS);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_BRIGHTNESS");
}

void commandSetSpecialBrightness(uint8_t value) {
  if (workMode != MASTER) return;
  e131_packet_t *packet = makeCommandPacket(CMD_SPCBRIGHTNESS);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_SPCBRIGHTNESS");
}

void commandSetImmediateText(String str) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_RUNTEXT);
  if (!packet) return;
  str.getBytes(&packet->property_values[4], str.length() + 1);  
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_RUNTEXT");
}

void commandStopText() {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_STOPTEXT);
  if (!packet) return;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);      
  //DEBUGLN("SEND CMD_STOPTEXT");
}

void commandSetCurrentTime(uint8_t hh, uint8_t mm, uint8_t ss, uint8_t dd, uint8_t nn, uint16_t yy) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_SPCBRIGHTNESS);
  if (!packet) return;
  packet->property_values[4] = hh;
  packet->property_values[5] = mm;
  packet->property_values[6] = ss;
  packet->property_values[7] = dd;
  packet->property_values[8] = nn;
  packet->property_values[9] = yy % 100;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_SPCBRIGHTNESS");
}

void commandSetMode(int8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_EFFECT);
  if (!packet) return;
  packet->property_values[4]  = value;
  packet->property_values[5]  = effectSpeed[value];
  packet->property_values[6]  = effectContrast[value];
  packet->property_values[7]  = effectScaleParam[value];
  packet->property_values[8]  = effectScaleParam2[value];

  CRGB color = CRGB(globalColor);
  packet->property_values[9]  = color.r;
  packet->property_values[10] = color.g;
  packet->property_values[11] = color.b;

  packet->property_values[12] = pWIDTH;
  packet->property_values[13] = pHEIGHT;
  
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_EFFECT");
}

void commandSetSpecialMode(int8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_SPCEFFECT);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_SPCEFFECT");
}

void commandSetEffectSpeed(uint8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_SPEED);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_SPEED");
}

void commandSetEffectContrast(uint8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_CONTRAST);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_CONTRAST");
}

void commandSetEffectParam(uint8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_PARAM1);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_PARAM1");
}

void commandSetEffectParam2(uint8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_PARAM2);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_PARAM2");
}

void commandSetGlobalColor(uint32_t col) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_COLOR);
  if (!packet) return;
  CRGB color = CRGB(col);
  packet->property_values[4] = color.r;
  packet->property_values[5] = color.g;
  packet->property_values[6] = color.b;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_COLOR");
}

void commandSetDimension(uint8_t w, uint8_t h) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_DIMENSION);
  if (!packet) return;
  packet->property_values[4] = w;
  packet->property_values[5] = h;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);    
  //DEBUGLN("SEND CMD_DIMENSION");  
}

void commandSetTextSpeed(uint8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_TEXTSPEED);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);      
  //DEBUGLN("SEND CMD_TEXTSPEED");  
}

void commandSetClockSpeed(uint8_t value) {
  if (!(workMode == MASTER && syncMode == COMMAND)) return;
  e131_packet_t *packet = makeCommandPacket(CMD_CLOCKSPEED);
  if (!packet) return;
  packet->property_values[4] = value;
  e131->sendPacket(packet, 1, 170);
  delay(5);
  free(packet);      
  //DEBUGLN("SEND CMD_CLOCKSPEED");
}
#endif
