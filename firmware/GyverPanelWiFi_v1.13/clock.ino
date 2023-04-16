/*
Модуль формирования отображения часов, погоды и календаря

 Спасибо Дмитрию (Dimansion) 
 GitHub: https://github.com/7918514; Почта: dimon7000@mail.ru; Telegram: @dim_msk
 за тщательное и въедливое тестирование для огромного количества комбинаций размеров матрицы,
 отображения дат и времени, режимов отображения часов и всего, что с этим связано.

 Надеюсь, что благодаря его стараниям ошибок в этой части кода больше не осталось. 
*/

// ****************** НАСТРОЙКИ ЧАСОВ *****************
#define MIN_COLOR CRGB::White          // цвет минут
#define HOUR_COLOR CRGB::White         // цвет часов
#define DOT_COLOR CRGB::White          // цвет точек

#define NORMAL_CLOCK_COLOR CRGB::White // Нормальный цвет монохромных цветов

#define CONTRAST_COLOR_1 CRGB::Orange  // контрастный цвет часов
#define CONTRAST_COLOR_2 CRGB::Green   // контрастный цвет часов
#define CONTRAST_COLOR_3 CRGB::Yellow  // контрастный цвет часов

#define HUE_STEP 5          // шаг цвета часов в режиме радужной смены
#define HUE_GAP 30          // шаг цвета между цифрами в режиме радужной смены

// ****************** ДЛЯ РАЗРАБОТЧИКОВ ****************
uint8_t clockHue;

CRGB clockLED[5] = {HOUR_COLOR, HOUR_COLOR, DOT_COLOR, MIN_COLOR, MIN_COLOR};

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {

  // Пока включена отладка позиционирования часов - запросы на текущее время не выполнять
  if (debug_hours >= 0 && debug_mins >= 0) return;
  
  DEBUG(F("Отправка NTP пакета на сервер "));
  DEBUGLN(ntpServerName);
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write((const uint8_t*) packetBuffer, NTP_PACKET_SIZE);  
  udp.endPacket();
  udp.flush();
  delay(0);
}

void parseNTP() {
  getNtpInProgress = false;
  DEBUGLN(F("Разбор пакета NTP"));
  unsigned long highWord = word(incomeBuffer[40], incomeBuffer[41]);
  unsigned long lowWord = word(incomeBuffer[42], incomeBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;    
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  unsigned long seventyYears = 2208988800UL ;
  unsigned long t = secsSince1900 - seventyYears + (timeZoneOffset) * 3600UL;
  String t2 = getDateTimeString(t);

  DEBUG(F("Секунд с 1970: "));
  DEBUGLN(t);
  DEBUG(F("Получено время: ")); 
  DEBUGLN(t2);

  // Замечена ситуация, когда полученное время при корректировке слишком кардинально отличается
  // от текущего установленного времени и после применения часы "сбиваются" на несеолько часов
  // И остаются в таком состоянии до следующей корректировки.
  // Если время полученное отличается от текущего более чем на 60 минут 30 секунд - считаем корректировку недостоверной
  // Часы не могут так быстро уходить вперед или отставать за период между корректировками
  // Исключение - момент перехода с летнего на зимнее время и обратно
  if (init_time && abs((long)(now() - t)) > 3630) {
    DEBUG(F("Текущее время: ")); 
    DEBUGLN(getDateTimeString(now()));
    DEBUGLN(F("Полученное время слишком отличается от текущего.")); 
    DEBUGLN(F("Время признано недостоверным. Корректировка отменена")); 
    return;
  }

  setTime(t);  
  // этот вызов нужен, чтобы отработали сопутствующие установке времени процедуры
  setCurrentTime(hour(),minute(),second(),day(),month(),year());

  // Если время запуска еще не определено - инициализировать его
  if (upTime == 0) {
    upTime = t - millis() / 1000L;
  }

  ntp_t = 0; ntp_cnt = 0; init_time = true; refresh_time = false;

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(256);
  String out;
  doc["act"] = F("TIME");
  doc["server_name"] = ntpServerName;
  doc["server_ip"] = timeServerIP.toString();
  doc["result"] = F("OK");
  doc["time"] = secsSince1900;
  doc["s_time2"] = t2;
  serializeJson(doc, out);      
  SendMQTT(out, TOPIC_TME);
  #endif
}

void getNTP() {
  if (!wifi_connected) return;
  WiFi.hostByName(ntpServerName, timeServerIP);
  IPAddress ip1, ip2;
  ip1.fromString(F("0.0.0.0"));
  ip2.fromString(F("255.255.255.255"));
  if (timeServerIP == ip1 || timeServerIP == ip2) {
    DEBUG(F("Не удалось получить IP aдрес сервера NTP -> "));
    DEBUG(ntpServerName);
    DEBUG(F(" -> "));
    DEBUGLN(timeServerIP);
    timeServerIP.fromString(F("85.21.78.91"));  // Один из ru.pool.ntp.org  // 91.207.136.55, 91.207.136.50, 46.17.46.226
    DEBUG(F("Используем сервер по умолчанию: "));
    DEBUGLN(timeServerIP);
  }
  getNtpInProgress = true;
  printNtpServerName();  
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  ntp_t = millis();  

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(256);
  String out;
  doc["act"] = F("TIME");
  doc["server_name"] = ntpServerName;
  doc["server_ip"] = timeServerIP.toString();
  doc["result"] = F("REQUEST");
  serializeJson(doc, out);
  SendMQTT(out, TOPIC_TME);
  #endif
}

String clockCurrentText() {
  
  hrs = hour();
  mins = minute();

  String sHrs = "0" + String(hrs);  
  String sMin = "0" + String(mins);
  if (sHrs.length() > 2) sHrs = sHrs.substring(1);
  if (sMin.length() > 2) sMin = sMin.substring(1);
  return sHrs + ":" + sMin;
}

String dateCurrentTextShort() {
  
  aday = day();
  amnth = month();
  ayear = year();

  String sDay = "0" + String(aday);  
  String sMnth = "0" + String(amnth);
  String sYear = String(ayear);
  if (sDay.length() > 2) sDay = sDay.substring(1);
  if (sMnth.length() > 2) sMnth = sMnth.substring(1);
  return sDay + "." + sMnth + "." + sYear;
}

String dateCurrentTextLong() {
  
  aday = day();
  amnth = month();
  ayear = year();

  String sDay = "0" + String(aday);  
  String sMnth = getMonthString(amnth);
  String sYear = String(ayear);
    
  if (sDay.length() > 2) sDay = sDay.substring(1);
  return sDay + " " + sMnth + " " + sYear + " года";
}

void clockColor() {
  
  int8_t color_idx = 0;
  switch (thisMode) {
    case MC_CLOCK:
      // Если режим часов включен как "Ночные часы" - цвет определять по настроенному цвету ночных часов
      // Иначе - цвет часов зависит от режима (Монохром, Каждая цифра свой цвет, Цвет часов-точек-минут" и т.д)
      color_idx = isNightClock ? -2 : COLOR_MODE;
      break;
    case MC_COLORS:
    case MC_FILL_COLOR:
      color_idx = -1;
      break;
    default:
      color_idx = COLOR_MODE;
      break;
  }
    
  if (color_idx == -2) {
    // Цвет по индексу настроек текущего ночного цвета     
    CRGB color = getNightClockColorByIndex(nightClockColor);
    for (uint8_t i = 0; i < 5; i++) clockLED[i] = color;
  } else if (color_idx == -1) {     
    // Инверсный от основного цвет
    CRGB color = globalColor == 0xFFFFFF
      ? color = CRGB::Navy
      : -CRGB(globalColor);
    for (uint8_t i = 0; i < 5; i++) clockLED[i] = color;  
  } else if (color_idx == 0) {     
    // Монохромные часы  
    uint8_t hue = effectScaleParam[MC_CLOCK];
    CHSV color = hue <= 1 ? CHSV(255, 0, 255): CHSV(hue, 255, 255);
    for (uint8_t i = 0; i < 5; i++) clockLED[i] = color;
  } else if (color_idx == 1) {
    // Каждая цифра своим цветом, плавная смена цвета
    for (uint8_t i = 0; i < 5; i++) clockLED[i] = CHSV(clockHue + HUE_GAP * i, 255, 255);
    clockLED[2] = CHSV(clockHue + 128 + HUE_GAP * 1, 255, 255); // точки делаем другой цвет
  } else if (color_idx == 2) {
    // Часы, точки, минуты своим цветом, плавная смена цвета
    clockLED[0] = CHSV(clockHue + HUE_GAP * 0, 255, 255);
    clockLED[1] = CHSV(clockHue + HUE_GAP * 0, 255, 255);
    clockLED[2] = CHSV(clockHue + 128 + HUE_GAP * 1, 255, 255); // точки делаем другой цвет
    clockLED[3] = CHSV(clockHue + HUE_GAP * 2, 255, 255);
    clockLED[4] = CHSV(clockHue + HUE_GAP * 2, 255, 255);
  } else {
    CRGB color = getGlobalClockColor();
    for (uint8_t i = 0; i < 5; i++) clockLED[i] = color;
  }
}

uint8_t getClockSizeType() {
  uint8_t clock_size = CLOCK_SIZE;
  // Если часы авто или большие - определить - а поместятся ли они на матрицу по ширине при горизонтальном режиме / по высоте при вертикальном
  // Большие часы для шрифта 5x7 требуют 4*5 /цифры/ + 4 /двоеточие/ + 2 /пробел между цифрами часов и минут / = 23, 25 или 26 колонки (одинарные / двойные точки в часах) если 23 - нет пробела вокруг точек
  if ((clock_size == 0 || clock_size == 2) && ((CLOCK_ORIENT == 0 && pWIDTH < 23) || (CLOCK_ORIENT == 1 && pHEIGHT < 15))) clock_size = 1;
  if (clock_size == 0) clock_size = 2;
  return clock_size;
}

// Вычисление позиции отрисовки пикселя для часов, сдвигающихся по кругу.
int8_t getClockX(int8_t x) {
#if (DEVICE_TYPE == 0)
  return x<0 ? pWIDTH + x : x;
#else
  return x;
#endif
}

// нарисовать часы
void drawClock(uint8_t hrs, uint8_t mins, bool dots, int8_t X, int8_t Y) {

  // Для отладки позиционирования часов с температурой
  bool debug = debug_hours >= 0 && debug_mins >= 0;
  if (debug) {
    hrs = debug_hours; mins = debug_mins; 
    #if (USE_WEATHER == 1)
    temperature = debug_temperature;
    #endif
  }

  int8_t x = X;

  uint8_t h10 = hrs / 10;
  uint8_t h01 = hrs % 10;
  uint8_t m10 = mins / 10;
  uint8_t m01 = mins % 10;

  #if (USE_WEATHER == 1)
  uint8_t t = abs(temperature);
  uint8_t dec_t = t / 10;
  #endif
  
  // Старший байт - ширина часов, младший - ширина температуры
  // Выбираем бОльшую из значений ширины и центрируем ее относительно ширины матрицы
  uint16_t ww = getClockWithTempWidth(hrs, mins);
  uint8_t  cw = ((ww & 0xff00) >> 8);
  uint8_t  tw = ( ww & 0x00ff);
  int8_t   dx = tw - cw;  

  if (tw > cw) cw = tw;
  CLOCK_W = cw;

  x = round((pWIDTH - CLOCK_W) / 2.0) + (dx > 0 ? dx : 0);
  while(dx == 0 && x > 0 && x + CLOCK_W >= pWIDTH) x--;
  if (clockScrollSpeed >= 240) {
    X = 0;
    if (CLOCK_W >= 23 && pWIDTH <= 25) X++;
  }

  CLOCK_FX = x - (dx > 0 ? dx : 0);
  
  if (CLOCK_ORIENT == 0) {
    if (dx < 0) {
      if (h10 == 1 && h01 == 1) x++;
      if (h10 == 1 && h01 == 1 && m10 == 1 && m01 == 1) CLOCK_FX++;
      if (h10 == 1 && h01 == 1 && c_size == 1) x--;
      if (h10 == 1 && h01 == 1 && m10 == 1 && m01 == 1 && c_size == 1) CLOCK_FX--;
    } else if (dx > 0) {
      if (h10 == 1 && h01 == 1 && (m10 != 1 || m01 != 1)) x++;
    }
    if (c_size != 1 && pWIDTH >= 23 && pWIDTH <= 25) {
      CLOCK_FX--;
      #if (USE_WEATHER == 1)
      if (dec_t > 0 && m10 != 1 && m01 == 1) {
        x--;
      }
      #endif
    }
  }
  if (debug) {
    drawPixelXY(getClockX(X + CLOCK_FX), Y - 1, CRGB::Green);      
  }

  if (CLOCK_ORIENT == 0) {

    if (c_size == 1) {
      
      // Часы занимают 4 знакоместа в 3x5 + 3 разделительных колонки, итого 15 точек по горизонтали
      // Однако реально они могут занимать меньше места, если в часах/минутах есть 11. Экстремальное значение 1:11 занимает 10 точек и также должно быть отцентрировано в этих 15 точках
      // Центр композиции - разделительное двоеточие
      // Если десятки часов = 1 - смещаем начала отрисовки на 1 вправо 
      // Если единицы часов = 1 - смещаем начала отрисовки на 1 вправо
      // если десятки часов - 0 - не рисуется, смещаем начало отрисовки на 3 вправо

      // 0 в часах не выводим, для центрирования сдвигаем остальные цифры влево на место нуля
      if (h10 > 0) {
        x += (h10 == 1 ? -1 : 0);
        drawDigit3x5(h10, getClockX(X + x), Y, clockLED[0]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
        x += (h10 == 1 ? 3 : 4);
      }      

      x += (h10 == 0 && h01 == 1 ? -1 : 0);
      drawDigit3x5(h01, getClockX(X + x), Y, clockLED[1]);
      x += 3;
      
      if (dots) {
        drawPixelXY(getClockX(X + x), Y + 1, clockLED[2]);
        drawPixelXY(getClockX(X + x), Y + 3, clockLED[2]);
      }
      x++;

      drawDigit3x5(m10, getClockX(X + x), Y, clockLED[3]);      

      x += (m01 == 1 ? 3 : (m10 == 1 ? 3 : 4));
      drawDigit3x5(m01, getClockX(X + x) , Y, clockLED[4]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать влево на 1 колонку
      
      x += (m01 == 1 ? 1 : 2);
      CLOCK_LX = x;     // колонка в которой находится правая точка часов
      if (debug) {
        drawPixelXY(getClockX(X + CLOCK_LX), Y - 1, CRGB::Red);
      }

    } else {
      
      // отрисовка часов 5x7
      x += (h10 == 1 && h01 == 1 ? -1 : 0);
      x += (h10 == 1 && h01 == 1 && m10 == 1 && m01 == 1 ? 1 : 0);

      // Если скроллинг часов отключим - центрируем конгломерат часов и температуры на матрице
      // Если скроллинг есть - рисуем как есть в переданной позиции X
      if (clockScrollSpeed >= 240) X = 0;      
              
      // 0 в часах не выводим, для центрирования сдвигаем остальные цифры влево на место нуля
      if (h10 > 0) {
        x += (h10 == 1 ? -1 : 0);        
        drawDigit5x7(h10, getClockX(X + x), Y, clockLED[0]); // шрифт 5x7 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
        x += (h10 == 1 ? (h01 == 1 ? 6 : 5) : 6);
      }      

      x += (h01 == 1 ? -1 : 0); 
      drawDigit5x7(h01, getClockX(X + x), Y, clockLED[1]);      
      x += (h01 == 1 ? 5 : 6);
      
      // Если матрица 23 или 24 - пробел ДО одинарных точек не рисовать)
      if (pWIDTH < 25) x--;
              
      if (dots) {
        // Для ширины матрицы в 25 колонок - рисовать одинарные точки разделения часов/минут, если больше - сдвоенные
        drawPixelXY(getClockX(X + x), Y + 1, clockLED[2]);         
        drawPixelXY(getClockX(X + x), Y + 2, clockLED[2]);
        drawPixelXY(getClockX(X + x), Y + 4, clockLED[2]);
        drawPixelXY(getClockX(X + x), Y + 5, clockLED[2]);
        x++;
        if (pWIDTH > 25) {
          drawPixelXY(getClockX(X + x), Y + 1, clockLED[2]);
          drawPixelXY(getClockX(X + x), Y + 2, clockLED[2]);
          drawPixelXY(getClockX(X + x), Y + 4, clockLED[2]);
          drawPixelXY(getClockX(X + x), Y + 5, clockLED[2]);
          x++;
        }
      } else {
        x += (pWIDTH > 25 ? 2 : 1);
      }

      // Если матрица 23 или 24 - пробел ПОСЛЕ одинарных точек не рисовать), цифры сдвинуть влево
      if (pWIDTH < 25) x--;

      // Для часов шириной 25 точек (одинарные разделительные точки) смещать вывод минут на одну позицию влево
      x += (m10 == 1 ? 0 : 1);      
      drawDigit5x7(m10, getClockX(X + x), Y, clockLED[3]);
      x += 6;
      
      x += (m10 == 1 ? -1 : 0);      
      drawDigit5x7(m01, getClockX(X + x), Y, clockLED[4]);  // шрифт 5x7 в котором 1 - по центру знакоместа - смещать влево на 1 колонку

      x += (m01 == 1 ? 3 : 4);
      CLOCK_LX = x;     // Колонка в которой находится правая точка часов
      if (debug) {
        drawPixelXY(getClockX(X + CLOCK_LX), Y - 1, CRGB::Red);      
      }

    }
  } else { // Вертикальные часы

    if (c_size == 1) {
      
      // Малые часы
      drawDigit3x5(h10, X + x, Y + 6, clockLED[0]);
      drawDigit3x5(h01, X + x + 4, Y + 6, clockLED[1]);
      if (dots) { // Мигающие точки легко ассоциируются с часами
        drawPixelXY(getClockX(X + x + 3), Y + 5, clockLED[2]);
      }
      drawDigit3x5(m10, X + x, Y, clockLED[3]);
      drawDigit3x5(m01, X + x + 4, Y, clockLED[4]);

      CLOCK_LX = x + 6 ;     // Колонка в которой находится правая точка часов
      if (debug) {
        drawPixelXY(getClockX(X + CLOCK_LX), Y - 1, CRGB::Red);
      }

    } else {

      // Большие часы
      drawDigit5x7(h10, X + x, Y + 8, clockLED[0]);
      drawDigit5x7(h01, X + x + 6, Y + 8, clockLED[1]);
      if (dots) { // Мигающие точки легко ассоциируются с часами
        for (uint8_t i=0; i<3; i++) drawPixelXY(getClockX(X + x + 4 + i), Y + 7, clockLED[2]);
      }
      drawDigit5x7(m10, X + x, Y, clockLED[3]);
      drawDigit5x7(m01, X + x + 6, Y, clockLED[4]);

      CLOCK_LX = x + 10;     // Колонка в которой находится правая точка часов
      if (debug) {
        drawPixelXY(getClockX(X + CLOCK_LX), Y - 1, CRGB::Red);
      }

    }    
  }
}

// Вычисляет ширину, требуемую для отображения текущей температуры и часов.
// Возврат - старший байт - ширина часов, младший байт - ширина температуры
uint16_t getClockWithTempWidth(uint8_t hrs, uint8_t mins) {
  uint8_t cw= 0;
  uint8_t tw = 0;  

  uint8_t h10 = hrs / 10;
  uint8_t h01 = hrs % 10;
  uint8_t m10 = mins / 10;
  uint8_t m01 = mins % 10;  

#if (USE_WEATHER == 1)      
  uint8_t t = abs(temperature);
  uint8_t dec_t = t / 10;
  uint8_t edc_t = t % 10;

  // Ширина отображения температуры
  // На вертикальных часах температура не показывается
  if (CLOCK_ORIENT == 0 && useWeather > 0) {
    if (t == 0) {
      tw = c_size == 1 ? 6 : 15;     // малые - 0c; большие - 0°C "0" и "C" - по 5 точек + "°" - 3 точки + два пробела между символами
    } else {
      if (c_size == 1) {
        tw = 11;      // 2 цифры по 3 точек + "-/+" - 3 точки + 2 пробела
        if (dec_t == 0) tw -= 4; // незначащий 0 - минус 3 точек ширины и одну точку пробела
        if (dec_t == 1) tw -= 2; // "1" - ширина 2, т.к нет пробела и нет пробела между знаком
        if (edc_t == 1) tw -= 2; // "1" - ширина 2, т.к нет пробела
        if (dec_t == 1 && edc_t == 1) tw += 1;
      } else {
        tw = 24;      // 2 цифры по 5 точек + "°" - 3 точки + "C" - 5 точек  + "-/+" - 3 точки + 3 пробела между символами (между +- и первой цифрой пробела нет)
        if (dec_t == 0) tw -= 5; // незначащий 0 - минус 5 точек ширины
        if (dec_t == 1) tw -= 2; // "1" занимает 3 точки, а не 5 как другие
        if (edc_t == 1) tw -= 2; // "1" занимает 3 точки, а не 5 как другие
        if (dec_t == 1 && edc_t == 1) tw+=2; // две рядом стоящие "1" - пробел между ними две точки, а не одна     
        if (dec_t == 1 && edc_t != 1) tw++;
        if (dec_t == 0 && edc_t != 1) tw--;
      }
    }  
    if (c_size != 1) {
      if (pWIDTH <= 24) tw--; // Если ширина матрицы <= 24 - нет пробела между градусом и "C"
      if (pWIDTH <= 23) tw--; // Если ширина матрицы <= 23 - нет пробела между единицами градуса и градусом
    }
  }
#endif

  // Ширина отображения часов
  if (CLOCK_ORIENT == 0) {
    // Горизонтальные часы
    if (c_size == 1) {
      cw = 15;                  // 00:00 - 4 цифры по 3 точек + 3 - пробелы между цифрами
      if (h10 == 0) cw -= 4;    // незначащий * в десятках часов + 1 точка пробела
      if (h10 == 1) cw -= 2;    // "1" занимает 2 точки, а не 3 как другие 
      if (h01 == 1) cw -= 1;    // "1" занимает 2 точки, а не 3 как другие 
      if (m10 == 1) cw -= 1;    // "1" занимает 2 точки, а не 3 как другие 
      if (m01 == 1) cw -= 2;    // "1" занимает 2 точки, а не 3 как другие 
      if (h10 == 1 && h01 == 1) cw++;
      if (m10 == 1 && m01 == 1) cw++;
    } else {
      cw = 26;                  // 00:00 - 4 цифры по 5 точек + 2 - пробелы между цифрами в минутах/часах + 2 точки ширина точек + 2 пробела между точками и цифрами справа/слева
      if (h10 == 0) cw -= 6;    // незначащий * в десятках часов + 1 точка пробела
      if (h10 == 1) cw -= 2;    // "1" занимает 3 точки, а не 5 как другие 
      if (h01 == 1) cw -= 2;    // "1" занимает 3 точки, а не 5 как другие 
      if (m10 == 1) cw -= 2;    // "1" занимает 3 точки, а не 5 как другие 
      if (m01 == 1) cw -= 2;    // "1" занимает 3 точки, а не 5 как другие 
      if (h10 == 1 && h01 == 1) cw++;    // две рядом стоящие "1" - пробел между ними две точки, а не одна 
      if (m10 == 1 && m01 == 1) cw++;    // две рядом стоящие "1" - пробел между ними две точки, а не одна     
      if (pWIDTH <= 25) cw--;    // Если ширина матрицы <= 25 - ширина "точек" одна колонка вместо двух
      if (pWIDTH <= 24) cw -= 2; // Если ширина матрицы <= 24 - нет пробела между точками и цифрами слева и справа
    }
  } else {
    // Вертикальные часы
    if (c_size == 1) {
     cw = 7;
    } else {
     cw = 11;
    }    
  }

  return (uint16_t)((cw << 8) | tw);
}

void drawTemperature(int8_t X) {
#if (USE_WEATHER == 1)      

  // позиция вывода температуры по X вычисляется при отрисовке часов и сохраняется в глобальную переменную CLOCK_WX
  // При этом рассчитана на символы шириной 3
  // Если десятки градусов = 1 - сместить на колонку вправо
  // Если единицы градусов = 1 - сместить на колонку вправо
  // Если десятки градусов = 0 - сместить на 4 колонки вправо и 0 не рисовать
  // Если температура 0 - рисовать 0C
  volatile int8_t temp_x = CLOCK_LX; 
  int8_t temp_y = CLOCK_WY;

  if (clockScrollSpeed >= 240) X = 0; 

  uint8_t t = abs(temperature);
  uint8_t dec_t = t / 10;
  uint8_t edc_t = t % 10;

  if (!allow_two_row) {
    temp_y = CLOCK_Y;
    uint8_t width = 0;
    if (temperature == 0) {
      width = (c_size == 1 ? 6 : 15);
    } else {
      width = (c_size == 1 ? 11 : 25);
      if (dec_t == 0) width -= (c_size == 1 ? 4 : 6);
      if (dec_t == 1) width -= (c_size == 1 ? 1 : 2);
      if (edc_t == 1) width -= (c_size == 1 ? 1 : 2);
    }
    temp_x = ((pWIDTH - width) / 2) + width;
    while (temp_x >= pWIDTH) temp_x--;
  }
    
  // Получить цвет отображения значения температуры
  CRGB color;          
  if (isNightClock) 
    color = useTemperatureColorNight ? (temperature < -3 ? nightClockBrightness : (temperature > 3 ? nightClockBrightness << 16 : nightClockBrightness << 16 | nightClockBrightness << 8 | nightClockBrightness)) : getNightClockColorByIndex(nightClockColor);
  else
    color = useTemperatureColor ? CRGB(HEXtoInt(getTemperatureColor(temperature))) : clockLED[0]; 

  if (c_size == 1) {
    // Отрисовка температуры в малых часах
    // Для правильного позиционирования - рисуем справа налево
    if (temperature == 0) {
      // При температуре = 0 - рисуем маленький значок C
      if (!(useTemperatureColor || isNightClock)) color = clockLED[1]; 
      temp_x -= 1;  
      for(uint8_t i = 0; i < 3; i++) {
        drawPixelXY(getClockX(X + temp_x), temp_y + i, color);      
      }
      drawPixelXY(getClockX(X + temp_x + 1), temp_y, color);      
      drawPixelXY(getClockX(X + temp_x + 1), temp_y + 2, color);      
      temp_x -= 2;
    }

    uint8_t last_digit = edc_t;
    // Единицы градусов
    if (!(useTemperatureColor || isNightClock)) color = clockLED[0]; 
    temp_x -= (edc_t == 1 ? 1 : 2);
    drawDigit3x5(edc_t, getClockX(X + temp_x), temp_y, color);

    // Десятки градусов
    if (dec_t != 0) {
      if (!(useTemperatureColor || isNightClock)) color = clockLED[4]; 
      temp_x -= (dec_t == 1 ? 3 : (edc_t == 1 ? 3 : 4));
      drawDigit3x5(dec_t, getClockX(X + temp_x), temp_y, color);
      last_digit = dec_t;
    }
            
    // Нарисовать '+' или '-' если температура не 0
    // Горизонтальная черта - общая для '-' и '+'
    if (temperature != 0) {
      if (!(useTemperatureColor || isNightClock)) color = clockLED[2]; 
      temp_x -= (last_digit == 1 ? 3 : 4);
      for(uint8_t i = 0; i < 3; i++) {
        drawPixelXY(getClockX(X + temp_x + i), temp_y + 2, color);      
      }
      
      // Для плюcа - вертикальная черта
      if (temperature > 0) {
        drawPixelXY(getClockX(X + temp_x + 1), temp_y + 1, color);
        drawPixelXY(getClockX(X + temp_x + 1), temp_y + 3, color);
      }
    }    
  } else {
    // Отрисовка температуры в больших часах
    temp_x = CLOCK_LX;

    temp_y = allow_two_row ? CLOCK_Y - 5 : CLOCK_Y;
    while (temp_y < 0) temp_y++; 

    // Буква 'C'
    temp_x -= 4;
    if (!(useTemperatureColor || isNightClock))color = clockLED[1]; 
    for(uint8_t i=0; i<5; i++) drawPixelXY(getClockX(X + temp_x), temp_y + 1 + i, color);
    for(uint8_t i=0; i<3; i++) drawPixelXY(getClockX(X + temp_x + 1 + i), temp_y, color);
    for(uint8_t i=0; i<3; i++) drawPixelXY(getClockX(X + temp_x + 1 + i), temp_y + 6, color);
    drawPixelXY(getClockX(X + temp_x + 4), temp_y + 5, color);
    drawPixelXY(getClockX(X + temp_x + 4), temp_y + 1, color);

    // Знак градусов; если ширина <= 24 - пробела между градусом и С нет
    if (!(useTemperatureColor || isNightClock))color = clockLED[0]; 
    temp_x -= (pWIDTH <= 24 ? 3 : 4);
    drawPixelXY(getClockX(X + temp_x),   temp_y + 5, color);
    drawPixelXY(getClockX(X + temp_x),   temp_y + 4, color);
    drawPixelXY(getClockX(X + temp_x + 1), temp_y + 6, color);
    drawPixelXY(getClockX(X + temp_x + 1), temp_y + 3, color);
    drawPixelXY(getClockX(X + temp_x + 2), temp_y + 5, color);
    drawPixelXY(getClockX(X + temp_x + 2), temp_y + 4, color);

    // Единицы температуры; если ширина <= 23 - пробела между цифрой и градусом нет
    temp_x -= (edc_t == 1 ? 5 : 6);
    if (pWIDTH <= 23) temp_x++;
    if (!(useTemperatureColor || isNightClock))color = clockLED[4]; 
    drawDigit5x7(edc_t, getClockX(X + temp_x), temp_y, color);

    // Десятки температуры
    if (dec_t != 0) {
    if (!(useTemperatureColor || isNightClock))color = clockLED[3]; 
      temp_x -= (dec_t == 1 ? 5 : 6);
      temp_x -= (dec_t > 1 && edc_t == 1 ? -1 : 0);
      drawDigit5x7(dec_t, getClockX(X + temp_x), temp_y, color);
    }

    // Если температура не нулевая - рисуем знак '+' или '-'
    if (t != 0) {
      if (!(useTemperatureColor || isNightClock))color = clockLED[2]; 
      temp_x -= 3;
      for(uint8_t i=0; i<3; i++) drawPixelXY(getClockX(X + temp_x + i), temp_y + 3, color);
      if (temperature > 0) {
        for(uint8_t i=0; i<3; i++) drawPixelXY(getClockX(X + temp_x + 1), temp_y + 2 + i, color);
      }
    }
  }
#endif
}

// нарисовать дату календаря
void drawCalendar(uint8_t aday, uint8_t amnth, int16_t ayear, bool dots, int8_t X, int8_t Y) {

  uint8_t d10 = aday / 10;
  uint8_t d01 = aday % 10;
  uint8_t m10 = amnth / 10;
  uint8_t m01 = amnth % 10;

  uint8_t d1 = ayear / 1000;
  uint8_t d2 = (ayear / 100) % 10;
  uint8_t d3 = (ayear / 10) % 10;
  uint8_t d4 = ayear % 10;

  uint8_t cx = 0, dy = 0;
  
  if (!allow_two_row) {
    Y = CLOCK_Y;
  }

  if (c_size == 1) {

    CALENDAR_W = 15;   // 4 цифры шириной 3 + 3 пробела между цифрами

    if (pWIDTH >= 15) {
      dy = allow_two_row ? 6 : 0;
      
      // Отрисовка календаря в малых часах  
      // Число месяца
      drawDigit3x5(d10, X, Y + dy, clockLED[3]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
      drawDigit3x5(d01, X + 4, Y + dy, clockLED[3]);
    
      // разделитель числа/месяца
      if (dots) {
        drawPixelXY(getClockX(X + 7), Y + dy - (allow_two_row ? 1 : 0), clockLED[2]);
      }
      
      // Месяц
      drawDigit3x5(m10, X + 8, Y + dy, clockLED[4]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
      drawDigit3x5(m01, X + 12, Y + dy, clockLED[4]);
    
      // Год  
      if (allow_two_row) {
        drawDigit3x5(d1, X, Y, clockLED[0]);
        drawDigit3x5(d2, X + 4, Y, clockLED[0]);
        drawDigit3x5(d3, X + 8, Y, clockLED[1]);
        drawDigit3x5(d4, X + 12, Y, clockLED[1]);
      }
    } else {

      uint8_t x = 0;
      if (clockScrollSpeed >= 240) {
        X = 0;
        x = 0.51 + (pWIDTH - 7) / 2.0;
      }
      // По ширине влазит только вертикальное расположение двух цифр - дата и ниже - две цифры месяца
      drawDigit3x5(d10, X + x, Y + 6, clockLED[3]);
      drawDigit3x5(d01, X + x + 4, Y + 6, clockLED[3]);
      if (dots) { // Мигающие точки легко ассоциируются с часами
        drawPixelXY(getClockX(X + x + 3), Y + 5, clockLED[2]);
      }
      drawDigit3x5(m10, X + x, Y, clockLED[4]);
      drawDigit3x5(m01, X + x + 4, Y, clockLED[4]);
    }

  } else {

    // Отрисовка календаря в больших часах

    CALENDAR_W = 26;                   // 4 цифры шириной 5 + 2 пробела между цифрами + 2 - ширина точки + 2 - слева/справа от точки
    if (pWIDTH <= 25) CALENDAR_W--;    // На матрицах 25 и меньше - ширина разделит. точки - один столбец
    if (pWIDTH <= 24) CALENDAR_W -= 2; // На матрицах 24 и меньше - нет пробелов слева и справа от разделит. точки

    // Это рассчетн
    
    if (CLOCK_ORIENT == 0 || !allow_two_row) {

      // Если скроллинга часов нет - брать позицию календаря - центр матрицы 
      if (clockScrollSpeed >= 240) X = 0;

      // Цифра 1 - три точки в ширину, другие - 5 точек в ширину                     + 2  пробела между цифрами дня и месяца
      uint8_t width_d = (d10 == 1 ? 3 : 5) + (d01 == 1 ? 3 : 5) + (m10 == 1 ? 3 : 5) + (m01 == 1 ? 3 : 5) + 2;
      
      if (pWIDTH > 25)  width_d += 4;  // 2 точки - разделитель числа / месяца + пробел ДО точки + пробел ПОСЛЕ точки
      else
      if (pWIDTH == 25) width_d += 3;  // 1 точка - разделитель числа / месяца + пробел ДО точки + пробел ПОСЛЕ точки
      else
                        width_d += 1;  // 1 точка - разделитель числа / месяца, нет разделителей-пробелоа ДО точки и ПОСЛЕ точки

      // Цифра 1 - три точки в ширину, другие - 5 точек в ширину + 3 точки разделителя между цифрами
      uint8_t width_y = (d1 == 1 ? 3 : 5) + (d2 == 1 ? 3 : 5) + (d3 == 1 ? 3 : 5) + (d4 == 1 ? 3 : 5) + 3;

      // Если год не отображается - ширина равне ширине числа+месяца
      // Если год отображается - ширина равне максимальной ширине числа/месяца или года
      uint8_t realWidth = allow_two_row ? max(width_d, width_y ) : width_d;

      uint8_t cx = realWidth == pWIDTH ? 0 : (0.51 + ((pWIDTH - realWidth) / 2.0));
      if (pWIDTH - realWidth %2 == 1) cx++;

      bool double_dot = pWIDTH > 25;
      bool dot_spaces = pWIDTH >= 25 || width_d <= 21;

      // Правая граница календаря
      int8_t right_x = X + cx + realWidth;
      if (pWIDTH == 23 && realWidth == 21) right_x++;
      
      int8_t y = Y;

      int8_t x = right_x;

      dy = allow_two_row ? 9 : 0;
      while (y + dy + 7 > pHEIGHT) dy--;
      
      x -= (m01 == 1 ? 4 : 5);
      drawDigit5x7(m01, x , y + dy, clockLED[1]);  x += m01 == 1 ? 1 : 0;
      x -= (m10 == 1 ? 5 : 6);
      drawDigit5x7(m10, x, y + dy, clockLED[0]);   x += m10 == 1 ? 1 : 0;

      // 2 точки - разделитель числа / месяца + пробел ДО точки + пробел ПОСЛЕ точки
      if (double_dot) x -= 3;
      else
      // 1 точка - разделитель числа / месяца + пробел ДО точки + пробел ПОСЛЕ точки
      if (pWIDTH == 25) x -= 1;
      else
      // нет пробелов, ширина разделителя - 1 точка
      x--;
      if (!double_dot && dot_spaces) x--;
      
      drawPixelXY(getClockX(x), y + dy    , clockLED[2]);         
      drawPixelXY(getClockX(x), y + dy + 1, clockLED[2]);
      if (double_dot) {
        drawPixelXY(getClockX(x + 1), y + dy    , clockLED[2]);
        drawPixelXY(getClockX(x + 1), y + dy + 1, clockLED[2]);        
      }

      // Если есть разделитель -пробел между датой и точкой - 
      if (dot_spaces) x--;
            
      x -= (d01 == 1 ? 4 : 5);
      drawDigit5x7(d01, x, y + dy, clockLED[4]); x += d01 == 1 ? 1 : 0;
      x -= (d10 == 1 ? 5 : 6);
      drawDigit5x7(d10, x, y + dy, clockLED[3]);
            
      // Если позволено рисовать в два ряда - нарисовать год календаря
      // Отрисовку проводить справа налево, чтобы выравнивать по правому краю даты (день/месяц)
      if (allow_two_row) {
        x = right_x;
        x -= (d4 == 1 ? 4 : 5);
        drawDigit5x7(d4, x, y, clockLED[0]); x += d4 == 1 ? 1 : 0;
        x -= (d3 == 1 ? 5 : 6);
        drawDigit5x7(d3, x, y, clockLED[1]); x += d3 == 1 ? 1 : 0;
        x -= (d2 == 1 ? 5 : 6);
        drawDigit5x7(d2, x, y, clockLED[3]); x += d2 == 1 ? 1 : 0;
        x -= (d1 == 1 ? 5 : 6);
        drawDigit5x7(d1, x, y, clockLED[4]);
      }

    } else {

      // Вертикальное расположение календаря - также как часы ДД в верхней строке, ММ - в нижней строке
      drawDigit5x7(d10, X, Y + 8, clockLED[0]);
      drawDigit5x7(d01, X + 6, Y + 8, clockLED[1]);
      if (dots) { // Мигающие точки легко ассоциируются с часами
        for (uint8_t i=0; i<3; i++) drawPixelXY(getClockX(X + 4 + i), Y + 7, clockLED[2]);
      }
      drawDigit5x7(m10, X, Y, clockLED[3]);
      drawDigit5x7(m01, X + 6, Y, clockLED[4]);
    }
  }
}

void clockRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_CLOCK;
  }

  // Очистка экрана. Сама отрисовка часов производится как
  // наложение оверлея на черный экран в doEffectWithOverlay()
  FastLED.clear();  
}

void clockTicker() {  

  hrs = hour();
  mins = minute();
  aday = day();
  amnth = month();
  ayear = year();

  if (isTurnedOff && needTurnOffClock && init_time) {
    #if (USE_TM1637 == 1)
    display.displayByte(_empty, _empty, _empty, _empty);
    display.point(false);
    #endif
    return;
  }

  bool halfSec = halfsecTimer.isReady();
  if (halfSec) {    
    clockHue += HUE_STEP;
    setOverlayColors();
    dotFlag = !dotFlag;
  }

#if (USE_TM1637 == 1)

  if (isButtonHold || bCounter > 0) {
    // Удержание кнопки - изменение яркости + 2 сек после того как кнопка отпущена - 
    // отображать показание текущего значения яркости в процентах 0..99
    if (isButtonHold) bCounter = 4;
    if (!isButtonHold && bCounter > 0 && halfSec) bCounter--;
    uint8_t prcBrightness = map8(globalBrightness,0,99);
    uint8_t m10 = getByteForDigit(prcBrightness / 10);
    uint8_t m01 = getByteForDigit(prcBrightness % 10);
    display.displayByte(_b_, _r_, m10, m01);
    display.point(false);
  } else if (wifi_print_ip) {
    // Четырехкратное нажатие кнопки запускает отображение по частям текущего IP лампы  
    if (dotFlag && halfSec) {
      if (wifi_print_idx<=3) {
        String  ip = WiFi.localIP().toString();
        int16_t value = atoi(GetToken(ip, wifi_print_idx + 1, '.').c_str()); 
        display.displayInt(value);
        display.point(false);
        wifi_print_idx++;
      } else {
        wifi_print_idx = 0; 
        wifi_print_ip = false;
      }
    }
  } else {
    // Если время еще не получено - отображать прочерки
    if (!init_time) {
      if (halfSec) display.displayByte(_dash, _dash, _dash, _dash);
    } else if (!isAlarmStopped && (isPlayAlarmSound || isAlarming)) {
      // Сработал будильник (звук) - плавное мерцание текущего времени      
      if (halfSec) display.displayClock(hour(),minute());
      if (millis() - fade_time > 65) {
        fade_time = millis();
        display.setBrightness(aCounter);        
        if (aDirection) aCounter++; else aCounter--;
        if (aCounter > 7) {
          aDirection = false;
          aCounter = 7;
        }
        if (aCounter == 0) {
          aDirection = true;
        }
      }
    } else {
      // Время получено - отображать часы:минуты  
      #if (USE_WEATHER == 1)
      // RailWar Evgeny (C)
      if (((useWeather > 0)) && weather_ok && (((second() + 10) % 30) >= 28)) {
        uint8_t t = abs(temperature);
        uint8_t atH = t / 10;
        uint8_t atL = t % 10;
        display.point(false);
        if (atH == 0)
          display.displayByte(_empty, (temperature >= 0) ? _empty : _dash, display.encodeDigit(atL), _degree);
        else
          display.displayByte((temperature >= 0) ? _empty : _dash, display.encodeDigit(atH), display.encodeDigit(atL), _degree);
      } else 
      #endif
      {
        display.displayClock(hour(),minute());         
        // Отображение часов - разделительное двоеточие...
        if (halfSec) display.point(dotFlag);
      }
      display.setBrightness(isTurnedOff ? 1 : 7);
    }
  }
#endif  
}

void overlayWrap() {
  // В оверлей отправляется полоса от y_low до y_high во всю ширину матрицы  
  int16_t thisLED = 0;  
  for (uint8_t i = 0; i < pWIDTH; i++) {
    for (uint8_t j = y_overlay_low; j <= y_overlay_high; j++) {
      int16_t pn = getPixelNumber(i,j);
      if (pn >= 0 && pn < NUM_LEDS) {
        overlayLEDs[thisLED] = leds[pn];
      }
      thisLED++;
    }
  }
}

void overlayUnwrap() {
  int16_t thisLED = 0;  
  for (uint8_t i = 0; i < pWIDTH; i++) {
    for (uint8_t j = y_overlay_low; j <= y_overlay_high; j++) {
      int16_t pn = getPixelNumber(i,j);
      if (pn >= 0 && pn < NUM_LEDS) {
        leds[pn] = overlayLEDs[thisLED];
      }  
      thisLED++; 
    }
  }
}

void checkCalendarState() {
  if (millis() - showDateStateLastChange > (showDateState ? showDateDuration : showDateInterval) * 1000L) {
    showDateStateLastChange = millis();
    showDateState = !showDateState;
    // В однострочных часах чередовать отображение даты и температуры
    if (!allow_two_row && init_weather && showDateState) {
      if (showDateInClock) {
        showWeatherState = !showWeatherState;
      } else {
        showWeatherState = showDateState;
      }
    }
  }  
}

void contrastClock() {  
  for (uint8_t i = 0; i < 5; i++) clockLED[i] = NORMAL_CLOCK_COLOR;
}

void contrastClockA() {  
  for (uint8_t i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_1;
}

void contrastClockB() {
  for (uint8_t i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_2;
}

void contrastClockC(){
  CRGB color = CONTRAST_COLOR_3;
  CRGB gc = CRGB(globalColor);
  if (color == gc) color = -color;
  for (uint8_t i = 0; i < 5; i++) clockLED[i] = color;  
}

void setOverlayColors() {
  if (COLOR_MODE == 0 && !(thisMode == MC_CLOCK || thisMode == MC_TEXT || thisMode == MC_FILL_COLOR || thisMode == MC_COLORS)) {
    switch (thisMode) {
      case MC_SNOW:
      case MC_NOISE_ZEBRA: 
      case MC_NOISE_MADNESS:
      case MC_NOISE_CLOUD:
      case MC_NOISE_FOREST:
      case MC_NOISE_OCEAN: 
      case MC_RAIN:      
        contrastClockA(); // оранжевые
        break;
      case MC_NOISE_LAVA:
        contrastClockB(); // зеленые
        break;
      case MC_DAWN_ALARM:
        contrastClockC(); // желтые или инверсные
        break;      
      default:
        contrastClock();  // белые
        break;
    }
  } else {
    clockColor();
  }
}

void calculateDawnTime() {

  uint8_t alrmHour;
  uint8_t alrmMinute;
  
  dawnWeekDay = 0;
  if (!init_time || alarmWeekDay == 0) return;       // Время инициализировано? Хотя бы на один день будильник включен?

  int8_t alrmWeekDay = weekday()-1;                  // day of the week, Sunday is day 0   
  if (alrmWeekDay == 0) alrmWeekDay = 7;             // Sunday is day 7, Monday is day 1;

  uint8_t h = hour();
  uint8_t m = minute();
  uint8_t w = weekday()-1;
  if (w == 0) w = 7;

  uint8_t cnt = 0;
  while (cnt < 2) {
    cnt++;
    while ((alarmWeekDay & (1 << (alrmWeekDay - 1))) == 0) {
      alrmWeekDay++;
      if (alrmWeekDay == 8) alrmWeekDay = 1;
    }
      
    alrmHour = alarmHour[alrmWeekDay-1];
    alrmMinute = alarmMinute[alrmWeekDay-1];
  
    // "Сегодня" время будильника уже прошло? 
    if (alrmWeekDay == w && (h * 60L + m > alrmHour * 60L + alrmMinute)) {
      alrmWeekDay++;
      if (alrmWeekDay == 8) alrmWeekDay = cnt == 1 ? 1 : 7;
    }
  }

  // расчёт времени рассвета
  if (alrmMinute > dawnDuration) {                  // если минут во времени будильника больше продолжительности рассвета
    dawnHour = alrmHour;                            // час рассвета равен часу будильника
    dawnMinute = alrmMinute - dawnDuration;         // минуты рассвета = минуты будильника - продолж. рассвета
    dawnWeekDay = alrmWeekDay;
  } else {                                          // если минут во времени будильника меньше продолжительности рассвета
    dawnWeekDay = alrmWeekDay;
    dawnHour = alrmHour - 1;                        // значит рассвет будет часом раньше
    if (dawnHour < 0) {
      dawnHour = 23;     
      dawnWeekDay--;
      if (dawnWeekDay == 0) dawnWeekDay = 7;                           
    }
    dawnMinute = 60 - (dawnDuration - alrmMinute);  // находим минуту рассвета в новом часе
    if (dawnMinute == 60) {
      dawnMinute=0; dawnHour++;
      if (dawnHour == 24) {
        dawnHour=0; dawnWeekDay++;
        if (dawnWeekDay == 8) dawnWeekDay = 1;
      }
    }
  }

  DEBUGLN(String(F("Следующий рассвет в ")) + padNum(dawnHour,2)+ F(":") + padNum(dawnMinute,2) + ", " + getWeekdayString(dawnWeekDay));
}

// Проверка времени срабатывания будильника
void checkAlarmTime() {

  uint8_t h = hour();
  uint8_t m = minute();
  uint8_t w = weekday()-1;
  if (w == 0) w = 7;

  // Будильник включен?
  if (init_time && dawnWeekDay > 0) {

    // Время срабатывания будильника после завершения рассвета
    uint8_t alrmWeekDay = dawnWeekDay;
    uint8_t alrmHour = dawnHour;
    uint8_t alrmMinute = dawnMinute + dawnDuration;
    if (alrmMinute >= 60) {
      alrmMinute = 60 - alrmMinute;
      alrmHour++;
    }
    if (alrmHour > 23) {
      alrmHour = 24 - alrmHour;
      alrmWeekDay++;
    }
    if (alrmWeekDay > 7) alrmWeekDay = 1;

    // Текущий день недели совпадает с вычисленным днём недели рассвета?
    if (w == dawnWeekDay) {
       // Часы / минуты начала рассвета наступили? Еще не запущен рассвет? Еще не остановлен пользователем?
       if (!isAlarming && !isAlarmStopped && ((w * 1000L + h * 60L + m) >= (dawnWeekDay * 1000L + dawnHour * 60L + dawnMinute)) && ((w * 1000L + h * 60L + m) < (alrmWeekDay * 1000L + alrmHour * 60L + alrmMinute))) {
         // Сохранить параметры текущего режима для восстановления после завершения работы будильника
         saveSpecialMode = specialMode;
         saveSpecialModeId = specialModeId;
         saveMode = thisMode;
         // Включить будильник
         specialMode = false;
         specialModeId = -1;
         set_isAlarming(true);
         set_isAlarmStopped(false);
         loadingFlag = true;         
         isButtonHold = false;
         set_thisMode(MC_DAWN_ALARM);
         setTimersForMode(thisMode);
         // Реальная продолжительность рассвета
         realDawnDuration = (alrmHour * 60L + alrmMinute) - (dawnHour * 60L + dawnMinute);
         if (realDawnDuration > dawnDuration) realDawnDuration = dawnDuration;
         // Отключить таймер автоперехода в демо-режим
         DEBUGLN(F("Переход в авторежим отключен"));
         idleTimer.stopTimer();
         #if (USE_MP3 == 1)
         if (useAlarmSound) PlayDawnSound();
         #endif
         sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
         DEBUGLN(String(F("Рассвет ВКЛ в ")) + padNum(h,2) + ":" + padNum(m,2));

         #if (USE_MQTT == 1)
         DynamicJsonDocument doc(256);
         String out;
         doc["act"]   = F("ALARM");
         doc["state"] = F("on");
         doc["type"]  = F("dawn");
         serializeJson(doc, out);      
         SendMQTT(out, TOPIC_ALM);
         #endif
       }
    }
    
    delay(0); // Для предотвращения ESP8266 Watchdog Timer
    
    // При наступлении времени срабатывания будильника, если он еще не выключен пользователем - запустить режим часов и звук будильника
    if (alrmWeekDay == w && alrmHour == h && alrmMinute == m && isAlarming) {
      DEBUGLN(String(F("Рассвет Авто-ВЫКЛ в ")) + padNum(h,2) + ":" + padNum(m,2));
      set_isAlarming(false);
      set_isAlarmStopped(false);
      set_isPlayAlarmSound(true);
      setSpecialMode(1);      
      alarmSoundTimer.setInterval(alarmDuration * 60000L);
      alarmSoundTimer.reset();
      // Играть звук будильника
      // Если звук будильника не используется - просто запустить таймер.
      // До окончания таймера индикатор TM1637 будет мигать, лампа гореть ярко белым.
      #if (USE_MP3 == 1)
      if (useAlarmSound) {
        PlayAlarmSound();
      }
      #endif
      sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника

      #if (USE_MQTT == 1)
      DynamicJsonDocument doc(256);
      String out;
      doc["act"]   = F("ALARM");
      doc["state"] = F("on");
      doc["type"]  = F("alarm");
      serializeJson(doc, out);      
      SendMQTT(out, TOPIC_ALM);
      #endif

    }

    delay(0); // Для предотвращения ESP8266 Watchdog Timer

    // Если рассвет начинался и остановлен пользователем и время начала рассвета уже прошло - сбросить флаги, подготовив их к следующему циклу
    if (isAlarmStopped && ((w * 1000L + h * 60L + m) > (alrmWeekDay * 1000L + alrmHour * 60L + alrmMinute + alarmDuration))) {
      set_isAlarming(false);
      set_isAlarmStopped(false);
      StopSound(0);
    }
  }
  
  // Подошло время отключения будильника - выключить, вернуться в предыдущий режим
  if (alarmSoundTimer.isReady()) {

    // Во время работы будильника индикатор плавно мерцает.
    // После завершения работы - восстановить яркость индикатора
    #if (USE_TM1637 == 1)
    display.setBrightness(7);
    #endif
    DEBUGLN(String(F("Будильник Авто-ВЫКЛ в ")) + padNum(h,2)+ ":" + padNum(m,2));
    
    alarmSoundTimer.stopTimer();
    set_isPlayAlarmSound(false);
    StopSound(1000);   

    calculateDawnTime();
    
    resetModes();  
    setManualModeTo(false);

    if (saveSpecialMode){
       setSpecialMode(saveSpecialModeId);
    } else {
       setEffect(saveMode);
    }

    sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника

    #if (USE_MQTT == 1)
    DynamicJsonDocument doc(256);
    String out;
    doc["act"]   = F("ALARM");
    doc["state"] = F("off");
    doc["type"]  = F("auto");
    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_ALM);
    #endif
  }

  delay(0); // Для предотвращения ESP8266 Watchdog Timer

  #if (USE_MP3 == 1)
  // Плавное изменение громкости будильника
  if (fadeSoundTimer.isReady()) {
    if (fadeSoundDirection > 0) {
      // увеличение громкости
      dfPlayer.increaseVolume(); delay(GUARD_DELAY);
      fadeSoundStepCounter--;
      if (fadeSoundStepCounter <= 0) {
        fadeSoundDirection = 0;
        fadeSoundTimer.stopTimer();
      }
    } else if (fadeSoundDirection < 0) {
      // Уменьшение громкости
      dfPlayer.decreaseVolume(); delay(GUARD_DELAY);
      fadeSoundStepCounter--;
      if (fadeSoundStepCounter <= 0) {
        set_isPlayAlarmSound(false);
        fadeSoundDirection = 0;
        fadeSoundTimer.stopTimer();
        StopSound(0);
      }
    }
  }
  #endif
    
  delay(0); // Для предотвращения ESP8266 Watchdog Timer    
}

void stopAlarm() {
  if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) {
    DEBUGLN(String(F("Рассвет ВЫКЛ в ")) + padNum(hour(),2) + ":" + padNum(minute(),2));
    set_isAlarming(false);
    set_isAlarmStopped(true);
    set_isPlayAlarmSound(false);
    cmd95 = "";

    alarmSoundTimer.stopTimer();

    // Во время работы будильника индикатор плавно мерцает.
    // После завершения работы - восстановить яркость индикатора

    #if (USE_TM1637 == 1)
    display.setBrightness(7);
    #endif

    StopSound(1000);

    resetModes();  
    setManualModeTo(false);
    calculateDawnTime();
    
    if (saveSpecialMode){
       setSpecialMode(saveSpecialModeId);
    } else {
       setEffect(saveMode);
    }
    delay(0);    
    sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника

    #if (USE_MQTT == 1)
    DynamicJsonDocument doc(256);
    String out;
    doc["act"]   = F("ALARM");
    doc["state"] = F("off");
    doc["type"]  = F("stop");
    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_ALM);
    #endif
  }
}

// Проверка необходимости включения режима 1 по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
void checkAutoMode1Time() {
  if (AM1_effect_id <= -3 || AM1_effect_id >= MAX_EFFECT || !init_time) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало время активации режима - активировать
  if (!AM1_running && AM1_hour == hrs && AM1_minute == mins) {
    AM1_running = true;
    SetAutoMode(1);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (AM1_running && (AM1_hour != hrs || AM1_minute != mins)) {
    AM1_running = false;
  }
}

// Проверка необходимости включения режима 2 по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
void checkAutoMode2Time() {

  // Действие отличается от "Нет действия" и время установлено?
  if (AM2_effect_id <= -3 || AM2_effect_id >= MAX_EFFECT || !init_time) return;

  // Если сработал будильник - рассвет - режим не переключать - остаемся в режиме обработки будильника
  if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) return;

  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало время активации режима - активировать
  if (!AM2_running && AM2_hour == hrs && AM2_minute == mins) {
    AM2_running = true;
    SetAutoMode(2);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (AM2_running && (AM2_hour != hrs || AM2_minute != mins)) {
    AM2_running = false;
  }
}

// Проверка необходимости включения режима 1 по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
void checkAutoMode3Time() {
  if (AM3_effect_id <= -3 || AM3_effect_id >= MAX_EFFECT || !init_time) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало время активации режима - активировать
  if (!AM3_running && AM3_hour == hrs && AM3_minute == mins) {
    AM3_running = true;
    SetAutoMode(3);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (AM3_running && (AM3_hour != hrs || AM3_minute != mins)) {
    AM3_running = false;
  }
}

// Проверка необходимости включения режима 1 по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
void checkAutoMode4Time() {
  if (AM4_effect_id <= -3 || AM4_effect_id >= MAX_EFFECT || !init_time) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало время активации режима - активировать
  if (!AM4_running && AM4_hour == hrs && AM4_minute == mins) {
    AM4_running = true;
    SetAutoMode(4);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (AM4_running && (AM4_hour != hrs || AM4_minute != mins)) {
    AM4_running = false;
  }
}

#if (USE_WEATHER == 1)  
// Проверка необходимости включения режима "Рассвет" по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
void checkAutoMode5Time() {
  if (dawn_effect_id <= -3 || dawn_effect_id >= MAX_EFFECT || !init_weather || useWeather == 0) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало время активации режима - активировать
  if (!dawn_running && dawn_hour == hrs && dawn_minute == mins) {
    dawn_running = true;
    SetAutoMode(5);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (dawn_running && (dawn_hour != hrs || dawn_minute != mins)) {
    dawn_running = false;
  }
}

// Проверка необходимости включения режима "Закат" по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
void checkAutoMode6Time() {
  if (dusk_effect_id <= -3 || dusk_effect_id >= MAX_EFFECT || !init_weather || useWeather == 0) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало время активации режима - активировать
  if (!dusk_running && dusk_hour == hrs && dusk_minute == mins) {
    dusk_running = true;
    SetAutoMode(6);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (dusk_running && (dusk_hour != hrs || dusk_minute != mins)) {
    dusk_running = false;
  }
}
#endif  

// Выполнение включения режима 1,2,3,4,5,6 (amode) по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы, 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
void SetAutoMode(uint8_t amode) {

  uint8_t   AM_hour, AM_minute;
  int8_t AM_effect_id;
  switch(amode) {
    case 1:  AM_hour = AM1_hour;  AM_minute = AM1_minute;  AM_effect_id = AM1_effect_id;  break;
    case 2:  AM_hour = AM2_hour;  AM_minute = AM2_minute;  AM_effect_id = AM2_effect_id;  break;
    case 3:  AM_hour = AM3_hour;  AM_minute = AM3_minute;  AM_effect_id = AM3_effect_id;  break;
    case 4:  AM_hour = AM4_hour;  AM_minute = AM4_minute;  AM_effect_id = AM4_effect_id;  break;
    case 5:  AM_hour = dawn_hour; AM_minute = dawn_minute; AM_effect_id = dawn_effect_id; break;
    case 6:  AM_hour = dusk_hour; AM_minute = dusk_minute; AM_effect_id = dusk_effect_id; break;
    default: return;
  }
  
  bool   no_action = false;
  
  String text = F("Авторежим ");
  String mode_name = "";
  if (amode == 5)
    mode_name = F("'Рассвет'");
  else if (amode == 6)    
    mode_name = F("'Закат'");
  else  
    mode_name = String(amode);
    
  text += mode_name + F(" [") + padNum(AM_hour,2) + ":" + padNum(AM_minute,2) + F("] - ");

  int8_t ef = AM_effect_id;

  //ef: -3 - нет действия; 
  //    -2 - выключить панель (черный экран); 
  //    -1 - ночные часы; 
  //     0 - случайный,
  //     1 и далее - эффект из EFFECT_LIST по списку

  // включить указанный режим
  if (ef <= -3 || ef >= MAX_EFFECT) {
    no_action = true;
    text += F("нет действия");
  } else if (ef == -2) {

    // Выключить матрицу (черный экран)
    text += F("выключение панели");
    turnOff();

  } else if (ef == -1) {

    // Ночные часы
    text += F("ночные часы");
    setSpecialMode(8);
    
  } else {
    text += F("включение режима ");    
    // Если режим включения == 0 - случайный режим и автосмена по кругу
    resetModes();  
    setManualModeTo(ef != 0);    

    String s_tmp = String(EFFECT_LIST);
    
    if (ef == 0) {
      // "Случайный" режим и далее автосмена
      text += F("демонстрации эффектов:");
      uint32_t cnt = CountTokens(s_tmp, ','); 
      ef = random8(0, cnt - 1); 
    } else {
      ef -= 1; // Приведение номера эффекта (номер с 1) к индексу в массиве EFFECT_LIST (индекс c 0)
    }

    s_tmp = GetToken(s_tmp, ef+1, ',');
    text += F(" эффект ");
    text += "'" + s_tmp + "'";

    // Включить указанный режим из списка доступных эффектов без дальнейшей смены
    // Значение ef может быть 0..N-1 - указанный режим из списка EFFECT_LIST (приведенное к индексу с 0)      
    setEffect(ef);
  }

  if (!no_action) {
    DEBUGLN(text);  

    #if (USE_MQTT == 1)
    DynamicJsonDocument doc(256);
    String out;
    doc["act"]       = F("AUTO");
    doc["mode"]      = amode;
    doc["mode_name"] = mode_name;
    doc["text"]      = text;
    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_AMD);
    #endif
  }
}

#if (USE_TM1637 == 1)
uint8_t getByteForDigit(uint8_t digit) {
  switch (digit) {
    case 0: return _0_;
    case 1: return _1_;
    case 2: return _2_;
    case 3: return _3_;
    case 4: return _4_;
    case 5: return _5_;
    case 6: return _6_;
    case 7: return _7_;
    case 8: return _8_;
    case 9: return _9_;
    default: return _empty;
  }
}
#endif

void checkClockOrigin() {

  // Получить текущий тип размера горизонтальных часов: 0 - авто; 1 - шрифт 3х5; 2 - шрифт 5х7;
  c_size = getClockSizeType();
  
  allowVertical   = pWIDTH >= 7  && pHEIGHT >= 11;
  allowHorizontal = pWIDTH >= 15 && pHEIGHT >= 5;

  if (allowVertical || allowHorizontal) {
    // Если ширина матрицы не позволяет расположить часы горизонтально - переключить в вертикальный режим
    if (CLOCK_ORIENT == 1 && !allowVertical) {
      set_CLOCK_ORIENT(0);
    }
    // Если высота матрицы не позволяет расположить часы вертикально - переключить в горизонтальный режим
    if (CLOCK_ORIENT == 0 && !allowHorizontal) {
      set_CLOCK_ORIENT(1);
    }
  } else {
    set_clockOverlayEnabled(false);
    return;
  }

  while (true) {
    if (CLOCK_ORIENT == 0) {
      // Горизонтальные часы, календарь
      if (c_size == 1) {
        // Малые часы 
        CLOCK_W = (4*3 + 3*1);
        CLOCK_H = (1*5);
        CLOCK_X = (uint8_t((pWIDTH - CLOCK_W) / 2.0 + 0.51));         // 4 цифры * (шрифт 3 пикс шириной) 3 + пробела между цифрами), /2 - в центр  
        CLOCK_Y = (uint8_t((pHEIGHT - CLOCK_H) / 2.0 + 0.51));        // Одна строка цифр 5 пикс высотой  / 2 - в центр
        CALENDAR_W = (4*3 + 1);
        CALENDAR_H = (2*5 + 1);
        CALENDAR_X = (uint8_t((pWIDTH - CALENDAR_W) / 2.0));          // 4 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами) /2 - в центр
        CALENDAR_Y = (uint8_t((pHEIGHT - CALENDAR_H) / 2.0) + 0.51);  // Две строки цифр 5 пикс высотой + 1 пробел между строками / 2 - в центр      
      } else {
        // Большие часы       
        // Если ширина матрицы - 25 колонок - ширина точки, разделяющих часы /минуты и день/месяц - не 2 колонки, а одна
        if (pWIDTH == 25) {
          CLOCK_W = 25;
          CALENDAR_W = 25;
        } else
        // Если ширина матрицы - 23 ли 24 колонки - ширина точки, разделяющих часы /минуты и день/месяц - не 2 колонки, а одна, пробелов между 
        // последней цифрой часов и точками, а также между точками и первой цифрой минут нет
        if (pWIDTH == 23 || pWIDTH == 24) {
          CLOCK_W = 23;
          CALENDAR_W = 23;
        } else {
          // На матрицах шириной 26 и выше - ширина часов - 26
          CLOCK_W = 26;    // (4*5 + 2*1 + 4);
          CALENDAR_W = 26; // (4*5 + 2*1 + 4);
        }
        CLOCK_H = (1*7);
        CALENDAR_H = (2*7 + 2);                                   // В больших часах календарь - две строки высотой 7 + 2 пробела между строками
        CLOCK_X = (uint8_t((pWIDTH - CLOCK_W) / 2.0 + 0.51));         // 4 цифры * (шрифт 5 пикс шириной) 3 + пробела между цифрами) + 4 - ширина точек-разделителей, /2 - в центр 
        CLOCK_Y = (uint8_t((pHEIGHT - CLOCK_H) / 2.0 + 0.51));        // Одна строка цифр 7 пикс высотой  / 2 - в центр
        CALENDAR_X = (uint8_t((pWIDTH - CALENDAR_W) / 2.0));          // 4 цифры * (шрифт 5 пикс шириной) 3 + пробела между цифрами) + 4 - ширина точек-разделителей, /2 - в центр 
        CALENDAR_Y = (uint8_t((pHEIGHT - CALENDAR_H) / 2.0) + 0.51);  // Одна строка цифр 7 пикс высотой  / 2 - в центр
      }     
    } else {
      // Вертикальные часы, календарь
      if (c_size == 1) {
        // Малые часы 
        CLOCK_W = (2*3 + 1);
        CLOCK_H = (2*5 + 1);
        CLOCK_X = (uint8_t((pWIDTH - CLOCK_W) / 2.0 + 0.51));         // 2 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами) /2 - в центр
        CLOCK_Y = (uint8_t((pHEIGHT - CLOCK_H) / 2.0 + 0.51));        // Две строки цифр 5 пикс высотой + 1 пробел между строками / 2 - в центр
        CALENDAR_W = (4*3 + 1);
        CALENDAR_H = (2*5 + 1);        
        CALENDAR_X = (uint8_t((pWIDTH - CALENDAR_W) / 2.0));          // 4 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами) /2 - в центр
        CALENDAR_Y = CLOCK_Y;                                     // Вертикальные часы имеют тот же размер, что и календарь
      } else {
        // Большие часы     
        CLOCK_W = (2*5 + 1);
        CLOCK_H = (2*7 + 1);
        CLOCK_X = (uint8_t((pWIDTH - CLOCK_W) / 2.0 + 0.51));         // 2 цифры * (шрифт 5 пикс шириной) 1 + пробел между цифрами) /2 - в центр
        CLOCK_Y = (uint8_t((pHEIGHT - CLOCK_H) / 2.0 + 1));           // Две строки цифр 7 пикс высотой + 1 пробел между строками / 2 - в центр
        CALENDAR_W = (2*5 + 1);
        CALENDAR_H = (2*7 + 2);
        CALENDAR_X = CLOCK_X;                                     // Вертикальные часы имеют тот же размер, что и календарь
        CALENDAR_Y = CLOCK_Y;                                     // Вертикальные часы имеют тот же размер, что и календарь
      }     
    }
    
    while (CLOCK_X > 0 && CLOCK_X + CLOCK_W > pWIDTH)  CLOCK_X--;
    while (CLOCK_Y > 0 && CLOCK_Y + CLOCK_H > pHEIGHT) CLOCK_Y--;
      
    while (CALENDAR_X > 0 && CALENDAR_X + CALENDAR_W > pWIDTH)  CALENDAR_X--; 
    while (CALENDAR_Y > 0 && CALENDAR_Y + CALENDAR_H > pHEIGHT) CALENDAR_Y--;

    // Если большие часы всё равно не влазят в рабочее поле матрицы - переключиться на малые часы и пересчитать размеры и положение
    if (c_size != 1 && (CALENDAR_X + CALENDAR_W > pWIDTH)) {
      c_size = 1;
      continue;
    }

    break;
  }

  // Можно ли отображать горизонтальные часы вместе с температурой и календарь - дата с годом - это требует двух строк
  // Если в две строки отображать нельзя - календарь отображается без года, температура - не вместе с часами, а чередуется с календарем
  allow_two_row = (c_size == 1)
    ? pHEIGHT >= 11  // Малые часы - шрифт 3x5 - две строки + пробел между строками - высота 11 строк 
    : pHEIGHT >= 15; // Большие часы - шрифт 5x7 - две строки + пробел между строками - высота 15 строк

  CLOCK_XC = CLOCK_X;
  CALENDAR_XC = CALENDAR_X;

  if (!allow_two_row) {
    CALENDAR_Y = CLOCK_Y;
  }
}

uint32_t getNightClockColorByIndex(uint8_t idx) {
  // nightClockBrightness - яркость каждой компоненты цвета ночных часов - R,G,B
  // Нужный свет получаем комбинируя яркость каждого из компонентов RGB цвета
  uint32_t color = nightClockBrightness << 16;  // Red
  switch (idx) {
    case 0: color = nightClockBrightness << 16; break;  // Red
    case 1: color = nightClockBrightness << 8;  break;  // Green
    case 2: color = nightClockBrightness;       break;  // Blue
    case 3: color = nightClockBrightness << 8  | nightClockBrightness; break;  // Cyan
    case 4: color = nightClockBrightness << 16 | nightClockBrightness; break;  // Magenta
    case 5: color = nightClockBrightness << 16 | nightClockBrightness << 8; break;  // Yellow
    case 6: color = nightClockBrightness << 16 | nightClockBrightness << 8 | nightClockBrightness; break;  // White
  }  
  return color;
}

void printNtpServerName() {
  DEBUG(F("NTP-сервер "));
  DEBUG(ntpServerName);
  DEBUG(F(" -> "));
  DEBUGLN(timeServerIP);
}
