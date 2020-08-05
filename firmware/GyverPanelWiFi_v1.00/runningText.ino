// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------

int offset = WIDTH;

void runningText() {
  String text = "";
  if (wifi_print_ip_text) 
    text = wifi_current_ip;
  else if (init_time)
    switch (formatClock){
      case 0:
        text = clockCurrentText();
        break;
      case 1:
        text = clockCurrentText() + " " + dateCurrentTextShort();
        break;
      default:  
        text = clockCurrentText() + " " + dateCurrentTextLong();
        break;
    }
  else
    text = String(FIRMWARE_VER);

  uint32_t color = COLOR_TEXT_MODE;
  if (color == 0 || color > 2) color = globalTextColor;

  fillString(text, color);
}

void fillString(String text, uint32_t color) {
  if (loadingFlag) {
    offset = WIDTH;   // перемотка в правый край
    // modeCode = MC_TEXT;
    loadingFlag = false;    
#if (SMOOTH_CHANGE == 1)
    loadingFlag = thisMode == MC_TEXT && fadeMode < 2 ;
#else
    loadingFlag = false;        
#endif
    fullTextFlag = false;
  }
  
  FastLED.clear();
  byte i = 0, j = 0;
  while (text[i] != '\0') {
    if ((byte)text[i] > 191) {    // работаем с русскими буквами!
      i++;
    } else {
      drawLetter(j, text[i], offset + j * (LET_WIDTH + SPACE), color);
      i++;
      j++;
    }
  }
  fullTextFlag = false;

  offset--;
  if (offset < -j * (LET_WIDTH + SPACE)) {    // строка убежала
    offset = WIDTH + 3;
    fullTextFlag = true;
  }      
}

byte getTextY() {
  int8_t LH = LET_HEIGHT;
  if (LH > HEIGHT) LH = HEIGHT;
  int8_t offset_y = (HEIGHT - LH) / 2;     // по центру матрицы по высоте
  return offset_y; 
}

void drawLetter(uint8_t index, uint8_t letter, int16_t offset, uint32_t color) {
  int8_t LH = LET_HEIGHT;
  if (LH > HEIGHT) LH = HEIGHT;

  int8_t start_pos = 0, finish_pos = LET_WIDTH;
  int8_t offset_y = getTextY();
  
  CRGB letterColor;
  if (color == 1) letterColor = CHSV(byte(offset * 10), 255, 255);
  else if (color == 2) letterColor = CHSV(byte(index * 30), 255, 255);
  else letterColor = color;

  if (offset < -LET_WIDTH || offset > WIDTH) return;
  if (offset < 0) start_pos = -offset;
  if (offset > (WIDTH - LET_WIDTH)) finish_pos = WIDTH - offset;

  for (byte i = start_pos; i < finish_pos; i++) {
    int thisByte;
    if (MIRR_V) thisByte = getFont((byte)letter, LET_WIDTH - 1 - i);
    else thisByte = getFont((byte)letter, i);

    for (byte j = 0; j < LH; j++) {
      boolean thisBit;

      if (MIRR_H) thisBit = thisByte & (1 << j);
      else thisBit = thisByte & (1 << (LH - 1 - j));

      // рисуем столбец (i - горизонтальная позиция, j - вертикальная)
      if (thisBit) leds[getPixelNumber(offset + i, offset_y + j)] = letterColor;
    }
  }
}

// ------------- СЛУЖЕБНЫЕ ФУНКЦИИ --------------

// интерпретатор кода символа в массиве fontHEX (для Arduino IDE 1.8.* и выше)
uint8_t getFont(uint8_t font, uint8_t row) {
  font = font - '0' + 16;   // перевод код символа из таблицы ASCII в номер согласно нумерации массива
  if (font <= 90) {
    return pgm_read_byte(&(fontHEX[font][row]));     // для английских букв и символов
  } else if (font >= 112 && font <= 159) {           // и пизд*ц для русских
    return pgm_read_byte(&(fontHEX[font - 17][row]));
  } else if (font >= 96 && font <= 111) {
    return pgm_read_byte(&(fontHEX[font + 47][row]));
  }
  return 0;
}
