// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------

int16_t offset;

void runningText() {
  String text = "";
  if (thisMode == MC_TEXT) {
    FastLED.clear();    
    // Включен режим (не оверлей!) бегущей строки
    if (wifi_print_ip_text) 
      text = wifi_current_ip;
    else if (init_time)
      text = clockCurrentText();
    else
      text = String(FIRMWARE_VER);
  } else {
    // Вывод текста оверлея бегущей строки
    textHasDateTime = init_time && (currentText.indexOf("{D") >= 0 || currentText.indexOf("{R") >= 0 || currentText.indexOf("{P") >= 0 || currentText.indexOf("{S") >= 0);  
    if (textHasDateTime) {
      text = processDateMacrosInText(currentText);          // Обработать строку, превратив макросы даты в отображаемые значения
      if (text.length() == 0) {                             // Если дата еще не инициализирована - вернет другую строку, не требующую даты
         fullTextFlag = true;                               // Если нет строк, не зависящих от даты - вернется пустая строка - отображать нечего
         return;
      }      
    } else {
      text = currentText;                                   // Строка не содержит изменяемых во времени компонентов - отобразить ее как есть
    }
  }

  fillString(text);
}

void fillString(String text) {
  if (loadingTextFlag) {
    offset = pWIDTH;   // перемотка в правый край
    // modeCode = MC_TEXT;
    loadingTextFlag = false;    
#if (SMOOTH_CHANGE == 1)
    loadingTextFlag = thisMode == MC_TEXT && fadeMode < 2 ;
#else
    loadingTextFlag = false;        
#endif
  }

  uint32_t color;

  // Для многоцветной строки начальный цвет хранится в textColor[0]
  // Задан ли специальный цвет отображения строки?
  // Если режим цвета - монохром (0) или задан неверно (>2) - использовать глобальный или специальный цвет
  
  uint8_t i = 0, j = 0, pos = 0, modif = 0;
  
  while (text[i] != '\0') {

    // Если строка - многоцветная (содержит несколько макросов определения цвета) - определить каким цветом выводится текущая буква строки
    if (textHasMultiColor) {
      if (pos < (MAX_COLORS - 1) && i >= textColorPos[pos+1]) {      
          pos++;
      }      
      color = pos < MAX_COLORS ? textColor[pos] : globalTextColor;
    } else {
      // 0 - монохром: 1 - радуга; 2 -  каждая буква свой цвет
      color = COLOR_TEXT_MODE;
      if (color == 0 || color > 2) {
        color = useSpecialTextColor ? specialTextColor : globalTextColor;  
      }
    }

    // Определились с цветом - выводим очередную букву  
    if ((uint8_t)text[i] > 191) {    // работаем с русскими буквами!
      modif = (uint8_t)text[i];
      i++;
    } else {      
      drawLetter(j, text[i], modif, offset + j * (LET_WIDTH + SPACE), color);
      modif = 0;
      i++;
      j++;
    }
  }
  fullTextFlag = false;

  // Строка убежала?
  if (offset < -j * (LET_WIDTH + SPACE)) {
    offset = pWIDTH + 3;
    fullTextFlag = true;
  }      
}

uint8_t getTextY() {
  int8_t LH = LET_HEIGHT;
  if (LH > pHEIGHT) LH = pHEIGHT;
  int8_t offset_y = (pHEIGHT - LH) / 2;     // по центру матрицы по высоте
  return offset_y; 
}

void drawLetter(uint8_t index, uint8_t letter, uint8_t modif, int16_t offset, uint32_t color) {
  int8_t LH = LET_HEIGHT;
  if (LH > HEIGHT) LH = pHEIGHT;

  int8_t start_pos = 0, finish_pos = LET_WIDTH;
  int8_t offset_y = getTextY();
  
  CRGB letterColor;
  if (color == 1) letterColor = CHSV(uint8_t(offset * 10), 255, 255);
  else if (color == 2) letterColor = CHSV(uint8_t(index * 30), 255, 255);
  else letterColor = color;

  if (offset < -LET_WIDTH || offset > pWIDTH) return;
  if (offset < 0) start_pos = -offset;
  if (offset > (pWIDTH - LET_WIDTH)) finish_pos = pWIDTH - offset;

  for (uint8_t i = start_pos; i < finish_pos; i++) {
    uint16_t thisByte; // байт колонки i отображаемого символа шрифта
    uint16_t diasByte; // байт колонки i отображаемого диакритического символа
    int8_t   diasOffs; // смещение по Y отображения диакритического символа: diasOffs > 0 - позиция над основной буквой; diasOffs < 0 - позиция ниже основной буквы
    uint16_t pn;       // номер пикселя в массиве leds[]
    
    if (MIRR_V) {
      thisByte = getFont(letter, modif, LET_WIDTH - 1 - i);
      diasByte = getDiasByte(letter, modif, LET_WIDTH - 1 - i);
    } else {
      thisByte = getFont(letter, modif, i);
      diasByte = getDiasByte(letter, modif, i);
    }
    diasOffs = getDiasOffset(letter, modif);

    for (uint8_t j = 0; j < LH; j++) {
      bool thisBit;

      if (MIRR_H) thisBit = thisByte & (1 << j);
      else        thisBit = thisByte & (1 << (LH - 1 - j));

      // рисуем столбец буквы шрифта (i - горизонтальная позиция, j - вертикальная)      
      if (thisBit) { 
        int8_t y = offset_y + j;
        if (y >= 0 && y < pHEIGHT) {
          pn = getPixelNumber(offset + i, offset_y + j);
          if (pn < NUM_LEDS) {
            leds[pn] = letterColor;
          }
        }
      }

      if (MIRR_H) thisBit = diasByte & (1 << j);
      else        thisBit = diasByte & (1 << (LH - 1 - j));

      // рисуем столбец диакритического символа (i - горизонтальная позиция, j - вертикальная)      
      if (thisBit) { 
        int8_t y = offset_y + j + diasOffs;
        if (y >= 0 && y < pHEIGHT) {
          pn = getPixelNumber(offset + i, y);
          if (pn >= 0 && pn < NUM_LEDS) {
            leds[pn] = letterColor;
          }
        }
      }
    }
  }
}

// Сдвинуть позицию отображения строки
void shiftTextPosition() {
  offset--;
}

// получить строку статусов массива строк текстов бегущей строки
//  0 - серый - пустая
//  1 - черный - отключена
//  2 - зеленый - активна - просто текст? без макросов
//  3 - фиолетовый - активна, содержит макросы кроме даты
//  4 - синий - активная, содержит макрос даты
//  5 - красный - для строки 0 - это управляющая строка
String getTextStates() {
  uint8_t size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  char buf[size + 1];
  memset(buf, '\0', size + 1);
  for (uint8_t i=0; i < size; i++) {
    String text = textLines[i];    
    char c = '0';    // статус - пустая
    if (text.length() > 0) {
      if (i == 0 && text[0] == '#')
        c = '5';     // статус - строка 0 - управляющая последовательность
      else if (text[0] == '-' || text.indexOf("{-}") >=0)
        c = '1';     // статус - отключена
      else if (text.indexOf("{") < 0)
        c = '2';     // статус - текст без макросов
      else if (text.indexOf("{") >= 0 && text.indexOf("{D") < 0 && text.indexOf("{R") < 0 && text.indexOf("{P") < 0 && text.indexOf("{S") < 0)
        c = '3';     // статус - текст с макросами, но без дато-зависимых макросов
      else if (text.indexOf("{D") >= 0 || text.indexOf("{R") >= 0 || text.indexOf("{P") >= 0 || text.indexOf("{S") >= 0)
        c = '4';     // статус - текст с дато-зависимыми макросами
    }
    buf[i] = c;
  }  
  return String(buf);
}

#if (BIG_FONT == 0)
 // Шрифт меньше/равно 8 точек - достаточно байта
 #define read_char pgm_read_byte
#else
 // Шрифт меньше/равно 16 точек - достаточно двух байт (word)
 #define read_char pgm_read_word
#endif

// ------------- СЛУЖЕБНЫЕ ФУНКЦИИ --------------

// интерпретатор кода символа в массиве fontHEX (для Arduino IDE 1.8.* и выше)
uint16_t getFont(uint8_t font, uint8_t modif, uint8_t row) {
  font = font - '0' + 16;   // перевод код символа из таблицы ASCII в номер согласно нумерации массива  
  if (font <= 94) {
    return read_char(&(fontHEX[font][row]));       // для английских букв и символов
  } else if (modif == 209 && font == 116) {        // є
    return read_char(&(fontHEX[161][row])); 
  } else if (modif == 209 && font == 118) {        // і
      return read_char(&(fontHEX[73][row])); 
  } else if (modif == 209 && font == 119) {        // ї
      return read_char(&(fontHEX[162][row])); 
  } else if (modif == 209 && font == 113) {        // ё
      return read_char(&(fontHEX[132][row])); 
  } else if (modif == 208 && font == 100) {        // Є
      return read_char(&(fontHEX[160][row])); 
  } else if (modif == 208 && font == 102) {        // І
    return read_char(&(fontHEX[41][row])); 
  } else if (modif == 208 && font == 103) {        // Ї
    return read_char(&(fontHEX[41][row])); 
  } else if (modif == 208 && font == 97) {         // Ё
    return read_char(&(fontHEX[100][row]));     
  } else if ((modif == 208 || modif == 209) && font >= 112 && font <= 159) {         // и пизд*ц для русских
    return read_char(&(fontHEX[font - 17][row]));
  } else if ((modif == 208 || modif == 209) && font >= 96 && font <= 111) {
    return read_char(&(fontHEX[font + 47][row]));
  } else if (modif == 194 && font == 144) {                                          // Знак градуса '°'
    return read_char(&(fontHEX[159][row]));
  } else if (modif == 195) {                                          // ß
    switch (font) {
      case 127: return read_char(&(fontHEX[163][row])); // ß - 195 127 - 163
      case 100: return read_char(&(fontHEX[33][row]));  // Ä - 195 100 - 33
      case 118: return read_char(&(fontHEX[47][row]));  // Ö - 195 118 - 47
      case 124: return read_char(&(fontHEX[53][row]));  // Ü - 195 124 - 53
      case 132: return read_char(&(fontHEX[65][row]));  // ä - 195 132 - 65
      case 150: return read_char(&(fontHEX[79][row]));  // ö - 195 150 - 79
      case 156: return read_char(&(fontHEX[85][row]));  // ü - 195 156 - 85    
    }
  } else if (modif == 196 || modif == 197) {                                         // Буквы литовского алфавита  Ą Č Ę Ė Į Š Ų Ū Ž ą č ę ė į š ų ū ž
    switch (font) {
      case 100: return read_char(&(fontHEX[33][row])); //Ą 196   100  -     33
      case 108: return read_char(&(fontHEX[35][row])); //Č 196   108  -     35
      case 120: return read_char(&(fontHEX[37][row])); //Ę 196   120  -     37
      case 118: return read_char(&(fontHEX[37][row])); //Ė 196   118  -     37
      case 142: return read_char(&(fontHEX[41][row])); //Į 196   142  -     41
      case 128: return read_char(&(fontHEX[51][row])); //Š 197   128  -     51
      case 146: return read_char(&(fontHEX[53][row])); //Ų 197   146  -     53
      case 138: return read_char(&(fontHEX[53][row])); //Ū 197   138  -     53
      case 157: return read_char(&(fontHEX[58][row])); //Ž 197   157  -     58
      case 101: return read_char(&(fontHEX[65][row])); //ą 196   101  -     65
      case 109: return read_char(&(fontHEX[67][row])); //č 196   109  -     67  
      case 121: return read_char(&(fontHEX[69][row])); //ę 196   121  -     69
      case 119: return read_char(&(fontHEX[69][row])); //ė 196   119  -     69
      case 143: return read_char(&(fontHEX[73][row])); //į 196   143  -     73
      case 129: return read_char(&(fontHEX[83][row])); //š 197   129  -     83
      case 147: return read_char(&(fontHEX[85][row])); //ų 197   147  -     85
      case 139: return read_char(&(fontHEX[85][row])); //ū 197   139  -     85
      case 158: return read_char(&(fontHEX[90][row])); //ž 197   158  -     90
    }
  }
  return 0;
}

uint16_t getDiasByte(uint8_t font, uint8_t modif, uint8_t row) {
  font = font - '0' + 16;   // перевод код символа из таблицы ASCII в номер согласно нумерации массива
  if ((modif == 208 && font == 97) || (modif == 195 && (font == 100 || font == 118 || font == 124))) {         // Ё, немец. Ä,Ö,Ü
    return read_char(&(diasHEX[0][row])); 
  } else if ((modif == 209 && font == 113) || (modif == 195 && (font == 132 || font == 150 || font == 156))) { // ё, немец. ä,ö,ü
    return read_char(&(diasHEX[0][row])); 
  } else if ((modif == 208) && font == 103) {      // Ї
    return read_char(&(diasHEX[0][row])); 
  } else if (modif == 196 || modif == 197) {                                           // Буквы литовского алфавита  Ą Č Ę Ė Į Š Ų Ū Ž ą č ę ė į š ų ū ž
    // 0 - Č - перевернутая крышечка над заглавной буквой Č Ž č ž
    // 1 - Ė - точка над заглавной буквой Ė ė
    // 2 - Ū - надстрочная черта над заглавной буквой Ū ū
    // 3 - Ą - хвостик снизу букв Ą ą Ę ę ų - смещение к правому краю буквы
    // 4 - Į - хвостик снизу букв Į į Ų     - по центру буквы    
    switch (font) {
      case 100: return read_char(&(diasHEX[4][row])); //Ą 196   100  -     33
      case 108: return read_char(&(diasHEX[1][row])); //Č 196   108  -     35
      case 120: return read_char(&(diasHEX[4][row])); //Ę 196   120  -     37
      case 118: return read_char(&(diasHEX[2][row])); //Ė 196   118  -     37
      case 142: return read_char(&(diasHEX[5][row])); //Į 196   142  -     41
      case 128: return read_char(&(diasHEX[1][row])); //Š 197   128  -     51
      case 146: return read_char(&(diasHEX[5][row])); //Ų 197   146  -     53
      case 138: return read_char(&(diasHEX[3][row])); //Ū 197   138  -     53
      case 157: return read_char(&(diasHEX[1][row])); //Ž 197   157  -     58
      case 101: return read_char(&(diasHEX[4][row])); //ą 196   101  -     65
      case 109: return read_char(&(diasHEX[1][row])); //č 196   109  -     67  
      case 121: return read_char(&(diasHEX[4][row])); //ę 196   121  -     69
      case 119: return read_char(&(diasHEX[2][row])); //ė 196   119  -     69
      case 143: return read_char(&(diasHEX[5][row])); //į 196   143  -     73
      case 129: return read_char(&(diasHEX[1][row])); //š 197   129  -     83
      case 147: return read_char(&(diasHEX[4][row])); //ų 197   147  -     85
      case 139: return read_char(&(diasHEX[3][row])); //ū 197   139  -     85
      case 158: return read_char(&(diasHEX[1][row])); //ž 197   158  -     90
    }
  }
  return 0;
}

int8_t getDiasOffset(uint8_t font, uint8_t modif) {
  font = font - '0' + 16;   // перевод код символа из таблицы ASCII в номер согласно нумерации массива
  if ((modif == 208 && font == 97) || (modif == 195 && (font == 100 || font == 118 || font == 124))) {         // Ё, немец. Ä,Ö,Ü
    return 3; 
  } else if ((modif == 209 && font == 113) || (modif == 195 && (font == 132 || font == 150 || font == 156))) { // ё, немец. ä,ö,ü
    #if (BIG_FONT == 0)
      return 1; 
    #else
      return 0; 
    #endif  
  } else if ((modif == 208) && font == 103) {      // Ї
    return 3; 
  } else if (modif == 196 || modif == 197) {       // Буквы литовского алфавита  Ą Č Ę Ė Į Š Ų Ū Ž ą č ę ė į š ų ū ž
    // Смещение надстрочных заглавных - 3
    // Смещение надстрочных маленьких букв - 0 или 1
    // Смещение подстрочного символа -1
    #if (BIG_FONT == 0)
      switch (font) {
        case 100: return -1; //Ą 196   100
        case 108: return  2; //Č 196   108
        case 120: return -1; //Ę 196   120
        case 118: return  3; //Ė 196   118
        case 142: return -1; //Į 196   142
        case 128: return  2; //Š 197   128
        case 146: return -1; //Ų 197   146
        case 138: return  3; //Ū 197   138
        case 157: return  2; //Ž 197   157
        case 101: return -1; //ą 196   101
        case 109: return  0; //č 196   109
        case 121: return -1; //ę 196   121
        case 119: return  1; //ė 196   119
        case 143: return -1; //į 196   143
        case 129: return  0; //š 197   129
        case 147: return -1; //ų 197   147
        case 139: return  1; //ū 197   139
        case 158: return  0; //ž 197   158
      }
    #else
      switch (font) {
        case 100: return -1; //Ą 196   100
        case 108: return  3; //Č 196   108
        case 120: return -1; //Ę 196   120
        case 118: return  3; //Ė 196   118
        case 142: return -1; //Į 196   142
        case 128: return  3; //Š 197   128
        case 146: return -1; //Ų 197   146
        case 138: return  3; //Ū 197   138
        case 157: return  3; //Ž 197   157
        case 101: return -1; //ą 196   101
        case 109: return  0; //č 196   109
        case 121: return -1; //ę 196   121
        case 119: return  0; //ė 196   119
        case 143: return -1; //į 196   143
        case 129: return  0; //š 197   129
        case 147: return -1; //ų 197   147
        case 139: return  0; //ū 197   139
        case 158: return  0; //ž 197   158
      }
    #endif
  }
  return 0;
}

// Получить / установить настройки отображения очередного текста бегущей строки
// Если нет строк, готовых к отображению (например все строки отключены) - вернуть false - 'нет готовых строк'
bool prepareNextText(String text) {  
  // Если есть активная строка текущего момента - отображать ее 
  int8_t nextIdx = momentTextIdx >= 0 ? momentTextIdx : nextTextLineIdx;

  textShowTime = -1;              // Если больше нуля - сколько времени отображать бегущую строку в секундах; Если 0 - используется textShowCount; В самой строке спец-макросом может быть указано кол-во секунд
  textShowCount = 1;              // Сколько раз прокручивать текст бегущей строки поверх эффектов; По умолчанию - 1; В самой строке спец-макросом может быть указано число 
  useSpecialTextColor = false;    // В текущей бегущей строке был задан цвет, которым она должна отображаться
  specialTextColor = 0xffffff;    // Цвет отображения бегущей строки, может быть указан макросом в строке. Если не указан - используются глобальные настройки цвета бегущей строки
  specialTextEffect = -1;         // Эффект, который нужно включить при начале отображения строки текста, может быть указан макросом в строке.  
  specialTextEffectParam = -1;    // Параметр для эффекта (см. выше). Например эффект MC_SDCARD имеет более 40 подэффектов. Номер подэффекта хранится в этой переменной, извлекается из макроса {E}
  nextTextLineIdx = -1;           // Какую следующую строку показывать, может быть указан макросом в строке. Если указан - интервал отображения игнорируется, строка показывается сразу;
  textHasDateTime = false;        // Строка имеет макрос отображения текущего времени - ее нужно пересчитывать каждый раз при отрисовке; Если макроса времени нет - рассчитать текст строки один раз на этапе инициализации
  textHasMultiColor = false;      // Строк имеет множественное определение цвета - многоцветная строка
  currentText  = "";              // Текст текущей отображаемой строки

  #if (USE_MP3 == 1) 
  runTextSound = -1;              // Нет звука, сопровождающего строку
  runTextSoundRepeat = false;     // Нет повторения звука (однократное воспроизведение)
  #endif
  
  // Получить очередную строку из массива строк, обработав все макросы кроме дато-зависимых
  // Если макросы, зависимые от даты есть - установить флаг textHasDateTime? макросы оставить в строке
  // Результат положить в currentText
  // Далее если флаг наличия даты в строке установлен - каждый раз перед отрисовкой выполнять подстановку даты и
  // обработку наступления даты последнего показа и переинициализацию строки
  // Если зависимостей от даты нет - вычисленная строка просто отображается пока не будет затребована новая строка

  offset = pWIDTH;   // перемотка новой строки в правый край
  if (text.length() != 0) {
    syncText = text;
    currentText = processMacrosInText(text);
  } else {
    // Размер массива строк
    uint8_t sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  
    // Если nextIdx >= 0 - значит в предыдущей строке было указано какую строку показывать следующей - показываем ее
    currentTextLineIdx = nextIdx >= 0 ? nextIdx : getNextLine(currentTextLineIdx);
    if (currentTextLineIdx >= sizeOfTextsArray) currentTextLineIdx = -1;
  
    currentText = currentTextLineIdx < 0 ? "" : textLines[currentTextLineIdx];
    syncText = currentText;
    // Если выбрана строка для принудительного показа - игнорировать запрет по '-' в начале строки или по макросу {-}
    if (nextIdx >= 0) {
      if (currentText[0] == '-') currentText[0] = ' ';
      currentText.replace("{-}", "");
    }
    currentText = processMacrosInText(currentText);
  }

  if (text.length() == 0 && currentTextLineIdx == 0 && currentText[0] == '#') currentText = "";

  // После обработки макросов в строке может оказаться, что строка не должна отображаться (например, по времени показа макроса {S}),
  // но в строке присутствует также макрос {#N}, который был разобран и результат сохранен в nextTextLineIdx.
  // Если основную строку показывать не нужно (возвращена пустая строка), то и "прицеп" нужно "обнулить".
  if (currentText.length() == 0) {
    nextTextLineIdx = -1;
  }
  
  return currentText.length() > 0;
}

// Получить индекс строки для отображения
// -1 - если показывать нечего
int8_t getNextLine(int8_t currentIdx) {
  // Если не задана следующая строка - брать следующую из массива в соответствии с правилом
  // sequenceIdx < 0 - просто брать следующую строку
  // sequenceIdx > 0 - строка textLines[0] содержит последовательность отображения строк, например "#12345ZYX"
  //                   в этом случае sequenceIdx = 1..textLines[0].length() и показывает на символ в строке, содержащий индекс следующей строки к отображению
  //                   Если извлеченный символ - '#' - брать случайную строку из массива
  int8_t nextLineIdx = currentIdx;
  if (sequenceIdx < 1) {
    nextLineIdx++;
  } else {
    if (sequenceIdx >= (int16_t)textLines[0].length()) {
      sequenceIdx = 1;  // перемотать на начало последовательности
    }
    char c = textLines[0].charAt(sequenceIdx);
    if (c == '#') {
      // textLines[0] == "##", sequenceIdx всегда 1; textLines[0].charAt(1) == '#';
      // Это значит надо выбрать случайную строку из тех, что заполнены
      uint8_t size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
      uint8_t arr[size + 1], cnt = 0;
      memset(arr, '\0', size + 1);
      // Перебрать весь массив строк, выбрать только заполненные, у заполненных проверить, что они не отключены, не содержат макроса {P},
      // а если содержат макрос {S}, то текущая дата попадает в диапазон разрешенных дат.
      for (uint8_t i = 0; i < size; i++) {
        String text = textLines[i];
        // Строка пустая? 
        // Отключена - в первом символе или наличие макроса {-}?
        // Нулевая строка - строка управления ('#' или '##')?
        // Строки с макросом события {P} не отображаются по интервалу показа - только сразу до/после события
        // Если строка содержит макрос {S} - текущая дата должна попадать в диапазон доступных для показа дат
        bool disabled = (text.length() == 0) || (i == 0 && text[0] == '#') || (text[0] == '-') || (text.indexOf("{-}") >= 0) || (text.indexOf("{P") >= 0);
        bool wrong_date = (text.indexOf("{S") >= 0) && !forThisDate(text);
        if (disabled || wrong_date) continue;
        arr[cnt++] = i;
      }

      // Массив arr содержит индексы строк, которые доступны к отображению.
      // Если массив пустой - cnt == 0 - нет строки для отображения, ничего не показывать
      if (cnt == 0) {
        // Нет строки для отображения
        nextLineIdx = -1;
        textLastTime = millis();
      } else {
        uint8_t att = 0;
        uint8_t idx = random8(0,cnt - 1);
        // Выбрать случайную строку. Если выбранная совпадает с текущей - выбрать другую.
        // Если другой нет (в массиве одна строка) - показать её
        while (arr[idx] == nextLineIdx && att < cnt) {
          att++; idx++;
          if (idx >= cnt) idx = 0;
        }        
        nextLineIdx = arr[idx];
      }
    } else {
      // Последовательное отображение строк как указано в последовательности в textLines[0] - '#12345'
      // здесь 'c' - char - индекс, выдернутый из указанной последовательности в очередной позиции
      nextLineIdx = getTextIndex(c); 
      uint8_t c_idx = sequenceIdx;
      bool found = false;
      while (!found) {
        // Проверить - доступен ли текст в указанной строке к отображению?
        String text = textLines[nextLineIdx];
        // Строка должна быть не пустая,
        // Не отключена - в первом символе или наличие макроса {-}?
        // Строки с макросом события {P} не отображаются по интервалу показа - только сразу до/после события
        // Если строка содержит макрос {S} - текущая дата должна попадать в диапазон доступных для показа дат
        bool disabled = (text.length() == 0) || (nextLineIdx == 0 && text[0] == '#') || (text[0] == '-') || (text.indexOf("{-}") >= 0) || (text.indexOf("{P") >= 0);
        bool wrong_date = (text.indexOf("{S") >= 0) && !forThisDate(text);
        // Если строка не отключена и доступна к отображению - брать ее
        if (!(disabled || wrong_date)) {
          found = true;
          break;
        }
        // Строка недоступна - брать следующий номер в последовательности
        sequenceIdx++;
        if (sequenceIdx >= (int16_t)textLines[0].length()) {
          sequenceIdx = 1;  // перемотать на начало последовательности
        }
        if (c_idx == sequenceIdx) break;
        // Если после перемотки вернулись в позицию с которой начали - строк доступных к показу нет
        c = textLines[0].charAt(sequenceIdx);
        nextLineIdx = getTextIndex(c); 
      }
      if (found) {
        sequenceIdx++;
      } else {
        nextLineIdx = -1;
        textLastTime = millis();
      }
    }
  }
  
  return nextLineIdx;
}

// Выполнить разбор строки на наличие макросов, применить указанные настройки
String processMacrosInText(const String text) {  

  String textLine = text;
  /*   
     Общие правила:
     - Макрос - последовательность управляющих конструкций, каждая из которых заключена в фигурные скобки, например "{Exx}" 
     - Если строка textLines[0] начинается с '#'  - строка содержит последовательность строк к отображению, например "#1234ZYX"
     - Если строка textLines[0] начинается с '##' - отображать строки из массива в случайном порядке
     - Если строка textLines[0] НЕ начинается с '#' или '##' - это обычная строка для отображения
     - Если строка начинается с '-' или пустая или содержит макрос '{-}' - строка отключена и не отображается 
     
     Строка, извлеченная из массива может содержать в себе следующие макросы:
      "{-}"     - в любом месте строки означает, что строка временно отключена, аналогично "-" в начале строки
      "{#N}"    - в любом месте строки означает,что после отображения строки, следующей строкой отображать строку с номером N, где N - 1..9,A..Z -> {#9}
                  Настройка из п.0 (через какое время показывать следующую строку) игнорируется, строка показывается сразу
                  после завершения отображения "родительской" строки
      "{An+}    - проиграть звук номер n из папки SD:/03; если есть '+' - проигрывать в цикле пока показывается строка, если нет суффикса '+' - проиграть один раз -> {A3+}
      "{Exx}"   - эту строку отображать на фоне эффекта xx - где xx - номер эффекта. Эффект не меняется пока отображается строка -> {E21}
      "{Exx-n}" - эту строку отображать на фоне эффекта xx, вариант n - где xx - номер эффекта, n - номер варианта -> {E24-8}
      "{Ts}"    - отображать строку указанное количество секунд s -> {T30}
      "{Nx}"    - отображать строку указанное количество раз x -> {N3} 
                  если S или N не указаны - строка прокручивается 1 раз;
                  Если указаны оба - работает модификатор показа по времени
      "{Cc}"    - отображать строку указанным цветом С; Цвет - в виде #AA77FE; Специальные значения - #000001 - радуга;  - #000002 - каждая буква свой цвет; -> {C#005500}
      "{Bc}"    - отображать строку на однотонном фоне указанного цвета С; Цвет - в виде #337700; -> {B#000000}
      "{WS}"    - отображать вместо {WS} состояние текущей погоды - "Пасмурно", "Ясно", "Дождь", "Гроза" и т.д. -> {WS}
      "{WT}"    - отображать вместо {WT} текущую температуру воздуха, например "+26", "-31" -> {WT}
      "{D:F}"   - где F - один из форматов даты / времени                         "С Новым {D:yyyy} годом!"
                  d    - день месяца, в диапазоне от 1 до 31.  (допускается D)
                  dd   - день месяца, в диапазоне от 01 до 31. (допускается DD)
                  ddd  - сокращенное название дня недели       (допускается DDD)
                  dddd - полное название дня недели            (допускается DDDD)
                  M    - месяц от 1 до 12
                  MM   - месяц от 01 до 12
                  MMM  - месяц прописью (янв..дек)
                  MMMМ - месяц прописью (января..декабря)
                  y    - год в диапазоне от 0 до 99            (допускается Y)
                  yy   - год в диапазоне от 00 до 99           (допускается YY)
                  yyyy - год в виде четырехзначного числа      (допускается YYYY)
                  h    - час в 12-часовом формате от 1 до 12
                  hh   - час в 12-часовом формате от 01 до 12
                  H    - час в 24-часовом формате от 0 до 23
                  HH   - час в 24-часовом формате от 00 до 23
                  m    - минуты в диапазоне от 0 до 59
                  mm   - минуты в диапазоне от 00 до 59
                  s    - секунды в диапазоне от 0 до 59        (допускается S)
                  ss   - секунды в диапазоне от 00 до 59       (допускается SS)
                  T    - Первый символ указателя AM/PM
                  TT   - Указатель AM/PM
                  t    - Первый символ указателя am/pm
                  tt   - Указатель am/pm
        
        Если формат не указан - используется формат H:mm        
        пример: "Красноярское время {D:HH:mm}" - бегущая строка "Красноярское время 07:15"  
                "Сегодня {D:DD MMMM YYYY} года" - бегущая строка "Сегодня 26 июля 2020 года"

     "{D}"      - просто часы вида "21:00" в виде бегущей строки

     "{R01.01.2021 0:00:00#N}" 
     "{R01.01.**** 0:00:00#N}" 
     "{R01.01.***+ 0:00:00#N}" 
       - где после R указана дата и опционально время до которой нужно отсчитывать оставшееся время, выводя строку остатка в формате:
           для R: X дней X часов X минут; 
           для P: X дней X часов X минут X секунд; 
         Если время не указано - считается 0:00
         Если дней не осталось - выводится только X часов X минут; 
         Если часов тоже уже не осталось - выводится X минут
         Если минут тоже уже не осталось - выводится X секунд
         Год в дате может быть указан '****', что означает "текущий год" или '***+', что означает "Следующий год"
         Время в макросе может быть указано с секундами: '23:59:59'
       
       - где после даты/времени может быть указан
         - #N - если осталось указанная дата уже наступила - вместо этой строки выводится строка с номером N
                если строка замены не указана - просроченная строка не выводится берется следующая строка
         - Внимание! При использовании макроса для вывода строки "До Нового года осталось" с годом, указанным звездочками '****'
           следует иметь в виду, что по достижении указанной даты год сменится на следующий, и событие "По достижении даты показовать строку #N"
           не сработает - вместо этого будет отображаться "До Нового года осталось 365 дней".
           Чтобы это избежать - и строку с макросом {R} и строку #N нужно использовать совместно с макросом {S}, который онграничивает
           перирод показа этой строки:
             textLines[2]   = "До {C#00D0FF}Нового года {C#FFFFFF}осталось {C#10FF00}{R01.01.***+}{S01.12.****#31.12.**** 23:59:59}";
             textLines[3]   = "С {C#00D0FF}Новым {C#0BFF00}{D:yyyy} {C#FFFFFF}годом!{S01.01.****#31.01.**** 23:59:59}";


     "{P01.01.2020#N#B#A}"
     "{P**.**.**** 7:00#N#B#A#F}"
     "{P01.01.2020#N#B#A}"
     "{P7:00#N#B#A#F}"   
       - где после P указана дата и опционально время до которой нужно отсчитывать оставшееся время, выводя строку остатка в формате:
          X дней X часов X минут X секунд; 
         Если дней не осталось - выводится только X часов X минут; 
         Если часов тоже уже не осталось - выводится X минут
         Если минут тоже уже не осталось - выводится X секунд

       Строка с режимом {P не выводится при периодическом переборе строк для отображения - только до/после непосредственно времени события:
       время наступления события мониторится и за #B секунд до события бегущая строка начинает отображаться на матрице и после события #A секунд отображается строка-заместитель 
       
       
       Для режима P дата может опускаться (означает "каждый день") или ее компоненты могут быть заменены звездочкой '*'
       **.10.2020 - весь октябрь 2020 года 
       01.**.**** - каждое первое число месяца
       **.**.2020 - каждый день 2020 года
       
       "{P7:00#N#120#30#12347}"  - каждый пн,вт,ср,чт,вс в 7 утра
       
       - где после даты может быть указан
         - #N - если осталось указанная дата уже наступила - вместо этой строки выводится строка с номером N
                если строка замены не указана - просроченная строка не выводится берется следующая строка
         - #B - начинать отображение за указанное количество секунд ДО наступления события (before). Если не указано - 60 секунд по умолчанию
         - #A - отображать строку-заместитель указанное количество секунд ПОСЛЕ наступления события (after). Если не указано - 60 секунд по умолчанию
         - #F - дни недели для которых работает эта строка - полезно когда дата не определена 1-пн..7-вс
                Если указана точная дата и указан день недели, который не соответствует дате - строка выведена не будет

     "{S01.01.2020#01.01.2020}"
     "{S01.01.2020 7:00#01.01.2020 19:00}"
     "{S01.01.**** 7:00#01.01.**** 19:00}"
       - где после S указаны даты начала и конца периода доступного для отображения строки.
       Для режима S элементы даты быть заменены звездочкой '*'
       **.10.2020 - весь октябрь 2020 года 
       01.**.**** - каждое первое число месяца
       **.**.2020 - каждый день 2020 года  
  */

  // Выполнять цикл поиска подходящей к отображению строки
  // Если ни одной строки не найдено - возвратить false

  bool    found = false;
  uint8_t attempt = 0;
  int16_t idx, idx2;
  String  tmp, tmp1;

  // Размер массива строк
  uint8_t sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк

  while (!found && (attempt < sizeOfTextsArray)) {
    
    // -------------------------------------------------------------    
    // Строка начинается с '-' или пустая или содержит "{-}" - эта строка отключена, брать следующую
    // Строку показывать даже если отключена когда установлен флаг ignoreTextOverlaySettingforEffect
    // -------------------------------------------------------------    

    if (textLine.length() > 0 && ignoreTextOverlaySettingforEffect) {
      if (textLine[0] == '-') textLine[0] = ' ';
      textLine.replace("{-}", "");
    }

    if (textLine.length() == 0 || textLine.charAt(0) == '-' || textLine.indexOf("{-}") >= 0) {
      attempt++;  
      currentTextLineIdx = getNextLine(currentTextLineIdx);
      textLine = (currentTextLineIdx < 0 || currentTextLineIdx >= sizeOfTextsArray) ? "" : textLines[currentTextLineIdx];
      continue;
    }

    // Если в строке содержится макрос, связанный с погодой, но погода еще не получена с сервера - пропускать строку, брать следующую
    if (!init_weather && (textLine.indexOf("{WS}") >= 0 || textLine.indexOf("{WT}") >= 0)) {
      attempt++;  
      currentTextLineIdx = getNextLine(currentTextLineIdx);
      textLine = (currentTextLineIdx < 0 || currentTextLineIdx >= sizeOfTextsArray) ? "" : textLines[currentTextLineIdx];
      continue;
    }

    // -------------------------------------------------------------
    // Эти форматы содержат строку, зависящую от текущего времени.
    // Оставить эти форматы как есть в строке - они будут обрабатываться на каждом проходе, подставляя текущее время
    // Сейчас просто выставить флаг, что строка содержит макросы, зависимые от даты
    //    "{D:F}" - где F - один из форматов даты / времени если формата даты нет - аналогично {D}
    //    "{D}"  - просто часы вида "21:00" в виде бегущей строки
    //    "{R01.01.****#N}" 
    //    "{P01.**.2021 8:00#N#B#A#F}" 
    //    "{S01.01.****}" 
    // -------------------------------------------------------------

    textHasDateTime = textLine.indexOf("{D") >= 0 || textLine.indexOf("{R") >= 0 || textLine.indexOf("{P") >= 0 || textLine.indexOf("{S") >= 0;  

    // Если время еще не инициализировано и строка содержит макросы, зависимые от времени -
    // строку отображать нельзя - пропускаем, переходим к поиску следующей строки
    if (!init_time && textHasDateTime) {
      textHasDateTime = false;
      attempt++;  
      continue;
    }

    found = true;

    // -------------------------------------------------------------    
    // "{#N}" - в любом месте строки означает,что после отображения строки, следующей строкой отображать строку с номером N, где N - 1..9,A..Z или двухзначный индекс 1..19
    // -------------------------------------------------------------    
    
    nextTextLineIdx = -1;
    idx = textLine.indexOf("{#");
    while (idx >= 0) {

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь номер эффекта, он должен быть 0..MAX_EFFECT-1, может быть двузначным
      tmp = "";
      if (idx2 - idx > 1) {
        tmp = textLine.substring(idx+2, idx2);
      }
      
      // удаляем макрос
      textLine.remove(idx, idx2 - idx + 1);

      int16_t len = tmp.length();
      if (len == 0 || len > 2) break;

      char c = tmp.charAt(0);
      if (len == 1 || (c >= 'A' && c <= 'Z')) {        
        // Запоминаем номер следующей к показу строки, извлеченный из макроса
          nextTextLineIdx = getTextIndex(c);
      } else {
         nextTextLineIdx = tmp.toInt();
      }
      
      if (nextTextLineIdx >= sizeOfTextsArray) {
        nextTextLineIdx = -1;
      }
      
      // удаляем макрос
      textLine.remove(idx, idx2 - idx + 1);
      
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{#");  
    }

    // -------------------------------------------------------------
    //  "{An+} - при начале строки проигрывать звук n из папки "SD://03"? если есть суффикс '+' - воспроизводить в цикле пока показывается строка
    // -------------------------------------------------------------
    #if (USE_MP3 == 1)

    runTextSound = -1;
    runTextSoundRepeat = false;
    
    idx = textLine.indexOf("{A");
    while (idx >= 0) {
      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь номер звука, может быть двузначным
      tmp = "";
      if (idx2 - idx > 1) {
        tmp = textLine.substring(idx+2, idx2);
      }
      
      // удаляем макрос
      textLine.remove(idx, idx2 - idx + 1);

      // Последним символом в выделенной строке может быть '+' - признак повторения звука (цикл)
      if (tmp.endsWith("+")) {
        runTextSoundRepeat = true;
        tmp = tmp.substring(0, tmp.length()-1);
      }

      // Преобразовать строку в число
      idx = tmp.length() > 0 ? tmp.toInt() : -1;
      runTextSound = idx;

      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{A");  
    }
    #endif

    // -------------------------------------------------------------
    //  "{Exx} - эту строку отображать на фоне эффекта xx - где xx - номер эффекта. Эффект не меняется пока отображается строка      
    //  "{Exx-n} - эту строку отображать на фоне эффекта xx с вариантом n - где xx - номер эффекта? n - номер варианта эффекта.
    // -------------------------------------------------------------

    specialTextEffect = -1;
    specialTextEffectParam = -1;
    idx = textLine.indexOf("{E");
    while (idx >= 0) {

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь номер эффекта, он должен быть 0..MAX_EFFECT-1, может быть двузначным
      tmp = "", tmp1 = "";
      if (idx2 - idx > 1) {
        tmp = textLine.substring(idx+2, idx2);
      }
      
      // удаляем макрос
      textLine.remove(idx, idx2 - idx + 1);

      // tmp может содержать кроме номера эффекта, номер варианта, отделенный знаком '-' (например из {E43} или {E34-12} будут строки '43' и '43-12'
      // если есть номер варианта - отделить его от номера эффекта
      uint8_t p = tmp.indexOf('-');
      if (p > 0) {
        tmp1 = tmp.substring(p + 1);
        tmp  = tmp.substring(0, p);
      }
      
      // Преобразовать строку в число
      idx = tmp.length() > 0 ? tmp.toInt() : -1;
      specialTextEffect = idx >= 0 && idx < MAX_EFFECT ? idx : -1;

      idx = tmp1.length() > 0 ? tmp1.toInt() : -1;
      specialTextEffectParam = idx >= 0 ? idx : -1;
            
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{E");  
    }

    // -------------------------------------------------------------
    //  "{TS}" - отображать строку указанное количество секунд S
    // -------------------------------------------------------------

    textShowTime = -1;
    idx = textLine.indexOf("{T");
    while (idx >= 0) {

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь количество секунд отображения этой бегущей строки
      tmp = "";
      if (idx2 - idx > 1) {
        tmp = textLine.substring(idx+2, idx2);
      }
      
      // удаляем макрос
      textLine.remove(idx, idx2 - idx + 1);

      // Преобразовать строку в число
      idx = tmp.length() > 0 ? tmp.toInt() : -1;
      textShowTime = idx > 0 ? idx : -1;
      
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{T");
    }

    // -------------------------------------------------------------
    //  "{NX}" - отображать строку указанное количество раз X
    // -------------------------------------------------------------

    textShowCount = 1;
    idx = textLine.indexOf("{N");
    while (idx >= 0) {

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь количество раз отображения этой бегущей строки
      tmp = "";
      if (idx2 - idx > 1) {
        tmp = textLine.substring(idx+2, idx2);
      }
      
      // удаляем макрос
      textLine.remove(idx, idx2 - idx + 1);

      // Преобразовать строку в число
      idx = tmp.length() > 0 ? tmp.toInt() : -1;
      textShowCount = idx > 0 ? idx : 1;
      
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{N");  
    }
      
    // -------------------------------------------------------------
    // "{BC}"- отображать строку  на однотонном фоне указанного цвета С; Цвет - в виде #007700
    // -------------------------------------------------------------

    useSpecialBackColor = false;
    idx = textLine.indexOf("{B");
    while (idx >= 0) {

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь цвет фона отображения этой бегущей строки
      tmp = "";
      if (idx2 - idx > 1) {
        tmp = textLine.substring(idx+2, idx2);
      }
      
      // удаляем макрос
      textLine.remove(idx, idx2 - idx + 1);
           
      // Преобразовать строку в число
      useSpecialBackColor = true;
      specialBackColor = (uint32_t)HEXtoInt(tmp);
      
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{B");  
    }

    #if (USE_WEATHER == 1)     
    
    // {WS} - отображать текущую погоду - "Ясно", "Облачно" и т.д
    idx = textLine.indexOf("{WS}");
    if (idx >= 0) {
      textLine.replace("{WS}", weather);
    }

    // {WT} - отображать текущую температуру в виде "+26" или "-26"
    // Если включено отображение температуры цветом - добавить макрос цвета перед температурой 
    idx = textLine.indexOf("{WT}");
    if (idx >= 0) {
      // Подготовить строку текущего времени HH:mm и заменить все вхождения {D} на эту строку
      String s_temperature = (temperature == 0 ? "" : (temperature > 0 ? "+" : "")) + String(temperature);
      String s_color = "";

      if (useTemperatureColor) {
        s_color = "{C" + getTemperatureColor(temperature) + "}";
      }
      
      textLine.replace("{WT}", s_color + s_temperature);
    }

    #endif
     
    // -------------------------------------------------------------
    // "{CC}"- отображать строку указанным цветом С; Цвет - в виде #AA77FE
    // -------------------------------------------------------------
    // Если макрос стоит в самом начале или в самом конце строки и единственный - всю строку отображать указанным цветом
    // Если макрос стоит в середине строки или макросов цвета несколько - они указывают позицию в строке с которой меняется цвет отображения
    // В этом случае два варианта - строка также содержит макрос, зависящий от времени?
    // - не содержит дат - разбор можно сделать один раз, сохранить позиции и цвета в массив и дальше использовать его при отображении
    // - содержит даты - разбор позиций цветов определять ПОСЛЕ формирования строки с вычисленной датой
    
    textHasMultiColor = checkIsTextMultiColor(textLine);

    // Если строка не содержит даты или не содержит множественного определения цвета
    // обработать макрос цвета, цвет сохранить в specialTextColor, установить флаг useSpecialTextColor
    // В дальнейшем при отображении строки не нужно каждый раз вычислять цвет и позицию - просто использовать specialTextColor

    // Если строка содержит множественное определение цвета и не содержит даты - подготовить массив цветов,
    // который будет отображаться для отрисовки строки
    
    if (textHasMultiColor && !textHasDateTime) {
      textLine = processColorMacros(textLine);
    } else 
    
    if (!textHasMultiColor) { 
      
      useSpecialTextColor = false;
      idx = textLine.indexOf("{C");
      while (idx >= 0) {
  
        // Закрывающая скобка
        // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
        idx2 = textLine.indexOf("}", idx);        
        if (idx2 < 0) break;
  
        // Извлечь цвет текста отображения этой бегущей строки
        tmp = "";
        if (idx2 - idx > 1) {
          tmp = textLine.substring(idx+2, idx2);
        }
        
        // удаляем макрос
        textLine.remove(idx, idx2 - idx + 1);
             
        // Преобразовать строку в число
        useSpecialTextColor = true;
        specialTextColor = (uint32_t)HEXtoInt(tmp);
        
        // Есть еще вхождения макроса?
        idx = textLine.indexOf("{C");  
      }

    }
    attempt++;
  }

  return found ? textLine : "";
}

// Обработать макросы даты в строке
// textLine - строка, содержащая макросы
String processDateMacrosInText(const String text) {

  String textLine = text;
  
  /* -------------------------------------------------------------
   Эти форматы содержат строку, зависящую от текущего времени.
   Оставить эти форматы как есть в строке - они будут обрабатываться на каждом проходе, подставляя текущее время
   Сейчас просто выставить флаг, что строка содержит макросы, зависимые от даты
      "{D}"  - просто часы вида "21:00" в виде бегущей строки
      "{D:F}" - где F - один из форматов даты / времени если формата даты нет - аналогично {D}
       Формат даты поддерживает следующие спецификаторы       
          d    - день месяца, в диапазоне от 1 до 31.  (допускается D)
          dd   - день месяца, в диапазоне от 01 до 31. (допускается DD)
          ddd  - сокращенное название дня недели       (допускается DDD)
          dddd - полное название дня недели            (допускается DDDD)
          M    - месяц от 1 до 12
          MM   - месяц от 01 до 12
          MMM  - месяц прописью (янв..дек)
          MMMМ - месяц прописью (января..декабря)
          y    - год в диапазоне от 0 до 99            (допускается Y)
          yy   - год в диапазоне от 00 до 99           (допускается YY)
          yyyy - год в виде четырехзначного числа      (допускается YYYY)
          h    - час в 12-часовом формате от 1 до 12
          hh   - час в 12-часовом формате от 01 до 12
          H    - час в 23-часовом формате от 0 до 23
          HH   - час в 23-часовом формате от 00 до 23
          m    - минуты в диапазоне от 0 до 59
          mm   - минуты в диапазоне от 00 до 59
          s    - секунды в диапазоне от 0 до 59        (допускается S)
          ss   - секунды в диапазоне от 00 до 59       (допускается SS)
          T    - Первый символ указателя AM/PM
          TT   - Указатель AM/PM
          t    - Первый символ указателя am/pm
          tt   - Указатель am/pm
  
      "{R01.01.2021#N}" 
      "{R01.01.****}" 
      "{R01.01.***+}" 
         - где после R указана дата до которой нужно отсчитывать оставшееся время, выводя строку остатка в формате:
             X дней X часов X минут; 
           Если дней не осталось - выводится только X часов X минут; 
           Если минут тоже уже не осталось - выводится X минут
           Год в дате может быть указан '****', что означает "текущий год" или '***+', что означает "Следующий год"
           Время в макросе может быть указано с секундами: '23:59:59'
       
         - где после даты может быть указан
           - #N - если осталось указанная дата уже наступила - вместо этой строки выводится строка с номером N
                  если строка замены не указана или отключена (символ "-" вначале строки) - просроченная строка не выводится
                  берется следующая строка
                  
         - Внимание! При использовании макроса для вывода строки "До Нового года осталось" с годом, указанным звездочками '****'
           следует иметь в виду, что по достижении указанной даты год сменится на следующий, и событие "По достижении даты показовать строку #N"
           не сработает - вместо этого будет отображаться "До Нового года осталось 365 дней".
           Чтобы это избежать - и строку с макросом {R} и строку #N нужно использовать совместно с макросом {S}, который онграничивает
           перирод показа этой строки:
             textLines[2]   = "До {C#00D0FF}Нового года {C#FFFFFF}осталось {C#10FF00}{R01.01.***+}{S01.12.****#31.12.**** 23:59}";
             textLines[3]   = "С {C#00D0FF}Новым {C#0BFF00}{D:yyyy} {C#FFFFFF}годом!{S01.01.****#31.01.**** 23:59}";

     "{PДД.ММ.ГГГГ#N#B#A#F}"
       - где после P указаны опционально дата и время до которой нужно отсчитывать оставшееся время, выводя строку остатка в формате:
         X дней X часов X минут X секунд; 
         Если дней не осталось - выводится только X часов X минут; 
         Если часов тоже уже не осталось - выводится X минут
         Если минут тоже уже не осталось - выводится X секунд

       Строка с режимом {P не выводится при периодическом переборе строк для отображения - только до/после непосредственно времени события:
       время наступления события мониторится и за #B секунд до события бегущая строка начинает отображаться на матрице и после события #A секунд отображается строка-заместитель 
       
       Для режима P дата может опускаться (означает "каждый день") или ее компоненты могут быть заменены звездочкой '*'
       **.10.2020 - весь октябрь 2020 года 
       01.**.**** - каждое первое число месяца любого года
       **.**.2020 - каждый день 2020 года
       
       "{P7:00#N#120#30#12347}"  - каждый пн,вт,ср,чт,вс в 7 утра (за 120 сек до наступления события и 30 секунд после наступления

       - где компоненты даты:
         ДД - число месяца
         MM - месяц
         ГГГГ - год
         Компоненты даты могут быть заменены * - означает "любой"
         
       - где после даты может быть указан
         - #N - если осталось указанная дата уже наступила - вместо этой строки выводится строка с номером N
                если строка замены не указана - просроченная строка не выводится берется следующая строка
         - #B - начинать отображение за указанное количество секунд ДО наступления события (before). Если не указано - 60 секунд по умолчанию
         - #A - отображать строку-заместитель указанное количество секунд ПОСЛЕ наступления события (after). Если не указано - 60 секунд по умолчанию
         - #F - дни недели для которых работает эта строка, когда дата не определена 1-пн..7-вс
                Если указана точная дата и указан день недели, который не соответствует дате - строка выведена не будет
          
     "{S01.01.2020#01.01.2020}"
     "{S01.01.2020 7:00#01.01.2020 19:00}"
     "{S01.01.**** 7:00#01.01.**** 19:00}"
       - где после S указаны даты начала и конца периода доступного для отображения строки.
       Для режима S элементы даты быть заменены звездочкой '*'
       **.10.2020 - весь октябрь 2020 года 
       01.**.**** - каждое первое число месяца
       **.**.2020 - каждый день 2020 года  
     ------------------------------------------------------------- 
  */

  String   str, tmp, s_time, outText;
  uint8_t  aday = day();
  uint8_t  amnth = month();
  uint16_t ayear = year();
  uint8_t  hrs = hour(), hrs_t;
  uint8_t  mins = minute();
  uint8_t  secs = second();
  bool     am = isAM();
  bool     pm = isPM();
  int8_t   idx, idx2;

  int8_t   wd = weekday()-1;  // day of the week, Sunday is day 0   
  if (wd == 0) wd = 7;        // Sunday is day 7, Monday is day 1;

  while (true) {
    // {D} - отображать текущее время в формате 'HH:mm'
    idx = textLine.indexOf("{D}");
    if (idx >= 0) {
      // Подготовить строку текущего времени HH:mm и заменить все вхождения {D} на эту строку
      String s_hour = String(hrs);
      String s_mins = String(mins);
      if (s_hour.length() < 2) s_hour = "0" + s_hour;
      if (s_mins.length() < 2) s_mins = "0" + s_mins;
      String s_time = s_hour + ":" + s_mins;
      textLine.replace("{D}", s_time);
    }

    // {D:F} (где F - форматная строка
    idx = textLine.indexOf("{D:");
    if (idx >= 0) {
  
      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;
  
      // Извлечь форматную строку
      String sFormat = "$", sFmtProcess = "#";
  
      if (idx2 - idx > 1) {
        sFormat = textLine.substring(idx+3, idx2);
      }
      sFmtProcess = sFormat;
      
      //  YYYY - год в виде четырехзначного числа
      str = String(ayear);
      sFmtProcess.replace("YYYY", str);
      sFmtProcess.replace("yyyy", str);
  
      //  YY   - год в диапазоне от 00 до 99
      str = str.substring(2);
      sFmtProcess.replace("YY", str);
      sFmtProcess.replace("yy", str);
  
      //  Y    - год в диапазоне от 0 до 99
      if (str[0] == '0') str = str.substring(1);
      sFmtProcess.replace("Y", str);
      sFmtProcess.replace("y", str);
      
      //  HH   - час в 23-часовом формате от 00 до 23
      str = String(hrs);
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("HH", str);
  
      //  H    - час в 23-часовом формате от 0 до 23
      str = String(hrs);
      sFmtProcess.replace("H", str);
  
      //  hh   - час в 12-часовом формате от 01 до 12
      hrs_t = hrs;
      if (hrs_t > 12) hrs_t = hrs_t - 12;
      if (hrs_t == 0) hrs_t = 12;
      str = String(hrs_t);
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("hh", str);
  
      //  h    - час в 12-часовом формате от 1 до 12
      str = String(hrs_t);
      sFmtProcess.replace("h", str);
      
      //  mm   - минуты в диапазоне от 00 до 59
      str = String(mins);
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("mm", str);
  
      //  m    - минуты в диапазоне от 0 до 59
      str = String(mins);
      sFmtProcess.replace("m", str);
  
      //  ss   - секунды в диапазоне от 00 до 59
      str = String(secs);
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("SS", str);
      sFmtProcess.replace("ss", str);
  
      //  s    - секунды в диапазоне от 0 до 59
      str = String(secs);
      sFmtProcess.replace("S", str);
      sFmtProcess.replace("s", str);
  
      //  tt   - Указатель AM/PM
      str = am ? "AM" : (pm ? "PM" : "");
      sFmtProcess.replace("TT", str);
      str.toLowerCase();
      sFmtProcess.replace("tt", str);
  
      //  t    - Первый символ указателя AM/PM
      str = am ? "A" : (pm ? "P" : "");
      sFmtProcess.replace("T", str);
      str.toLowerCase();
      sFmtProcess.replace("t", str);

      //  dddd - полное название дня недели            (допускается DDDD)
      str = getWeekdayString(wd);  // DDDD  
      str = substitureDateMacros(str);
      sFmtProcess.replace("DDDD", str);
      sFmtProcess.replace("dddd", str);
      
      //  ddd  - сокращенное название дня недели       (допускается DDD)
      str = getWeekdayShortString(wd);  // DDD
      str = substitureDateMacros(str);
      sFmtProcess.replace("DDD", str);
      sFmtProcess.replace("ddd", str);
  
      //  dd   - день месяца, в диапазоне от 01 до 31. (допускается DD)
      str = String(aday);  // DD
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("DD", str);
      sFmtProcess.replace("dd", str);
  
      //  d    - день месяца, в диапазоне от 1 до 31.  (допускается D)
      str = String(aday);  // D
      sFmtProcess.replace("D", str);
      sFmtProcess.replace("d", str);

      //  MMMМ - месяц прописью (января..декабря)
      str = getMonthString(amnth);
      str = substitureDateMacros(str);
      sFmtProcess.replace("MMMM", str);
      
      //  MMM  - месяц прописью (янв..дек)
      str = substitureDateMacros(str);
      str = getMonthShortString(amnth);
      sFmtProcess.replace("MMM", str);
  
      //  MM   - месяц от 01 до 12
      str = String(amnth);
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("MM", str);
  
      //  M    - месяц от 1 до 12
      str = String(amnth);
      sFmtProcess.replace("M", str);
      sFmtProcess = unsubstitureDateMacros(sFmtProcess);

      // Заменяем в строке макрос с исходной форматной строкой на обработанную строку с готовой текущей датой
      textLine.replace("{D:" + sFormat + "}", sFmtProcess);
      
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{D:");
    }

    // "{R01.01.****}" 
    // "{R01.01.***+}" 
    // "{R01.01.2023#N}" 
    // "{R10.10.2023 7:00#N}" 
    idx = textLine.indexOf("{R");
    if (idx >= 0) {
            
      // Если время  события уже наступило и в строке указана строка-заместитель для отображения ПОСЛЕ наступления события - показывать эту строку
      // Если замены строки после наступления события нет - textLine будет пустой - отображать нечего
      // Строка замены снова может содержать метки времени - поэтому отправить проверку / замену на второй круг

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь дату события, время и строку замены ПОСЛЕ события (если есть)
      str = "";
      if (idx2 - idx > 1) {
        str = textLine.substring(idx+2, idx2);
        str.trim();
      }

      // удаляем макрос; точка вставки строки остатка будет будет в позицию idx
      textLine.remove(idx, idx2 - idx + 1);

      // Здесь str имеет вид '01.01.2020', или '01.01.****', или '01.01.***+' если строки замены ПОСЛЕ события нет или '01.01.2020#J' или '01.01.2020#19' если строка замены указана (J=19 - индекс в массиве)
      // так же после даты может быть указано время '01.01.2020 7:00:00'
      if (str.length() > 0) {

        time_t t_now = now();
        time_t t_event = now();
        time_t t_diff = 0;

        // Точка вставки строки остатка
        uint16_t insertPoint = idx;
        int8_t  afterEventIdx = -1;

        // Здесь - str - дата (и время) события, s_nn - номер строки замены или пустая строка, если строки замены нет
        // Получить дату наступления события из строки 'ДД.ММ.ГГГГ'

        // Есть индекс строки замены? Если есть - отделить ее от даты события
        idx = str.indexOf("#");
        String s_nn = "";
        if (idx >= 0) {
          s_nn = idx>=0 ? str.substring(idx+1) : "";
          str = str.substring(0,idx);
        }

        tmElements_t tm = ParseDateTime(str);
        t_event = makeTime(tm);

        /*
        DEBUGLN("------------------------------------");
        DEBUGLN("Исходная R-дата: '" + str + "'");
        DEBUGLN(String(F("Дата события: ")) + padNum(tm.Day,2) + "." + padNum(tm.Month,2) + "." + padNum(tmYearToCalendar(tm.Year),4) + " " + padNum(tm.Hour,2) + ":" + padNum(tm.Minute,2) + ":" + padNum(tm.Second,2));
        DEBUGLN("------------------------------------");
        */
        
        // Если t_now >= t_event - событие уже прошло, нужно заменять обрабатываемую строку на строку подстановки, указанную (или нет) в s_nn 
        if (t_now >= t_event) {

          uint8_t len = s_nn.length();
          if (len > 0) {
            // Преобразовать строку в индекс
            char c = s_nn.charAt(0);
            if (len == 1 || (c >= 'A' && c <= 'Z')) {        
              // Номер следующей к показу строки, извлеченный из макроса в формате 1..9,A..Z
              afterEventIdx = getTextIndex(c);
            } else {
              // Номер следующей к показу строки, извлеченный из макроса в формате десятичного числа
              afterEventIdx = s_nn.toInt();
            }
      
            uint8_t sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
            if (afterEventIdx >= sizeOfTextsArray) {
              afterEventIdx = -1;
            }
          } else {
            // s_nn не содержит индекса строки замены
            afterEventIdx = -1; 
          }

          // Строка замены не указана - завершить показ строки
          if (afterEventIdx < 0) {
            return "";    
          }

          // Получить текст строки замены. Он может содержать макросы, в т.ч. и макросы даты. Их нужно обработать.
          // Для того, чтобы указанная строка замены не отображалась при обычном переборе - она должна быть помечена как отключенная - '-' в начале строки или '{-}' в любом месте
          // Когда же пришло время отобразить строку - перед обработкой макросов ее нужно "включить"
          textLine = textLines[afterEventIdx];

          // содержит ли строка замены ограничение на показ в периоде, указанном макросом {S}?
          bool wrong_date = (textLine.indexOf("{S") >= 0) && !forThisDate(textLine);
          if (wrong_date) {
            return "";    
          }

          if (textLine.length() == 0) return "";              
          if (textLine[0] == '-') textLine = textLine.substring(1);
          if (textLine.indexOf("{-}") >= 0) textLine.replace("{-}", "");
          if (textLine.length() == 0) return "";    
          
          textLine = processMacrosInText(textLine);
          
          // Строка замены содержит в себе макросы даты? Если да - вычислить всё снова для новой строки                   
          if (textLine.indexOf("{D}") >= 0 || textLine.indexOf("{D:") >= 0 ||textLine.indexOf("{R") >= 0 || textLine.indexOf("{P") >= 0 || textLine.indexOf("{S") >= 0) continue;

          // Вернуть текст строки замены, отображаемой после того, как событие прошло
          return textLine;
        }

        // Событие еще не наступило
        
        // Чтобы строка замены наступившего события не показывалась в обычном порядке - она должна быть отключена
        // Проверить, что это так и если не отключена - отключить
        uint8_t len = s_nn.length();        
        if (len > 0) {
          // Преобразовать строку в индекс
          char c = s_nn.charAt(0);
          int8_t nm = -1;
          if (len == 1 || (c >= 'A' && c <= 'Z')) {        
            // Номер следующей к показу строки, извлеченный из макроса в формате 1..9,A..Z
            nm = getTextIndex(c);
          } else {
            // Номер следующей к показу строки, извлеченный из макроса в формате десятичного числа
            nm = s_nn.toInt();
          }
    
          uint8_t sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
          if (nm > 0 && nm < sizeOfTextsArray) {
            String s = textLines[nm];
            if (s.length() > 0 && s[0] != '-' && s.indexOf("{-}") < 0) {
              textLines[nm] = textLines[nm]+"{-}";
              saveTexts();
            }
          }
        }
        
        // Вычислить сколько до его наступления осталось времени
        t_diff = t_event - t_now; // кол-во секунд до события

        // Пересчет секунд в дней до события / часов до события / минут до события
        uint16_t restDays = t_diff / SECS_PER_DAY;
        uint8_t restHours = (t_diff - (restDays * SECS_PER_DAY)) / SECS_PER_HOUR;
        uint8_t restMinutes = (t_diff - (restDays * SECS_PER_DAY) - (restHours * SECS_PER_HOUR)) / SECS_PER_MIN;
        uint8_t restSeconds = (t_diff - (restDays * SECS_PER_DAY) - (restHours * SECS_PER_HOUR) - (restMinutes * SECS_PER_MIN));

        // Если осталось меньше минуты - отображать секунды
        // Если осталось меньше часа - отображать только минуты
        // Если осталось несколько часов - отображать часы и минуты
        // Если осталось меньше-равно 7 дней - отображать дни и часы
        // Если осталось больше 7 дней - отображать дни

        if (restDays == 0 && restHours == 0 && restMinutes == 0)
          tmp = String(restSeconds) + WriteSeconds(restSeconds);
        else if (restDays == 0 && restHours == 0 && restMinutes > 0)
          tmp = String(restMinutes) + WriteMinutes(restMinutes);
        else if (restDays == 0 && restHours > 0 && restMinutes > 0)
          tmp = String(restHours) + WriteHours(restHours) + " " + String(restMinutes) + WriteMinutes(restMinutes);
        else if (restDays > 0 && restDays <= 7 && restHours > 0)
          tmp = String(restDays) + WriteDays(restDays) + " " + String(restHours) + WriteHours(restHours);
        else  
          tmp = String(restDays) + WriteDays(restDays);

        textLine = textLine.substring(0, insertPoint) + tmp + textLine.substring(insertPoint);
      }
    }

    // "{P01.01.2021#N#B#A#F}" 
    // "{P7:00#N#B#A#F}" 
    // "{P**.01.2021 7:00#N#B#A#F}" 
    idx = textLine.indexOf("{P");
    if (idx >= 0) {

      // Строка с событием непрерывной проверки - при старте программы и изменении текста строк и после завершения отображения очередной такой строки формируется массив ближайших событий
      // Массив содержит время наступления события, сколько до и сколько после отображать а так же - индексы отображаемых строк ДО и строки замены ПОСЛЕ события
      // Если данная строка вызвана для отображения - значит событие уже наступило (#B секунд перед событием - его нужно просто отобразить) 
      // Строка ПОСЛЕ события не может содержать макроса {P} и в этот блок просто не попадаем
      // Выкусываем макрос {P} без анализа и отображаем то что осталось с учетом замены макроса на остаток времени

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // удаляем макрос. Позиция idx - точка вставки текста остатка времени      
      textLine.remove(idx, idx2 - idx + 1);
      uint16_t insertPoint = idx;

      // Текущее активное событие - momentIdx, который указывает на элемент массива moments[]
      if (momentIdx >= 0) {
        // Вычислить сколько до наступления события осталось времени. 
        // Время события - поле moment структуры Moment элемента массива
        // Данная строка, содержавшая макрос {P} - всегда ДО события, строка ПОСЛЕ события макроса {P} содержать не может и в этот блок не попадает
        time_t t_diff = moments[momentIdx].moment - now(); // кол-во секунд до события
  
        // Пересчет секунд в дней до события / часов до события / минут до события
        uint16_t restDays = t_diff / SECS_PER_DAY;
        uint8_t restHours = (t_diff - (restDays * SECS_PER_DAY)) / SECS_PER_HOUR;
        uint8_t restMinutes = (t_diff - (restDays * SECS_PER_DAY) - (restHours * SECS_PER_HOUR)) / SECS_PER_MIN;
        uint8_t restSeconds = (t_diff - (restDays * SECS_PER_DAY) - (restHours * SECS_PER_HOUR) - (restMinutes * SECS_PER_MIN));
  
        // Если осталось меньше минуты - отображать секунды
        // Если осталось меньше часа - отображать только минуты
        // Если осталось несколько часов - отображать часы и минуты
        // Если осталось меньше-равно 7 дней - отображать дни и часы
        // Если осталось больше 7 дней - отображать дни
  
        if (restDays == 0 && restHours == 0 && restMinutes == 0)
          tmp = String(restSeconds) + WriteSeconds(restSeconds);
        else if (restDays == 0 && restHours == 0 && restMinutes > 0)
          tmp = String(restMinutes) + WriteMinutes(restMinutes);
        else if (restDays == 0 && restHours > 0 && restMinutes > 0)
          tmp = String(restHours) + WriteHours(restHours) + " " + String(restMinutes) + WriteMinutes(restMinutes);
        else if (restDays > 0 && restDays <= 7 && restHours > 0)
          tmp = String(restDays) + WriteDays(restDays) + " " + String(restHours) + WriteHours(restHours);
        else  
          tmp = String(restDays) + WriteDays(restDays);
        
        textLine = textLine.substring(0, insertPoint) + tmp + textLine.substring(insertPoint);
      }
    }

    // "{S01.01.2020#01.01.2020}"
    // "{S01.01.2020 7:00#01.01.2020 19:00}"
    // "{S01.01.**** 7:00#01.01.**** 19:00}"
    idx = textLine.indexOf("{S");
    if (idx >= 0) {
      // Строка с событием проверки текущей даты - выводится только при совпадении текущей даты с указанной (вычисленной по маске)
      // Проверка строки производится раньше при решении какую строку показывать в getNextLine(). Если сюда попали - значит строка 
      // пригодна к отображению в текущую дату - просто выкусить макрос

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // удаляем макрос;
      textLine.remove(idx, idx2 - idx + 1);
    }

    // Если в строке еще остались макросы, связанные со временем - обработать их
    if (textLine.indexOf("{D}") >= 0 || textLine.indexOf("{D:") >= 0 || textLine.indexOf("{R") >= 0 || textLine.indexOf("{P") >= 0 || textLine.indexOf("{S") >= 0) continue;

    // Если при разборе строка помечена как многоцветная - обработать макросы цвета 
    if (textHasMultiColor) {                                 
      textLine = processColorMacros(textLine);
    }
    
    break;
  }

  return textLine;
}

String substitureDateMacros(const String txt) {  
  String str = txt;
  str.replace("DD", "~1~");
  str.replace("dd", "~2~");
  str.replace("D",  "~3~");
  str.replace("d",  "~4~");
  str.replace("MM", "~5~");
  str.replace("mm", "~6~");
  str.replace("M",  "~7~");
  str.replace("m",  "~8~");
  return str;
}

String unsubstitureDateMacros(const String txt) {  
  String str = txt;
  str.replace("~1~", "DD");
  str.replace("~2~", "dd");
  str.replace("~3~", "D");
  str.replace("~4~", "d");
  str.replace("~5~", "MM");
  str.replace("~6~", "mm");
  str.replace("~7~", "M");
  str.replace("~8~", "m");
  return str;
}

// Обработать макросы цвета в строке, в случае если строка многоцветная
String processColorMacros(const String txt) {

  String text = txt;
  // Обнулить массивы позиций цвета и самого цвета
  for (uint8_t i = 0; i<MAX_COLORS; i++) {
    textColorPos[i] = 0;
    textColor[i] = 0xFFFFFF;
  }

  // Если макрос цвета указан не с начала строки - начало строки отображать цветом globalTextColor
  uint8_t cnt = 0;
  int8_t idx, idx2;  

  idx = text.indexOf("{C");
  if (idx > 0) {
    textColorPos[cnt] = 0;
    textColor[cnt] = globalTextColor;   
    cnt++;
  }

  // Обработать все макросы цвета, входящие в строку
  while (idx >= 0) {
    // Закрывающая скобка
    // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
    idx2 = text.indexOf("}", idx);        
    if (idx2 < 0) break;

    // Извлечь цвет текста отображения этой бегущей строки
    String tmp = "";
    if (idx2 - idx > 1) {
      tmp = text.substring(idx+2, idx2);
    }
    
    // удаляем макрос
    text.remove(idx, idx2 - idx + 1);

    // Преобразовать строку цвета в число, сохранить позицию и цвет
    if (cnt < MAX_COLORS) {
      textColorPos[cnt] = idx;
      textColor[cnt] = (uint32_t)HEXtoInt(tmp);
      cnt++;
    }

    // Последний элемент массива - длина строки, чтобы все буквы до конца строки показывались правильным цветом
    textColorPos[cnt] = text.length();;
    textColor[cnt] = globalTextColor;
    
    // Есть еще вхождения макроса?
    idx = text.indexOf("{C");  
  }
  
  return text;
}

// Проверка содержит ли эта строка множественное задание цвета
bool checkIsTextMultiColor(const String text) {

  // Строка не содержит макроса цвета
  int16_t idx = text.indexOf("{C"), idx_first = idx;
  if (idx < 0) {
    return false;
  }
  
  // Строка отображается одним (указанным) цветом, если цвет указан только один раз в самом начале или в самом конце строки
  // Если цвет в середине строки - значит начало строки отображается цветом globalTextColor, а с позиции макроса и до конца - указанным в макросе цветом
  uint8_t cnt = 0;
  while (idx>=0 && cnt < 2) {
    cnt++;
    idx = text.indexOf("{C", idx + 1);  
  }


  if (cnt == 1 && (idx_first == 0 || idx_first == (int16_t)(text.length() - 10))) {  // text{C#0000FF} поз макр - 4, длина строки - 14, длина макроса - 10
    return false;
  }

  return true;  
}

// получить строку из массива строк текстов бегущей строки по индексу '0'..'9','A'..'Z'
int8_t getTextIndex(char c) {
  uint8_t size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  int8_t idx = -1;
  if (c >= '0' && c <= '9') 
    idx = (int8_t)(c - '0');
  else if (c >= 'A' && c <= 'Z') 
    idx = 10 + (int8_t)(c - 'A');        
  return (idx < 0 || idx >= size) ? -1 : idx;
}

// получить строку из массива строк текстов бегущей строки по индексу '0'..'9','A'..'Z'
char getAZIndex(uint8_t idx) {
  uint8_t size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  char c = '-';
  if (idx <= 9)             
     c = char('0' + idx);
  else if (idx >= 10 && idx < size)               
     c = char('A' + idx - 10);
  return c;
}

// получить строку из массива строк текстов бегущей строки по индексу '0'..'9','A'..'Z'
String getTextByAZIndex(char c) {
  uint8_t size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  int8_t idx = getTextIndex(c);
  return (idx < 0 || idx >= size) ? "" : textLines[idx];
}

// получить строку из массива строк текстов бегущей строки по индексу 0..35
String getTextByIndex(uint8_t idx) {
  uint8_t size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  return (idx >= size) ? "" : textLines[idx];
}

// Сканировать массив текстовых строк на наличие событий постоянного отслеживания - макросов {P}
void rescanTextEvents() {
/*
 "{PДД.ММ.ГГГГ#N#B#A#F}"
   - где после P указаны опционально дата и время до которой нужно отсчитывать оставшееся время, выводя строку остатка в формате:
     X дней X часов X минут X секунд; 
     Если дней не осталось - выводится только X часов X минут; 
     Если часов тоже уже не осталось - выводится X минут
     Если минут тоже уже не осталось - выводится X секунд

   Строка с режимом {P не выводится при периодическом переборе строк для отображения - только до/после непосредственно времени события:
   время наступления события мониторится и за #B секунд до события бегущая строка начинает отображаться на матрице и после события #A секунд отображается строка-заместитель 
   
   Для режима P дата может опускаться (означает "каждый день") или ее компоненты могут быть заменены звездочкой '*'
   **.10.2020 - весь октябрь 2020 года 
   01.**.**** - каждое первое число месяца любого года
   **.**.2020 - каждый день 2020 года
   
   "{P7:00#N#120#30#12347}"  - каждый пн,вт,ср,чт,вс в 7 утра (за 120 сек до наступления события и 30 секунд после наступления
   - где компоненты даты:
     ДД - число месяца
     MM - месяц
     ГГГГ - год
     Компоненты даты могут быть заменены * - означает "любой"
     
   - где после даты может быть указан
     - #N - если осталось указанная дата уже наступила - вместо этой строки выводится строка с номером N
            если строка замены не указана - просроченная строка не выводится берется следующая строка
     - #B - начинать отображение за указанное количество секунд ДО наступления события (before). Если не указано - 60 секунд по умолчанию
     - #A - отображать строку-заместитель указанное количество секунд ПОСЛЕ наступления события (after). Если не указано - 60 секунд по умолчанию
     - #F - дни недели для которых работает эта строка, когда дата не определена 1-пн..7-вс
            Если указана точная дата и указан день недели, который не соответствует дате - строка выведена не будет
*/
  // Предварительная очистка массива постоянно отслеживаемых событий
  for (uint8_t i = 0; i < MOMENTS_NUM; i++) {
    moments[i].moment = 0;
  }

  bool     found = false;
  uint8_t  stage = 0;         // 0 - разбор даты (день); 1 - месяц; 2 - год; 3 - часы; 4- минуты; 5 - строка замены; 6 - секунд ДО; 7 - секунд ПОСЛЕ; 8 - дни недели
  uint8_t  iDay = 0, iMonth = 0, iHour = 0, iMinute = 0, iSecond = 0, star_cnt = 0;
  uint16_t iYear = 0;
  uint32_t iBefore = 60, iAfter = 60, num = 0;
  uint8_t  moment_idx = 0;    // индекс элемента в формируемом массиве
  int8_t   text_idx = -1;     // индекс строки заменителя  
  String   wdays = "1234567"; // Дни недели. Если не указано - все дни пн..вс
  
  for (uint8_t i = 0; i < 36; i++) {
    
    String text = textLines[i];
    int8_t idx = text.indexOf("{P");
    if (idx < 0) continue;

    int8_t idx2 = text.indexOf("}", idx);        
    if (idx2 < 0) continue;

    // Вычленяем содержимое макроса "{P}"
    String str = text.substring(idx+2, idx2);
    str.trim();

    if (!found) {
      DEBUGLN(F("--------------------"));
      DEBUGLN(F("Строки с событием {P}"));
      found = true;
    }
    DEBUGLN(F("--------------------"));
    DEBUGLN(String(F("Строка: '")) + text + "'");

    // Сбрасываем переменные перед разбором очередной строки
    stage = 0; iDay = 0; iMonth = 0; iYear = 0; iHour = 0; iMinute = 0; iSecond = 0; iBefore = 60; iAfter = 60; star_cnt = 0; num = 0;
    wdays = "1234567";
    
    // Побайтово разбираем строку макроса
    bool err = false;
    for (uint16_t ix = 0; ix < str.length(); ix++) {
      if (err) {
        DEBUGLN();
        DEBUG(String(F("Ошибка в макросе\n'{P")) + str + String(F("}'\n  ")));
        for(uint8_t n=0; n<ix; n++) DEBUG('-');
        DEBUGLN('^');
        break;
      }      
      char c = str[ix];
      switch (c) {
        // замена элемента даты "любой" - день месяц или год
        case '*':
          // только для дня/месяца/года и не более двух звезд для дня/ месяца или четырех для года и  нельзя звезду сочетать с цифрой
          err = (stage > 2) || (star_cnt == 1 && num != 0) || (stage <= 1 && star_cnt > 2) || (stage == 2 && star_cnt > 4);  
          if (!err) {
            star_cnt++;  // счетчик звезд
            num = 0;     // обнулить число          
          }
          break;  
        // Разделитель даты дня/месяца/года
        case '.':
          err = stage > 2;  // точка - разделитель элементов даты и в других стадиях недопустима
          if (!err) {
            switch (stage) {
              case 0: iDay = num;   stage = 1; break; // Следующая стадия - разбор месяца
              case 1: iMonth = num; stage = 2; break; // Следующая стадия - разбор года
              case 2: iYear = num; break;
            }
            star_cnt = 0; num = 0;
          }
          break;  
        // Разделитель часов и минут  
        case ':':
          err = stage != 0 && stage != 3;  // Дата может быть опущена (stage == 0) и текущая стадия stage == 3 - был разбор часов. Если это не так - ошибка
          if (!err) {
            iHour = num;
            stage = 4;    // следующая стадия - разбор минут
            num = 0;
          }
          break;  
        // Разделитель строки замены/времени ДО/времени ПОСЛЕ/дней недели  
        case '#':
          switch (stage) {
            case 2: iYear = num;    stage = 5; star_cnt = 0; num = 0;break;   // Сейчас разбор года - переходим в стадию номера строки заменителя
            case 4: iMinute = num;  stage = 5; star_cnt = 0; num = 0; break;  // Сейчас разбор минут времени - переходим в стадию номера строки заменителя
            case 5: text_idx = num; stage = 6; break;  // Закончен разбор номера строки замены; Следующая стадия - разбор секунд ДО
            case 6: iBefore = num;  stage = 7; break;  // Закончен разбор номера секунд ДО; Следующая стадия - разбор секунд ПОСЛЕ
            case 7: iAfter = num;   stage = 8; break;  // Закончен разбор номера секунд ПОСЛЕ; Следующая стадия - разбор дней недели
            default:
              err = true;    // В любой другой стадии № не на своем месте - ошибка
              break;
          }          
          num = 0;  
          break;  
        // Разделитель даты и времени  
        case ' ':
          err = stage != 2;  // Разделитель даты и времени. Если предыдущая фаза - не разбор года - это ошибка. Пробел в других местах недопустим
          if (!err) {
            iYear = num;
            stage = 3; // следующая стадия - разбор часов
            num = 0;
            star_cnt = 0;
          }
          break;  
        default:
          // Здесь могут быть цифры 0..9 для любой стадии (день/месяц/год/часы/минуты/сек ДО/сек ПОСЛЕ/дни недели/номер строки замены)
          if (c >= '0' && c <= '9') {
            err = star_cnt != 0;     // Если число звезд не равно 0 - цифра сочетается со звездой - нельзя
            if (!err) {
              num = num * 10 + (c - '0');
            }
            break;
          }
          // Здесь могут быть буквы A..Z - только для стадии строка замены и при этом буква должна быть только одна
          if (c >= 'A' && c <= 'Z' && stage == 5 && num == 0) {
            num = getTextIndex(c);
            break;
          }
          // Любой другой символ - ошибка разбора макроса
          err = true;
          break;  
      }

      // Если случилась стадия 8 - все от текущей позиции до конца строки - дни недели
      // Просто копируем остаток строки в дни недели и завершаем разбор
      if (!err && stage == 8) {
        wdays = str.substring(ix+1);
        if (wdays.length() == 0) wdays = "1234567";
        break;
      }  

      // Это последний символ в строке?
      if (ix == str.length() - 1) {
        // Если строка кончилась ДО полного разбора даты или времени - ошибка
        // Остальные параметры могут быть опущены - тогда принимают значения по умолчанию
        // 0 - разбор даты (день); 1 - месяц; 2 - год; 3 - часы; 4- минуты; 5 - строка замены; 6 - секунд ДО; 7 -секунд ПОСЛЕ; 8 - дни недели
        switch (stage) {
          case 2:  iYear    = num; break;
          case 4:  iMinute  = num; break;
          case 5:  text_idx = num; break;
          case 6:  iBefore  = num; break;
          case 7:  iAfter   = num; break;
          default: err      = true;  break;
        }                  
      }
    }

    // Разбор прошел без ошибки? Добавить элемент в массив отслеживаемых событий
    if (!err) {
      // Если день/месяц/год отсутствуют или указаны заменителями - брать текущую   

      bool star_day = false, star_month = false, star_year = false;
      if (iDay   == 0) { iDay   = day();   star_day   = true; }
      if (iMonth == 0) { iMonth = month(); star_month = true; }
      if (iYear  == 0) { iYear  = year();  star_year  = true; }

      // Если год меньше текущего - событие в прошлом - его добавлять в отслеживаемые не нужно 
      if (iYear < year()) continue;
      
      // Сформировать ближайшее время события из полученных компонент
      tmElements_t tm = {iSecond, iMinute, iHour, 0, iDay, iMonth, (uint8_t)CalendarYrToTm(iYear)}; 
      time_t t_event = makeTime(tm);            
      
      // Если событие уже прошло - это может быть, когда дата опущена или звездочками, а время указано меньше текущего - брать то же время следующего дня/месяца/года
      const uint8_t monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};

      while ((uint32_t)t_event < (uint32_t)now() && (star_day || star_month || star_year)) {
        if (star_day) {
          iDay++;
          uint8_t daysInMonth = monthDays[iMonth-1];
          if (daysInMonth == 28) daysInMonth += uint8_t(LEAP_YEAR(iYear - 1970));
          if (iDay>daysInMonth) {
            iDay = 1;
            if (star_month) {
              iMonth++;
              if (iMonth>12) {
                iMonth=1;
                if (star_year) iYear++;
                else break;
              }
            } else {
              break;
            }
          }
        }
        else if (star_month) {
          iMonth++;
          if (iMonth>12) {
            iMonth=1;
            if (star_year) iYear++;
            else break;
          }
        }
        else if (star_year) {
          iYear++;          
        }        
        tm = {0, iMinute, iHour, 0, iDay, iMonth, (uint8_t)CalendarYrToTm(iYear)};
        t_event = makeTime(tm);        
      }
      
      // Если звездочек нет или после перехода к следующему дню время всё равно меньше текущего - событие в прошлом - добавлять не нужно
      if ((uint32_t)t_event < (uint32_t)now()) continue;

      // Полученное время события попадает в разрешенные дни недели? Если нет - добавлять не нужно
      int8_t weekDay = weekday(t_event) - 1;    // day of the week, Sunday is day 0   
      if (weekDay == 0) weekDay = 7;            // Sunday is day 7, Monday is day 1;

      char cc = weekDay + '0';            
      if (wdays.indexOf(cc) < 0) continue;
      
      breakTime(t_event, tm);
      
      DEBUGLN(String(F("Событие: ")) + padNum(tm.Day,2) + "." + padNum(tm.Month,2) + "." + padNum(tmYearToCalendar(tm.Year),4) + " " + padNum(tm.Hour,2) + ":" + padNum(tm.Minute,2) + 
                     String(F("; before=")) + String(iBefore) + String(F("; after=")) + String(iAfter) + String(F("; days='")) + wdays + String(F("'; replace='")) + String(getAZIndex(text_idx)) + "'");
      
      // Заполнить текущий элемент массива полученными параметрами
      moments[moment_idx].moment = t_event;
      moments[moment_idx].before = iBefore;
      moments[moment_idx].after = iAfter;
      moments[moment_idx].index_b = i;
      moments[moment_idx].index_a = text_idx;

      // Строка-заместитель должна быть отключена, чтобы она не отображалась как регулярная строка
      if (text_idx >= 0) {
        String text = textLines[text_idx];        
        if (text.length() > 0) {
          bool disabled = (text_idx == 0 && text[0] == '#') || text[0] == '-' || text.indexOf("{-}") >= 0; 
          if (!disabled) {
            textLines[text_idx] = "-" + text;
            saveTexts();
          }
        }
      }
      
      // К следующему элементу массива
      moment_idx++;
    }
  }
  DEBUGLN(F("--------------------"));  
  textCheckTime = millis();
}

// Проверить есть ли в настоящий момент активное событие?
// Возврат - индекс строки текста в массиве текстовых строк или -1, если активного события нет
void checkMomentText() {
  momentIdx = -1;                   // Индекс строки в массиве moments для активного текущего непрерывно отслеживаемого события
  momentTextIdx = -1;               // Индекс строки в массиве textLines для активного текущего непрерывно отслеживаемого события
  time_t this_moment = now();
  for (uint8_t i = 0; i < MOMENTS_NUM; i++) {
    // Не содержит события
    if (moments[i].moment == 0) break;
    // Время за #B секунд до наступления события? - отдать index_b
    if ((uint32_t)this_moment >= moments[i].moment - moments[i].before && this_moment < moments[i].moment) {
      momentIdx = i;
      momentTextIdx = moments[i].index_b; // before
      break;
    }    
    // Время #А секунд после наступления события? - отдать index_a
    if ((time_t)this_moment >= moments[i].moment && (time_t)this_moment <= moments[i].moment + (time_t)moments[i].after) {
      momentIdx = i;
      momentTextIdx = moments[i].index_a; // after
      break;
    }    
  }
}

// Проверить текст, содержащий макрос {S}
// Возвращает true - если дата в макросе после расшифровки совпадает с текущей датой - текст можно отображать
// Если дата не совпадает - текст отображать сегодня нельзя - еще не пришло (или уже прошло) время для этого текста
bool forThisDate(String text) {
  /*
     text - в общем случае - "{S01.01.**** 7:00:00#01.01.**** 19:00:00}" - содержит даты начала и конца, разделенные символом "#"
     Дата как правило имеет формат "ДД.ММ.ГГГГ ЧЧ:MM:СС"; В дате День может быть замене на "**" - текущий день, месяц - "**" - текущий месяц, год - "****" - текущий год или "***+" - следующий год
     Примеры:
       "{S01.01.2020#01.01.2020}"
       "{S01.01.2020 7:00#01.01.2020 19:00}"
       "{S01.01.**** 7:00#01.01.**** 19:00:00}"
         - где после S указаны даты начала и конца периода доступного для отображения строки.
         Для режима S элементы даты быть заменены звездочкой '*'
         **.10.2020 - весь октябрь 2020 года 
         01.**.**** - каждое первое число месяца
         **.**.2020 - каждый день 2020 года
         Если время начала периода отсутствует - считается 00:00:00
         Если время конца периода отсутствует  - считается 23:59:59
         Допускается указывать несколько макросов {S} в строке для определения нескольких разрешенных диапазонов
  */
  bool   ok = false;
  String str;
  int16_t idx2;
  
  int16_t idx = text.indexOf("{S");
  while (idx >= 0) {
    // Строка с событием проверки текущей даты - выводится только при совпадении текущей даты с указанной (вычисленной по маске)
    // Проверка строки производится раньше при решении какую строку показывать в getNextLine(). Если сюда попали - значит строка 
    // пригодна к отображению в текущую дату - просто выкусить макрос

    // Закрывающая скобка
    // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
    idx2 = text.indexOf("}", idx);        
    if (idx2 < 0) break;

    // Извлечь дату события из макроса
    str = "";
    if (idx2 - idx > 1) {
      str = text.substring(idx+2, idx2);
      str.trim();
    }

    // Проверить дату
    if (str.length() > 0) {
      /*
      DEBUGLN(F("--------------------")); 
      DEBUG(F("Строка: '"));
      DEBUGLN(text + "'");
      */
      time_t now_moment = now();
      extractMacroSDates(str);
      ok = (now_moment >= (time_t)textAllowBegin) && (now_moment <= (time_t)textAllowEnd);
      /*
      DEBUGLN("now=" + String(now_moment) + "; start=" + String(textAllowBegin) + "; end=" + String(textAllowEnd));
      if (ok) DEBUGLN(F("вывод разрешен"));
      else    DEBUGLN(F("вывод запрещен"));
      DEBUGLN(F("--------------------")); 
      */
    }

    // Дата проверена и совпадает с допустимым диапазоном - строку можно отображать
    if (ok) break;
    
    // удаляем макрос;
    text.remove(idx, idx2 - idx + 1);

    // Есть ли еще макрос {S} в строке?
    idx = text.indexOf("{S");
  }
  
//  DEBUGLN(F("--------------------"));

  return ok;
}

void extractMacroSDates(String text) {

  // Text - в общем случае - "01.01.**** 7:00:00#01.01.**** 19:00:00" - содержит даты начала и конца, разделенные символом "#"
  // Дата как правило имеет формат "ДД.ММ.ГГГГ ЧЧ:MM:СС"; В дате День может быть замене на "**" - текущий день, месяц - "**" - текущий месяц, год - "****" - текущий год или "***+" - следующий год
  // Если не указана дата начала интервала - берется дата конца интервала и время ставится в 00:00:00
  // Если не указана дата конца интервала - берется дата начала интервала и время ставится в 23:59:59
  
  textAllowBegin = 0;        // время начала допустимого интервала отображения unixTime
  textAllowEnd = 0;          // время конца допустимого интервала отображения unixTime

  if (text.length() == 0) {
    DEBUGLN(String(F("Строка: '")) + text + "'");
    DEBUGLN(F("Ошибка: макрос {S} не содержит интервала дат"));                 
    return;
  }      

  String s_date1, s_date2;
  int8_t idx = text.indexOf('#');

  bool hasDate1 = idx != 0;  // -1 или больше нуля означает,что '#' либо нет - тогда вся строка - data1, либо больше 0 - есть часть для data1; в строке '#07.01.****' есть data2, нет data1
  bool hasDate2 = idx > 0;   // Если в строке есть '#' - значит data2 присутствует
  
  s_date1 = idx < 0 ? text : ( idx == 0 ? "" : text.substring(0, idx));
  s_date2 = idx >=0 ? text.substring(idx+1) : "";

  // Сформировать ближайшее время события из полученных компонент  
  tmElements_t tm1, tm2;
  if (hasDate1) tm1 = ParseDateTime(s_date1);
  if (hasDate2) tm2 = ParseDateTime(s_date2);

  // Если нет начала интервала - брать время 00:00:00 и дату конца интервала 
  if (!hasDate1) tm1 = {0, 0, 0, 0, tm2.Day, tm2.Month, tm2.Year };

  // Если нет конца интервала - брать время 23:59:59 и дату начала интервала 
  if (!hasDate2) tm2 = {59, 59, 23, 0, tm1.Day, tm1.Month, tm1.Year };

  // Если в строке даты окончания периода время не указано вообще (отсутствует пробел как разделитель даты и времени) - время окончания поставить 23:59:59
  if (hasDate2 && s_date2.indexOf(' ') < 0) {
    tm2 = {59, 59, 23, 0, tm2.Day, tm2.Month, tm2.Year };
  }
      
  time_t t_event1 = makeTime(tm1);
  time_t t_event2 = makeTime(tm2);

  /*
  DEBUGLN(F("--------------------")); 
  DEBUGLN("date1='" + s_date1 + ";");
  DEBUGLN("date2='" + s_date2 + ";");
  DEBUGLN(String(F("Интервал показа: ")) + 
                 padNum(tm1.Day,2) + "." + padNum(tm1.Month,2) + "." + padNum(tmYearToCalendar(tm1.Year),4) + " " + padNum(tm1.Hour,2) + ":" + padNum(tm1.Minute,2) + ":" + padNum(tm1.Second,2) + " -- " +
                 padNum(tm2.Day,2) + "." + padNum(tm2.Month,2) + "." + padNum(tmYearToCalendar(tm2.Year),4) + " " + padNum(tm2.Hour,2) + ":" + padNum(tm2.Minute,2) + ":" + padNum(tm2.Second,2));
  DEBUGLN(F("--------------------")); 
  */
  
  if (t_event2 < t_event1) {
    DEBUGLN(String(F("Строка: '")) + text + "'");
    DEBUGLN(String(F("Интервал показа: ")) + 
                   padNum(tm1.Day,2) + "." + padNum(tm1.Month,2) + "." + padNum(tmYearToCalendar(tm1.Year),4) + " " + padNum(tm1.Hour,2) + ":" + padNum(tm1.Minute,2) + ":" + padNum(tm1.Second,2) + " -- " +
                   padNum(tm2.Day,2) + "." + padNum(tm2.Month,2) + "." + padNum(tmYearToCalendar(tm2.Year),4) + " " + padNum(tm2.Hour,2) + ":" + padNum(tm2.Minute,2) + ":" + padNum(tm2.Second,2));
    DEBUGLN(F("Ошибка: дата начала больше даты окончания разрешенного интервала"));                 
    textAllowBegin = 0; // время начала допустимого интервала отображения unixTime
    textAllowEnd   = 0; // время конца допустимого интервала отображения unixTime
    return;
  }      
  
  textAllowBegin = t_event1; // время начала допустимого интервала отображения unixTime
  textAllowEnd   = t_event2; // время конца допустимого интервала отображения unixTime
}

bool isFirstLineControl() {

  // По умолчанию - строка 0 - обычная строка для отображения, а не управляющая
  // Если textLines[0] начинается с '#'  - это строка содержит последовательность индексов строк в каком порядке их отображать. Индексы - 0..9,A..Z
  // Если textLines[0] начинается с '##' - это управляющая строка, показывающая, что строки отображать в случайном порядке
  
  bool isControlLine = false;

  // Однако если строка не пуста и начинается с '#' - это управляющая строка
  if (textLines[0].length() > 0 && textLines[0][0] == '#') {
    isControlLine = true;
    sequenceIdx = 1;
    // Строка из одного символа, и это '#' - считаем что строка - "##" - управляющая - случайное отображение строк
    if (textLines[0].length() < 2) {
      textLines[0] = "##";
    }
    // Если второй символ в строке тоже '#' - остальная часть строки нам не нужна - отбрасываем
    if (textLines[0][1] == '#') {
      textLines[0] = "##";
    }
  }

  if (isControlLine) {
    // Допускаются только 1..9,A-Z в верхнем регистре, остальные удалить; 
    // textLines[0][1] == '#' - допускается - значит брать случайную последовательность
    // '0' - НЕ допускается - т.к. строка массива с индексом 0 - и есть управляющая
    textLines[0].toUpperCase();
    for (uint16_t i = 1; i < textLines[0].length(); i++) {
      char c = textLines[0].charAt(i);
      if ((i == 1 && c == '#') || (c >= '1' && c <= '9') || (c >= 'A' && c <= 'Z')) continue;
      textLines[0][i] = ' ';
    }
    textLines[0].replace(" ", "");
    if (textLines[0].length() < 2) {
      textLines[0] = "##";
    }
  }
  
  return isControlLine;  
}

tmElements_t ParseDateTime(String &str) {

  uint8_t  aday = day();
  uint8_t  amnth = month();
  uint16_t ayear = year();
  uint8_t  hrs = hour();
  uint8_t  mins = minute();
  uint8_t  secs = second();

  String s_date;
  String s_time;
  uint16_t iYear = 0;  
  uint8_t iMonth = 0, iDay = 0, iHours = 0, iMinutes = 0, iSeconds = 0;
  int8_t idx;
  
  tmElements_t tm;  

  // Корректная дата - 10 символов (ДД.ММ.ГГГГ), точки в позициях 2 и 5; UUUU может быть '****' - текущий год или '***+' - следующий год
  // Если есть время - оно отделено пробелом от даты, формат (ЧЧ:MM:СС), часы и минуты и секундв разделены двоеточием
  if (str.length() >= 10) {
    idx = str.indexOf(" ");
    if (idx < 0) idx = str.length();
    s_date = str.substring(0,idx);
    s_time = str.substring(idx+1);
    s_time.trim();
    s_date.trim();
  }

  /*
  DEBUGLN("------------------------------------");
  DEBUGLN("text = '" + str + "'");
  DEBUGLN("parse -> date = '" + s_date + "'; time = '" + s_time + "'");
  */
  
  if (s_date.length() == 10 && s_date.charAt(2) == '.' && s_date.charAt(5) == '.') {
    idx = CountTokens(s_date, '.');
    if (idx > 0) {
      String sDay = GetToken(s_date, 1, '.');
      String sMonth = idx >= 2 ? GetToken(s_date, 2, '.') : "0";
      String sYear = idx >= 3 ? GetToken(s_date, 3, '.') : "0";
      iDay   = sDay == "**" ? aday : sDay.toInt();
      iMonth = sMonth == "**" ? amnth : sMonth.toInt();
      iYear  = sYear == "****" ? ayear : (sYear == "***+" ? (ayear + 1) : sYear.toInt());
      if (iDay   == 0) iDay   = aday;
      if (iMonth == 0) iMonth = amnth;
      if (iYear  == 0) iYear  = ayear;
    }
  }

  if (s_time.length() > 0) {
    idx = CountTokens(s_time, ':');              
    if (idx > 0) {
      iHours = GetToken(s_time, 1, ':').toInt();
      iMinutes = idx >= 2 ? GetToken(s_time, 2, ':').toInt() : 0;
      iSeconds = idx >= 3 ? GetToken(s_time, 3, ':').toInt() : 0;
    }
  }
  
  tm = {iSeconds, iMinutes, iHours, 0, iDay, iMonth, (uint8_t)CalendarYrToTm(iYear)};

  /*
  DEBUGLN(String(F("Parse out: ")) + padNum(tm.Day,2) + "." + padNum(tm.Month,2) + "." + padNum(tmYearToCalendar(tm.Year),4) + " " + padNum(tm.Hour,2) + ":" + padNum(tm.Minute,2) + ":" + padNum(tm.Second,2));
  DEBUGLN("------------------------------------");
  */
  
  return tm;
}
