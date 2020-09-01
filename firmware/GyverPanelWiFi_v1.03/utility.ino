// служебные функции

// шрифт 5х7
const uint8_t font5x7[][5] PROGMEM = {
  {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 0 0x30 48
  {0x00, 0x42, 0x7f, 0x40, 0x00}, // 1 0x31 49
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2 0x32 50
  {0x21, 0x41, 0x45, 0x4b, 0x31}, // 3 0x33 51
  {0x18, 0x14, 0x12, 0x7f, 0x10}, // 4 0x34 52
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5 0x35 53
  {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 6 0x36 54
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7 0x37 55
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8 0x38 56
  {0x06, 0x49, 0x49, 0x29, 0x1e}, // 9 0x39 57
};

// шрифт 3х5 квадратный
const uint8_t font3x5[][3] PROGMEM = {
  {0b11111, 0b10001, 0b11111},    // 0
  {0b00000, 0b11111, 0b00000},    // 1
  {0b10111, 0b10101, 0b11101},    // 2
  {0b10101, 0b10101, 0b11111},    // 3
  {0b11100, 0b00100, 0b11111},    // 4
  {0b11101, 0b10101, 0b10111},    // 5
  {0b11111, 0b10101, 0b10111},    // 6
  {0b10000, 0b10000, 0b11111},    // 7
  {0b11111, 0b10101, 0b11111},    // 8
  {0b11101, 0b10101, 0b11111},    // 9
};

// шрифт 3х5 скруглённый
const uint8_t font3x5_s[][3] PROGMEM = {
  {0b01111, 0b10001, 0b11110},    // 0
  {0b01000, 0b11111, 0b00000},    // 1
  {0b10011, 0b10101, 0b01001},    // 2
  {0b10001, 0b10101, 0b11111},    // 3
  {0b11100, 0b00100, 0b11111},    // 4
  {0b11101, 0b10101, 0b10111},    // 5
  {0b01111, 0b10101, 0b10111},    // 6
  {0b10000, 0b10011, 0b11100},    // 7
  {0b11111, 0b10101, 0b11111},    // 8
  {0b11101, 0b10101, 0b11110},    // 9
};

// отображаем счёт для игр
void displayScore(byte score) {
  if (SCORE_SIZE == 0) {
    if (score > 9) drawDigit3x5(score / 10, WIDTH / 2 - 4, HEIGHT / 2 - 3, GLOBAL_COLOR_1);
    drawDigit3x5(score % 10, WIDTH / 2, HEIGHT / 2 - 3, GLOBAL_COLOR_2);
  } else {
    if (score > 9) drawDigit5x7(score / 10, WIDTH / 2 - 6, HEIGHT / 2 - 4, GLOBAL_COLOR_1);
    drawDigit5x7(score % 10, WIDTH / 2, HEIGHT / 2 - 4, GLOBAL_COLOR_2);
  }
}

// нарисовать цифру шрифт 3х5 квадратный
void drawDigit3x5(byte digit, int8_t X, int8_t Y, CRGB color) {
  if (digit > 9) return;
  for (byte x = 0; x < 3; x++) {
    byte thisByte = pgm_read_byte(&(font3x5[digit][x]));
    for (byte y = 0; y < 5; y++) {
      if (y + Y > HEIGHT) continue;
      if (thisByte & (1 << y)) drawPixelXY(getClockX(x + X), y + Y, color);
    }
  }
}

// нарисовать цифру шрифт 3х5 скруглённый
void drawDigit3x5_s(byte digit, int8_t X, int8_t Y, CRGB color) {
  if (digit > 9) return;
  for (byte x = 0; x < 3; x++) {
    byte thisByte = pgm_read_byte(&(font3x5_s[digit][x]));
    for (byte y = 0; y < 5; y++) {
      if (y + Y > HEIGHT) continue;
      if (thisByte & (1 << y)) drawPixelXY(getClockX(x + X), y + Y, color);
    }
  }
}

// нарисовать цифру шрифт 5х7
void drawDigit5x7(byte digit, byte X, byte Y, CRGB color) {
  if (digit > 9) return;
  for (byte x = 0; x < 5; x++) {
    byte thisByte = pgm_read_byte(&(font5x7[digit][x]));
    for (byte y = 0; y < 7; y++) {
      if (y + Y > HEIGHT) continue;
      if (thisByte & (1 << 6 - y)) drawPixelXY(getClockX(x + X), y + Y, color);
    }
  }
}

// Заглушка чтения кнопок управления игрой
boolean checkButtons() {
  if (buttons != 4) return true;
  return false;
}

// залить все
void fillAll(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}

// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, CRGB color) {
  if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
  int thisPixel = getPixelNumber(x, y);
  leds[thisPixel] = color;
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(int thisSegm) {
  int thisPixel = thisSegm;
  if (thisPixel < 0 || thisPixel > NUM_LEDS - 1) return 0;
  return (((uint32_t)leds[thisPixel].r << 16) | ((long)leds[thisPixel].g << 8 ) | (long)leds[thisPixel].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(int8_t x, int8_t y) {
  return getPixColor(getPixelNumber(x, y));
}

// **************** НАСТРОЙКА МАТРИЦЫ ****************
#if (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y

#elif (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y x

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y x

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y (WIDTH - x - 1)

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y y

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y (WIDTH - x - 1)

#else
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y
#pragma message "Wrong matrix parameters! Set to default"

#endif

// получить номер пикселя в ленте по координатам
uint16_t getPixelNumber(int8_t x, int8_t y) {
  if ((THIS_Y % 2 == 0) || MATRIX_TYPE) {               // если чётная строка
    return (THIS_Y * _WIDTH + THIS_X);
  } else {                                              // если нечётная строка
    return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);
  }
}

// функция плавного угасания цвета для всех пикселей
void fader(byte step) {
  for (byte i = 0; i < WIDTH; i++) {
    for (byte j = 0; j < HEIGHT; j++) {
      fadePixel(i, j, step);
    }
  }
}

void fadePixel(byte i, byte j, byte step) {     // новый фейдер
  int pixelNum = getPixelNumber(i, j);
  if (getPixColor(pixelNum) == 0) return;

  if (leds[pixelNum].r >= 5 ||
      leds[pixelNum].g >= 5 ||
      leds[pixelNum].b >= 5) {
    leds[pixelNum].fadeToBlackBy(step);
  } else {
    leds[pixelNum] = 0;
  }
}

// hex string to uint32_t
uint32_t HEXtoInt(String hexValue) {

  hexValue.toUpperCase();
  if (hexValue.charAt(0) == '#') {
    hexValue = hexValue.substring(1);
  }

  if (hexValue.startsWith("0X")) {
    hexValue = hexValue.substring(2);
  }

  byte tens, ones, number1, number2, number3;
  tens = (hexValue[0] < '9') ? hexValue[0] - '0' : hexValue[0] - '7';
  ones = (hexValue[1] < '9') ? hexValue[1] - '0' : hexValue[1] - '7';
  number1 = (16 * tens) + ones;

  tens = (hexValue[2] < '9') ? hexValue[2] - '0' : hexValue[2] - '7';
  ones = (hexValue[3] < '9') ? hexValue[3] - '0' : hexValue[3] - '7';
  number2 = (16 * tens) + ones;

  tens = (hexValue[4] < '9') ? hexValue[4] - '0' : hexValue[4] - '7';
  ones = (hexValue[5] < '9') ? hexValue[5] - '0' : hexValue[5] - '7';
  number3 = (16 * tens) + ones;

  return ((uint32_t)number1 << 16 | (uint32_t)number2 << 8 | number3 << 0);
}

uint32_t CountTokens(String str, char separator) {

  uint32_t count = 0;
  int pos = 0;
  String l_str = str;

  l_str.trim();
  if (l_str.length() <= 0) return 0;
  pos = l_str.indexOf(separator);
  while (pos >= 0) {
    count++;
    pos = l_str.indexOf(separator, pos + 1);
  }
  return ++count;
}

String GetToken(String str, uint32_t index, char separator) {

  uint32_t count = CountTokens(str, separator);

  if (count <= 1 || index < 1 || index > count) return str;

  uint32_t pos_start = 0;
  uint32_t pos_end = str.length();

  count = 0;
  for (uint32_t i = 0; i < pos_end; i++) {
    if (str.charAt(i) == separator) {
      count++;
      if (count == index) {
        pos_end = i;
        break;
      } else {
        pos_start = i + 1;
      }
    }
  }
  return str.substring(pos_start, pos_end);
}

uint32_t getColorInt(CRGB color) {
  return color.r << 16 | color.g << 8 | color.b;
}

// Вычисление значения яркости эффектов по отношению к общей яркости
// Общая яркость регулируется через FastLED.setBrightness(); 
// Вычисленная яркость эффекта влияет на комонент яркости V в модели HCSV
byte getBrightnessCalculated(byte brightness, byte contrast) {
  // В данном варианте общая яркость не учитывается в расчете
  return map8(contrast, 16, 255);
}

String getMonthString(byte month) {
  String sMnth = "";
  switch (month) {
    case  1: sMnth = F("января");   break;
    case  2: sMnth = F("февраля");  break;
    case  3: sMnth = F("марта");    break;
    case  4: sMnth = F("апреля");   break;
    case  5: sMnth = F("мая");      break;
    case  6: sMnth = F("июня");     break;
    case  7: sMnth = F("июля");     break;
    case  8: sMnth = F("августа");  break;
    case  9: sMnth = F("сентября"); break;
    case 10: sMnth = F("октября");  break;
    case 11: sMnth = F("ноября");   break;
    case 12: sMnth = F("декабря");  break;
  }  
  return sMnth;
}

String getMonthShortString(byte month) {
  String sMnth = "";
  switch (month) {
    case  1: sMnth = F("янв"); break;
    case  2: sMnth = F("фев"); break;
    case  3: sMnth = F("мар"); break;
    case  4: sMnth = F("апр"); break;
    case  5: sMnth = F("май"); break;
    case  6: sMnth = F("июн"); break;
    case  7: sMnth = F("июл"); break;
    case  8: sMnth = F("авг"); break;
    case  9: sMnth = F("сен"); break;
    case 10: sMnth = F("окт"); break;
    case 11: sMnth = F("ноя"); break;
    case 12: sMnth = F("дек"); break;
  }  
  return sMnth;
}

String getWeekdayString(byte wd) {
  String str = "";
  switch (wd) {
    case  1: str = F("понедельник"); break;
    case  2: str = F("вторник");     break;
    case  3: str = F("среда");       break;
    case  4: str = F("четверг");     break;
    case  5: str = F("пятница");     break;
    case  6: str = F("суббота");     break;
    case  7: str = F("воскресенье"); break;
  }  
  return str;
}

String getWeekdayShortString(byte wd) {
  String str = "";
  switch (wd) {
    case  1: str = F("пон"); break;
    case  2: str = F("втр"); break;
    case  3: str = F("срд"); break;
    case  4: str = F("чтв"); break;
    case  5: str = F("птн"); break;
    case  6: str = F("сбт"); break;
    case  7: str = F("вск"); break;
  }  
  return str;
}

String getWeekdayShortShortString(byte wd) {
  String str = "";
  switch (wd) {
    case  1: str = F("пн"); break;
    case  2: str = F("вт"); break;
    case  3: str = F("ср"); break;
    case  4: str = F("чт"); break;
    case  5: str = F("пт"); break;
    case  6: str = F("сб"); break;
    case  7: str = F("вс"); break;
  }  
  return str;
}


// ---------- Склонение числительных остатка времени -----------

String WriteDays(int iDays) {
  int iDays2 = (iDays / 10) % 10;
  iDays = iDays %10;
  if (iDays2 == 1) return F(" дней");  
  if (iDays  == 1) return F(" день");
  if (iDays  >= 2 && iDays <= 4) return F(" дня");
  return F(" дней");
}

String WriteHours(int iHours) {
  if (iHours == 1 || iHours == 21) return F(" час");
  if ((iHours >= 2 && iHours <= 4) || (iHours >= 22 && iHours <= 24))return F(" часа");
  return F(" часов");
}

String WriteMinutes(int iMinutes){
  if (iMinutes >= 5 && iMinutes <= 20) return F(" минут");
  iMinutes = iMinutes %10;
  if (iMinutes == 1) return F(" минута");
  if (iMinutes >= 2 && iMinutes <= 4) return F(" минуты");
  return F(" минут");
}

String WriteSeconds(int iSeconds){
  if (iSeconds >= 5 && iSeconds <= 20) return F(" секунд");
  iSeconds = iSeconds %10;
  if (iSeconds == 1) return F(" секунда");
  if (iSeconds >= 2 && iSeconds <= 4) return F(" секунды");
  return F(" секунд");
}

// ------------------------- CRC16 -------------------------

uint16_t getCrc16(uint8_t * data, uint16_t len)
{
    uint8_t lo;
    union
    {
        uint16_t value;
        struct { uint8_t lo, hi; } bytes;
    } crc;
 
    crc.value = 0xFFFF;
 
    while ( len-- )
    {
        lo = crc.bytes.lo;
        crc.bytes.lo = crc.bytes.hi;
        crc.bytes.hi = lo ^ *data++;
 
        uint8_t mask = 1;
 
        if ( crc.bytes.hi & mask ) crc.value ^= 0x0240;
        if ( crc.bytes.hi & ( mask << 1 ) ) crc.value ^= 0x0480;
        if ( crc.bytes.hi & ( mask << 2 ) ) crc.bytes.hi ^= 0x09;
        if ( crc.bytes.hi & ( mask << 3 ) ) crc.bytes.hi ^= 0x12;
        if ( crc.bytes.hi & ( mask << 4 ) ) crc.bytes.hi ^= 0x24;
        if ( crc.bytes.hi & ( mask << 5 ) ) crc.bytes.hi ^= 0x48;
        if ( crc.bytes.hi & ( mask << 6 ) ) crc.bytes.hi ^= 0x90;
        if ( crc.bytes.hi & ( mask << 7 ) ) crc.value ^= 0x2001;
    }
 
    return crc.value;
}

uint8_t wrapX(int8_t x) {
  return (x + WIDTH) % WIDTH;
}

uint8_t wrapY(int8_t y) {
  return (y + HEIGHT) % HEIGHT;
}

// Сдвиг всей матрицы вниз
void shiftDown() {
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }
}

// Сдвиг всей матрицы вверх
void shiftUp() {
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = HEIGHT - 1; y > 0; y--) {
      drawPixelXY(x, y, getPixColorXY(x, y - 1));
    }
  }
}

// Сдвиг всей матрицы по диагонали
void shiftDiag() {
  for (byte y = 0; y < HEIGHT - 1; y++) {
    for (byte x = WIDTH - 1; x > 0; x--) {
      drawPixelXY(x, y, getPixColorXY(x - 1, y + 1));
    }
  }
}
