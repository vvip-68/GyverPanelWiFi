
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
byte clockHue;

CRGB clockLED[5] = {HOUR_COLOR, HOUR_COLOR, DOT_COLOR, MIN_COLOR, MIN_COLOR};

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  Serial.print(F("Отправка NTP пакета на сервер "));
  Serial.println(ntpServerName);
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
}

void parseNTP() {
  Serial.println(F("Разбор пакета NTP"));
  ntp_t = 0; ntp_cnt = 0; init_time = true; refresh_time = false;
  unsigned long highWord = word(incomeBuffer[40], incomeBuffer[41]);
  unsigned long lowWord = word(incomeBuffer[42], incomeBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  unsigned long seventyYears = 2208988800UL ;
  time_t t = secsSince1900 - seventyYears + (timeZoneOffset) * 3600;
  Serial.print(F("Секунд с 1970: "));
  Serial.println(t);
  setTime(t);
  calculateDawnTime();

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(256);
  String out;
  doc["act"] = F("TIME");
  doc["server_name"] = ntpServerName;
  doc["server_ip"] = timeServerIP.toString();
  doc["result"] = F("OK");
  doc["time"] = t;
  serializeJson(doc, out);      
  NotifyInfo(out);
  #endif
}

void getNTP() {
  if (!wifi_connected) return;
  WiFi.hostByName(ntpServerName, timeServerIP);
  IPAddress ip;
  ip.fromString(F("0.0.0.0"));
#if defined(ESP8266)
  if (!timeServerIP.isSet()) timeServerIP.fromString(F("192.36.143.130"));  // Один из ru.pool.ntp.org // 195.3.254.2
#endif
#if defined(ESP32)
  if (timeServerIP==ip) timeServerIP.fromString(F("192.36.143.130"));  // Один из ru.pool.ntp.org // 195.3.254.2
#endif
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
  NotifyInfo(out);
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
    // Цвет поиндексу настроек текущего ночного цвета     
    CRGB color = getNightClockColorByIndex(nightClockColor);
    for (byte i = 0; i < 5; i++) clockLED[i] = color;
  } else if (color_idx == -1) {     
    // Инверсный от основного цвет
    CRGB color = globalColor == 0xFFFFFF
      ? color = CRGB::Navy
      : -CRGB(globalColor);
    for (byte i = 0; i < 5; i++) clockLED[i] = color;  
  } else if (color_idx == 0) {     
    // Монохромные часы  
    CRGB color = getGlobalClockColor();
    for (byte i = 0; i < 5; i++) clockLED[i] = color;
  } else if (color_idx == 1) {
    // Каждая цифра своим цветом, плавная смена цвета
    for (byte i = 0; i < 5; i++) clockLED[i] = CHSV(clockHue + HUE_GAP * i, 255, 255);
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
    for (byte i = 0; i < 5; i++) clockLED[i] = color;
  }
}

byte getClockSizeType() {
  byte clock_size = CLOCK_SIZE;
  // Если часы авто или большие - определить - а поместятся ли они на матрицу по ширине
  // Большие часы для шрифта 5x7 требуют 4*5 /цифры/ + 4 /двоеточие/ + 2 /пробел между цифрами часов и минут / = 26 колонки
  if ((clock_size == 0 || clock_size == 2) && WIDTH < 26) clock_size = 1;
  if (clock_size == 0) clock_size = 2;
  return clock_size;
}

// Вычисление позиции отрисовки пикселя для часов, сдвигающихся по кругу.
byte getClockX(int8_t x) {
  return x<0 ? WIDTH + x : x;
}

// нарисовать часы
void drawClock(byte hrs, byte mins, boolean dots, int8_t X, int8_t Y) {

  int8_t x = X;

  byte h10 = hrs / 10;
  byte h01 = hrs % 10;
  byte m10 = mins / 10;
  byte m01 = mins % 10;
  
  if (CLOCK_ORIENT == 0) {

    if (c_size == 1) {

      // Часы занимают 4 знакоместав 3x5 + 3 разделительных колонуи, итого 15 точек по горизонтали
      // Однако реально они могут занимать меньше места, если в часах/минутах есть 11. Эксттремально е значение 1:11 занимает 10 точек и также должнго быть отцентрировано в этих 15 точках
      // Центр композиции - разделительное двоеточие
      // Если десятки часов = 1 - смещаем начала отрисовки на 1 вправо 
      // Если единицы часов = 1 - смещаем начала отрисовки на 1 вправо
      // если десятки часов - 0 - не рисуется, смещаем начало отрисовки на 3 вправо

      // Тут дурь, но не смог правильно бодобрать корректировку для всех вариантов ЧЧ:MM чтобы и центрировались правильно и расстояние между цифрами часов-минут было правильное      
      byte w = 15;      
      if (h10 == 1) w--;
      if (m01 == 1) w--;
      if (h10 == 0) w -= (h01 == 1 ? 3 : 4);
      x += ((15 - w) / 2) + (w == 15 ? 0 : 1);
      
      if (h10 < 2 && m01 != 1) x--;
      if (h10 == 1 && m01 == 1) x--;
      if (h10 == 2) x--;
      if (hrs == 0 && mins == 0) x--;
            
      // 0 в часах не выводим, для центрирования сдвигаем остальные цифры влево на место нуля
      if (h10 > 0) {
        x += (h10 == 2 ? 1 : 0);
        drawDigit3x5(h10, x, Y, clockLED[0]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
        x += (h10 == 1 ? 3 : 4);
        x += (h10 == 2 && h01 == 1 ? -1 : 0);
      }  

      drawDigit3x5(h01, x, Y, clockLED[1]);
      x += 3;
      
      if (dots) {
        drawPixelXY(getClockX(x), Y + 1, clockLED[2]);
        drawPixelXY(getClockX(x), Y + 3, clockLED[2]);
      }
      x++;

      drawDigit3x5(m10, x, Y, clockLED[3]);      
      x += (m10 == 1 ? 3 : 4);

      x += (m10 != 1 && m01 == 1 ? -1 : 0);
      drawDigit3x5(m01, x , Y, clockLED[4]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать влево на 1 колонку
      x += (m01 == 1 ? 2 : 3);

      CLOCK_WX = x - 11;   // температуру выравнивать по правому краю с часами. 11 - 3символа температуры 3x5 + 2 пробела

    } else {

      // отрисовка часов 5x7
      byte cx = 0;
      if (h10 == 1 && m01 == 1 && X > 0) X += 2;
      // 0 в часах не выводим, для центрирования сдвигаем остальные цифры влево на место нуля
      if (h10 > 0) {
        drawDigit5x7(h10, X + cx, Y, clockLED[0]); // шрифт 5x7 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
      } else {
        X -= 3;
      }  
      
      cx += (h10 == 1 ? -1 : 0);
      cx += (h10 > 1 && h01 == 1 ? -1 : 0);
      drawDigit5x7(h01, X + 6 + cx, Y, clockLED[1]);      

      cx += (h01 == 1 ? -1 : 0);
      if (dots) {
        drawPixelXY(getClockX(X + 12 + cx), Y + 1, clockLED[2]);         
        drawPixelXY(getClockX(X + 13 + cx), Y + 1, clockLED[2]);
        drawPixelXY(getClockX(X + 12 + cx), Y + 2, clockLED[2]);
        drawPixelXY(getClockX(X + 13 + cx), Y + 2, clockLED[2]);
        drawPixelXY(getClockX(X + 12 + cx), Y + 4, clockLED[2]);
        drawPixelXY(getClockX(X + 13 + cx), Y + 4, clockLED[2]);
        drawPixelXY(getClockX(X + 12 + cx), Y + 5, clockLED[2]);
        drawPixelXY(getClockX(X + 13 + cx), Y + 5, clockLED[2]);
      }

      cx += (m10 == 1 ? -1 : 0);
      drawDigit5x7(m10, X + 15 + cx, Y, clockLED[3]);

      cx += (m01 == 1 ? -1 : 0) + (m10 == 1 && m01 != 1 ? -1 : 0);
      drawDigit5x7(m01, X + 21 + cx, Y, clockLED[4]);  // шрифт 5x7 в котором 1 - по центру знакоместа - смещать влево на 1 колонку

    }
  } else { // Вертикальные часы
    
    //if (hrs > 9) // Так реально красивей
    drawDigit3x5(h10, X, Y + 6, clockLED[0]);
    drawDigit3x5(h01, X + 4, Y + 6, clockLED[1]);
    if (dots) { // Мигающие точки легко ассоциируются с часами
      drawPixelXY(getClockX(X + 3), Y + 5, clockLED[2]);
    }
    drawDigit3x5(m10, X, Y, clockLED[3]);
    drawDigit3x5(m01, X + 4, Y, clockLED[4]);

  }
}

void drawTemperature() {
#if (USE_WEATHER == 1)           

  if (weatherOverlayEnabled) {          

    // позиция выводатемпературы по X вычисляется при отрисовке часов и сохраняется в глобальную переменную CLOCK_WX
    // При этом рассчитана на символы шириной 3
    // Если десятки градусов = 1 - сместить на колонку вправо
    // Если единицы градусов = 1 - сместить на колонку вправо
    // Если десятки градусов = 0 - сместить на 4 колонки вправо и 0 не рисовать
    // Если температура 0 - рисовать 0C
    byte temp_x = CLOCK_WX + 12; 
    byte temp_y = CLOCK_WY;

    byte t = abs(temperature);
    byte dec_t = t / 10;
    byte edc_t = t % 10;
    
    // Получить цвет отображения значения температуры
    CRGB color;          
    if (isNightClock) 
      color = useTemperatureColorNight ? (temperature < -3 ? CRGB(0x000002) : (temperature > 3 ? CRGB(0x020000) : CRGB(0x020202))) : clockLED[0];
    else 
      color = useTemperatureColor ? CRGB(HEXtoInt(getTemperatureColor(temperature))) : CRGB::White;

    // Для правильного позиционирования - рисуем справа налево
    if (temperature == 0) {
      // При температуре = 0 - ресуем маленький значок C
      temp_x -= 3;  
      for(int i = 0; i < 3; i++) {
        drawPixelXY(getClockX(temp_x), temp_y + i, color);      
      }
      drawPixelXY(getClockX(temp_x + 1), temp_y, color);      
      drawPixelXY(getClockX(temp_x + 1), temp_y + 2, color);      
    }

    // Единицы градусов
    temp_x -= (edc_t == 1 ? 3 : 4);
    drawDigit3x5(edc_t, getClockX(temp_x), temp_y, color);
    temp_x += (dec_t == 0 && edc_t == 1 ? 1 : 0);

    // Десятки градусов
    if (dec_t != 0) {
      temp_x -= (dec_t == 1 ? 3 : 4);
      drawDigit3x5(dec_t, getClockX(temp_x), temp_y, color);
      temp_x += (dec_t == 1 ? 1 : 0);
    }
            
    // Нарисовать '+' или '-' если температура не 0
    // Горизонтальная черта - общая для '-' и '+'
    if (temperature != 0) {
       temp_x -= 4;
      for(int i = 0; i < 3; i++) {
        drawPixelXY(getClockX(temp_x + i), temp_y + 2, color);      
      }
      
      // Для плюcа - вертикальная черта
      if (temperature > 0) {
        drawPixelXY(getClockX(temp_x + 1), temp_y + 1, color);
        drawPixelXY(getClockX(temp_x + 1), temp_y + 3, color);
      }
      temp_x += 4;
    }    
  }
  
#endif
}

// нарисовать дату календаря
void drawCalendar(byte aday, byte amnth, int16_t ayear, boolean dots, int8_t X, int8_t Y) {
  
  // Число месяца
  drawDigit3x5(aday / 10, X, Y + 6, clockLED[0]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
  drawDigit3x5(aday % 10, X + 4, Y + 6, clockLED[0]);

  // разделитель числа/месяца
  if (dots) {
    drawPixelXY(getClockX(X + 7), Y + 5, clockLED[2]);
  }
  
  // Месяц
  drawDigit3x5(amnth / 10, X + 8, Y + 6, clockLED[1]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
  drawDigit3x5(amnth % 10, X + 12, Y + 6, clockLED[1]);

  // Год  
  drawDigit3x5(ayear / 1000, X, Y, clockLED[3]);
  drawDigit3x5((ayear / 100) % 10, X + 4, Y, clockLED[3]);
  drawDigit3x5((ayear / 10) % 10, X + 8, Y, clockLED[4]);
  drawDigit3x5(ayear % 10, X + 12, Y, clockLED[4]);
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
    byte prcBrightness = map8(globalBrightness,0,99);
    byte m10 = getByteForDigit(prcBrightness / 10);
    byte m01 = getByteForDigit(prcBrightness % 10);
    display.displayByte(_b_, _r_, m10, m01);
    display.point(false);
  } else if (wifi_print_ip) {
    // Четырехкратное нажатие кнопки запускает отображение по частям текущего IP лампы  
    if (dotFlag  && halfSec) {
      int value = atoi(GetToken(WiFi.localIP().toString(), wifi_print_idx + 1, '.').c_str()); 
      display.displayInt(value);
      display.point(false);
      wifi_print_idx++;
      if (wifi_print_idx>3) {
        wifi_print_idx = 0; 
        wifi_print_ip = false;
      }
    }
  } else {
    // Отображение часов - разделительное двоеточие...
    if (halfSec) display.point(dotFlag);
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
      if (halfSec) {
        display.displayClock(hour(),minute());
        display.setBrightness(isTurnedOff ? 1 : 7);
      }
    }
  }
#endif  
}

void overlayWrap() {
  // В оверлей отправляется полоса от y_low до y_high во всю штрину матрицы  
  int16_t thisLED = 0;  
  for (uint8_t i = 0; i < WIDTH; i++) {
    for (uint8_t j = y_overlay_low; j <= y_overlay_high; j++) {
      overlayLEDs[thisLED] = leds[getPixelNumber(i,j)];
      thisLED++;
    }
  }
}

void overlayUnwrap() {
  int16_t thisLED = 0;  
  for (uint8_t i = 0; i < WIDTH; i++) {
    for (uint8_t j = y_overlay_low; j <= y_overlay_high; j++) {
      leds[getPixelNumber(i, j)] = overlayLEDs[thisLED];
      thisLED++; 
    }
  }
}

void overlayWeatherWrap() {

  // Погода выводится - правая граница = правой границы часов, ширина вывода = 3 симв 3x5 + 2 пробела между - 11 симв.
  // Строка вывода - yw_overlay_low до yw_overlay_high, позиция отсчитывается от CLOCK_XC - позиции вывода часов
  int16_t thisLED = 0;
  int8_t xx = CLOCK_WX;
  for (int8_t i = xx; i < xx + 11; i++) {
    for (uint8_t j = yw_overlay_low; j <= yw_overlay_high; j++) {
      overlayWeather[thisLED] = leds[getPixelNumber(getClockX(i),j)];
      thisLED++;
    }
  }
}

void overlayWeatherUnwrap() {

  int16_t thisLED = 0;
  int8_t xx = CLOCK_WX;  // в часах - 4 символа, в погоде - 3 символа - т.е. позиция погоды сдвинута на одно знакоместо
  
  for (int8_t i = xx; i < xx + 11; i++) {
    for (uint8_t j = yw_overlay_low; j <= yw_overlay_high; j++) {
      leds[getPixelNumber(getClockX(i), j)] = overlayWeather[thisLED];
      thisLED++; 
    }
  }
}

void checkCalendarState() {
  if (millis() - showDateStateLastChange > (showDateState ? showDateDuration : showDateInterval) * 1000L) {
    showDateStateLastChange = millis();
    showDateState = !showDateState;
  }  
}

void contrastClock() {  
  for (byte i = 0; i < 5; i++) clockLED[i] = NORMAL_CLOCK_COLOR;
}

void contrastClockA() {  
  for (byte i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_1;
}

void contrastClockB() {
  for (byte i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_2;
}

void contrastClockC(){
  CRGB color = CONTRAST_COLOR_3;
  CRGB gc = CRGB(globalColor);
  if (color == gc) color = -color;
  for (byte i = 0; i < 5; i++) clockLED[i] = color;  
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
      case MC_WATERFALL:
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

  byte alrmHour;
  byte alrmMinute;
  
  dawnWeekDay = 0;
  if (!init_time || alarmWeekDay == 0) return;       // Время инициализировано? Хотя бы на один день будильник включен?

  int8_t alrmWeekDay = weekday()-1;                  // day of the week, Sunday is day 0   
  if (alrmWeekDay == 0) alrmWeekDay = 7;             // Sunday is day 7, Monday is day 1;

  byte h = hour();
  byte m = minute();
  byte w = weekday()-1;
  if (w == 0) w = 7;

  byte cnt = 0;
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

  // Serial.printf("Alarm: h:%d m:%d wd:%d\n", alrmHour, alrmMinute, alrmWeekDay);
  
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

  // Serial.printf("Dawn: h:%d m:%d wd:%d\n", dawnHour, dawnMinute, dawnWeekDay);

  Serial.print(String(F("Следующий рассвет в ")) + String(dawnHour)+ F(":") + String(dawnMinute) + ", " + getWeekdayString(dawnWeekDay));
}

// Проверка времени срабатывания будильника
void checkAlarmTime() {

  byte h = hour();
  byte m = minute();
  byte w = weekday()-1;
  if (w == 0) w = 7;

  // Будильник включен?
  if (init_time && dawnWeekDay > 0) {

    // Время срабатывания будильника после завершения рассвета
    byte alrmWeekDay = dawnWeekDay;
    byte alrmHour = dawnHour;
    byte alrmMinute = dawnMinute + dawnDuration;
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
         isAlarming = true;
         isAlarmStopped = false;
         loadingFlag = true;         
         thisMode = MC_DAWN_ALARM;
         setTimersForMode(thisMode);
         // Реальная продолжительность рассвета
         realDawnDuration = (alrmHour * 60L + alrmMinute) - (dawnHour * 60L + dawnMinute);
         if (realDawnDuration > dawnDuration) realDawnDuration = dawnDuration;
         // Отключмить таймер автоперехода в демо-режим
         idleTimer.setInterval(4294967295);
         #if (USE_MP3 == 1)
         if (useAlarmSound) PlayDawnSound();
         #endif
         sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
         Serial.println(String(F("Рассвет ВКЛ в "))+String(h)+ ":" + String(m));

         #if (USE_MQTT == 1)
         DynamicJsonDocument doc(256);
         String out;
         doc["act"]   = F("ALARM");
         doc["state"] = F("on");
         doc["type"]  = F("dawn");
         serializeJson(doc, out);      
         NotifyInfo(out);
         #endif
       }
    }
    
    delay(0); // Для предотвращения ESP8266 Watchdog Timer
    
    // При наступлении времени срабатывания будильника, если он еще не выключен пользователем - запустить режим часов и звук будильника
    if (alrmWeekDay == w && alrmHour == h && alrmMinute == m && isAlarming) {
      Serial.println(String(F("Рассвет Авто-ВЫКЛ в "))+String(h)+ ":" + String(m));
      isAlarming = false;
      isAlarmStopped = false;
      isPlayAlarmSound = true;
      setSpecialMode(1);      
      alarmSoundTimer.setInterval(alarmDuration * 60000L);
      alarmSoundTimer.reset();
      // Играть звук будильника
      // Если звук будильника не используется - просто запустить таймер.
      // До окончания таймера индикатор TM1637 будет мигать, лампа гореть ярко белым.
      #if (USE_MP3 == 1)
      if (useAlarmSound) {
        PlayAlarmSound();+++2
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
      NotifyInfo(out);
      #endif

    }

    delay(0); // Для предотвращения ESP8266 Watchdog Timer

    // Если рассвет начинался и остановлен пользователем и время начала рассвета уже прошло - сбросить флаги, подготовив их к следующему циклу
    if (isAlarmStopped && ((w * 1000L + h * 60L + m) > (alrmWeekDay * 1000L + alrmHour * 60L + alrmMinute + alarmDuration))) {
      isAlarming = false;
      isAlarmStopped = false;
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
    Serial.println(String(F("Будильник Авто-ВЫКЛ в "))+String(h)+ ":" + String(m));
    
    alarmSoundTimer.setInterval(4294967295);
    isPlayAlarmSound = false;
    StopSound(1000);   

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
    NotifyInfo(out);
    #endif

  }

  delay(0); // Для предотвращения ESP8266 Watchdog Timer

  #if (USE_MP3 == 1)
  // Плавное изменение громкости будильника
  if (fadeSoundTimer.isReady()) {
    if (fadeSoundDirection > 0) {
      // увеличение громкости
      dfPlayer.volumeUp();
      fadeSoundStepCounter--;
      if (fadeSoundStepCounter <= 0) {
        fadeSoundDirection = 0;
        fadeSoundTimer.setInterval(4294967295);
      }
    } else if (fadeSoundDirection < 0) {
      // Уменьшение громкости
      dfPlayer.volumeDown();
      fadeSoundStepCounter--;
      if (fadeSoundStepCounter <= 0) {
        isPlayAlarmSound = false;
        fadeSoundDirection = 0;
        fadeSoundTimer.setInterval(4294967295);
        StopSound(0);
      }
    }
  }
  #endif
  
  delay(0); // Для предотвращения ESP8266 Watchdog Timer    
}

void stopAlarm() {
  #if (USE_MP3 == 1)
  if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) {
    Serial.println(String(F("Рассвет ВЫКЛ в ")) + String(hour())+ ":" + String(minute()));
    isAlarming = false;
    isAlarmStopped = true;
    isPlayAlarmSound = false;
    cmd95 = "";

    alarmSoundTimer.setInterval(4294967295);

    // Во время работы будильника индикатор плавно мерцает.
    // После завершения работы - восстановить яркость индикатора

    #if (USE_TM1637 == 1)
    display.setBrightness(7);
    #endif

    StopSound(1000);

    resetModes();  
    setManualModeTo(false);
    
    if (saveSpecialMode){
       setSpecialMode(saveSpecialModeId);
    } else {
       setEffect(saveMode);
    }
    delay(0);    
    sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника

    DynamicJsonDocument doc(256);
    String out;
    doc["act"]   = F("ALARM");
    doc["state"] = F("off");
    doc["type"]  = F("stop");
    serializeJson(doc, out);      
    NotifyInfo(out);
  }
  #endif
}

// Проверка необходимости включения режима 1 по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
void checkAutoMode1Time() {
  if (AM1_effect_id <= -3 || AM1_effect_id >= MAX_EFFECT || !init_time) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало врема активации режима - активировать
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
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
void checkAutoMode2Time() {

  // Действие отличается от "Нет действия" и время установлено?
  if (AM2_effect_id <= -3 || AM2_effect_id >= MAX_EFFECT || !init_time) return;

  // Если сработал будильник - рассвет - режим не переключать - остаемся в режими обработки будильника
  if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) return;

  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало врема активации режима - активировать
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
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
void checkAutoMode3Time() {
  if (AM3_effect_id <= -3 || AM3_effect_id >= MAX_EFFECT || !init_time) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало врема активации режима - активировать
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
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
void checkAutoMode4Time() {
  if (AM4_effect_id <= -3 || AM4_effect_id >= MAX_EFFECT || !init_time) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало врема активации режима - активировать
  if (!AM4_running && AM4_hour == hrs && AM4_minute == mins) {
    AM4_running = true;
    SetAutoMode(4);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (AM4_running && (AM4_hour != hrs || AM4_minute != mins)) {
    AM4_running = false;
  }
}


// Выполнение включения режима 1,2,3,4 (amode) по установленному времени
// -3 - не используется; -2 - выключить матрицу; -1 - ночные часы, 0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
void SetAutoMode(byte amode) {

  byte   AM_hour, AM_minute;
  int8_t AM_effect_id;
  switch(amode) {
    case 1:  AM_hour = AM1_hour; AM_minute = AM1_minute; AM_effect_id = AM1_effect_id; break;
    case 2:  AM_hour = AM2_hour; AM_minute = AM2_minute; AM_effect_id = AM2_effect_id; break;
    case 3:  AM_hour = AM3_hour; AM_minute = AM3_minute; AM_effect_id = AM3_effect_id; break;
    case 4:  AM_hour = AM4_hour; AM_minute = AM4_minute; AM_effect_id = AM4_effect_id; break;
    default: return;
  }
  
  bool   no_action = false;
  
  String text = F("Авторежим ");
  text += String(amode) + F(" [") + String(AM_hour) + ":" + String(AM_minute) + F("] - ");

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
    setSpecialMode(0);
    
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
      text += F("демонcтрации эффектов:");
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
    Serial.println(text);  

    #if (USE_MQTT == 1)
    DynamicJsonDocument doc(256);
    String out;
    doc["act"]   = F("AUTO");
    doc["mode"]  = amode;
    doc["text"]  = text;
    serializeJson(doc, out);      
    NotifyInfo(out);
    #endif
  }
}

byte getByteForDigit(byte digit) {
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

void checkClockOrigin() {

  // Получить текущий тип размера горизонтальных часов: 0 - авто; 1 - шрифт 3х5; 2 - шрифт 5х7;
  c_size = getClockSizeType();
  
  if (allowVertical || allowHorizontal) {
    // Если ширина матрицы не позволяет расположить часы горизонтально - переключить в вертикальный режим
    if (CLOCK_ORIENT == 1 && !allowVertical) {
      CLOCK_ORIENT = 0;
      saveClockOrientation(CLOCK_ORIENT);
    }
    // Если высота матрицы не позволяет расположить часы вертикально - переключить в горизонтальный режим
    if (CLOCK_ORIENT == 0 && !allowHorizontal) {
      CLOCK_ORIENT = 1;
      saveClockOrientation(CLOCK_ORIENT);
    }
  } else {
    clockOverlayEnabled = false;
    saveClockOverlayEnabled(clockOverlayEnabled);
    return;
  }

  CLOCK_X = CLOCK_ORIENT == 0 ? (c_size == 1 ? CLOCK_X_H1 : CLOCK_X_H2) : CLOCK_X_V;
  CLOCK_Y = CLOCK_ORIENT == 0 ? (c_size == 1 ? CLOCK_Y_H1 : CLOCK_Y_H2) : CLOCK_Y_V;
  
  // ширина и высота отображения часов  
  byte cw = CLOCK_ORIENT == 0 ? 4*3 + 3*1 : 2*3 + 1; // гориз: 4 цифры * (шрифт 3 пикс шириной) 3 + пробела между цифрами) // ширина горизонтальных часов
                                                     // верт:  2 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами)  // ширина вертикальных часов
  byte ch = CLOCK_ORIENT == 0 ? 1*5 : 2*5 + 1;       // гориз: Одна строка цифр 5 пикс высотой                             // высота горизонтальных часов
                                                     // верт:  Две строки цифр 5 пикс высотой + 1 пробел между строкми     // высота вертикальных часовв

  while (CLOCK_X > 0 && CLOCK_X + cw > WIDTH)  CLOCK_X--;
  while (CLOCK_Y > 0 && CLOCK_Y + ch > HEIGHT) CLOCK_Y--;

  if (c_size == 1) {
    cw = 4*3 + 1;                                     // 4 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами)          // ширина календаря
    ch = 2*5 + 1;                                     // Две строки цифр 5 пикс высотой + 1 пробел между строкми             // высота календаря
    
    while (CALENDAR_X > 0 && CALENDAR_X + cw > WIDTH)  CALENDAR_X--; 
    while (CALENDAR_Y > 0 && CALENDAR_Y + ch > HEIGHT) CALENDAR_Y--;
  }

  CLOCK_XC = CLOCK_X;
  CALENDAR_XC = CALENDAR_X;
}

uint32_t getNightClockColorByIndex(byte idx) {
  uint32_t color = 0x010000;  // Red
  switch (idx) {
    case 0: color = 0x020000; break;  // Red
    case 1: color = 0x000200; break;  // Green
    case 2: color = 0x000002; break;  // Blue
    case 3: color = 0x000202; break;  // Cyan
    case 4: color = 0x020002; break;  // Magenta
    case 5: color = 0x020200; break;  // Yellow
    case 6: color = 0x020202; break;  // White
  }
  return color;
}

void printNtpServerName() {
  Serial.print(F("NTP-сервер "));
  Serial.print(ntpServerName);
  Serial.print(F(" -> "));
  Serial.println(timeServerIP);
}
