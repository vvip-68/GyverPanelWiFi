// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------

int offset = WIDTH;

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
    offset = WIDTH;   // перемотка в правый край
    // modeCode = MC_TEXT;
    loadingTextFlag = false;    
#if (SMOOTH_CHANGE == 1)
    loadingTextFlag = thisMode == MC_TEXT && fadeMode < 2 ;
#else
    loadingTextFlag = false;        
#endif
  }

  uint32_t color;

  // Для многоцветной строки начальный цвет храниртся в textColor[0]
  // Задан ли специальный цвет отображения строки?
  // Если режим цвета - монохром (0) или задан неверно (>2) - использовать глобальный или специальный цвет
  
  byte i = 0, j = 0, pos = 0;
  
  while (text[i] != '\0') {

    // Если строка - многоцветная (содержит несколько макросов определения цвета) - определить каким цветом выводется текущая буква строки
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
    if ((byte)text[i] > 191) {    // работаем с русскими буквами!
      i++;
    } else {      
      drawLetter(j, text[i], offset + j * (LET_WIDTH + SPACE), color);
      i++;
      j++;
    }
  }
  fullTextFlag = false;

  // Строка убежала?
  if (offset < -j * (LET_WIDTH + SPACE)) {
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
  byte size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  char buf[size + 1];
  memset(buf, '\0', size + 1);
  for (byte i=0; i < size; i++) {
    String text = textLines[i];    
    char c = '0';    // статус - пустая
    if (text.length() > 0) {
      if (i == 0 && text[0] == '#')
        c = '5';     // статус - строка 0 - управляющая последовательность
      else if (text[0] == '-' || text.indexOf("{-}") >=0)
        c = '1';     // статус - отключена
      else if (text.indexOf("{") < 0)
        c = '2';     // статус - текст без макросов
      else if (text.indexOf("{") >= 0 && text.indexOf("{D") < 0 && text.indexOf("{R") < 0)
        c = '3';     // статус - текст с макрросами, но без дато-зависимых макросов
      else if (text.indexOf("{D") >= 0 || text.indexOf("{R") >= 0)
        c = '4';     // статус - текст с макрросами, но без дато-зависимых макросов
    }
    buf[i] = c;
  }  
  return String(buf);
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

// Получить / установить настройки отображения очередного текста бегущей строки
// Если нет строк, готовых к ротображению (например все строки отключены) - вернуть false - энет готовых строк'
boolean prepareNextText() {
  
  int8_t nextIdx = nextTextLineIdx;
  textShowTime = -1;              // Если больше нуля - сколько времени отображать бегущую строку в секундах; Если 0 - используется textShowCount; В самой строке спец-макросом может быть указано кол-во секунд
  textShowCount = 1;              // Сколько раз прокручивать текст бегущей строки поверх эффектов; По умолчанию - 1; В самой строке спец-макросом может быть указано число 
  useSpecialTextColor = false;    // В текущей бегущей строке был задан цвет, которым она должна отображаться
  specialTextColor = 0xffffff;    // Цвет отображения бегущей строки, может быть указан макросом в строке. Если не указан - используются глобальные настройки цвета бегущей строки
  specialTextEffect = -1;         // Эффект, который нужно включить при начале отображения строки текста, может быть указан макросом в строке.
  nextTextLineIdx = -1;           // Какую следующую строку показыват, может быть указан макросом в строке. Если указан - интервал отображения игнорируется, строка показывается сразу;
  textHasDateTime = false;        // Строка имеет макрос отображения текущего времени - ее нужно пересчитывать каждый раз при отрисовкеж Если макроса времени нет - рассчитать текст строки один раз на этапе инициализации  
  textHasMultiColor = false;      // Строк имеет множественное определение цвета - многоцветная строка
  currentText  = "";              // Текст текущей отображаемаой строки

  // Получить очередную строку из массива строк, обработав все макросы кроме даты-зависимых
  // Если макросы, зависимые от даты есть - установить флаг textHasDateTime? макросы оставить в строке
  // Результат положить в currentText
  // Далее если флаг наличия даты в строке установлен - каждый раз перед отрисовкой выполнять подстановку даты и
  // обработку наступления даты последнего показа и переинициализацию строки
  // Если зависимостей от даты нет - вычисленная строка просто отображается пока не будет затребована новая строка

  offset = WIDTH;   // перемотка новой строки в правый край

  // Размер массива строк
  byte sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк

  // Если nextIdx >= 0 - значит в предыдущей строке было указано какую строку показывать следующей - показываем ее
  currentTextLineIdx = nextIdx >= 0 ? nextIdx : getNextLine(currentTextLineIdx);

  // Если индекс указанной строки, готовящейся к отображению выходит за рамки массива - брать нулевую/первую строку имеющегося массива строк
  // Нулевую, если textLines[0] не начинается с '#' - это просто строка, если начинается с '#' - это управляющая последовательность, брать первую строку
  if (currentTextLineIdx < 0 || currentTextLineIdx >= sizeOfTextsArray) {
    currentTextLineIdx = textLines[0].charAt(0) == '#' ? 1 : 0;
  }
  
  currentText = currentTextLineIdx < 0 ? "" : processMacrosInText(textLines[currentTextLineIdx]);

  return currentText.length() > 0;
}

// Получить индекс строки для отображения
// -1 - если показывать нечего
int8_t getNextLine(int8_t currentIdx) {
  // Если не задана следующая строка - брать следующую из массива? в соответствии с правилом
  // sequenceIdx < 0 - просто брать следующую строку
  // sequenceIdx > 0 - строка textLines[0] содержит последовательность отображения строк, например "#12345ZYX"
  //                   в этом случае sequenceIdx = 1..textLines[0].length() и показывает на символ в строке, содержащий индек следующей строки к отображению
  //                   Если извлеченный символ - '#' - брать случайную строку из массива
  int8_t nextLineIdx = currentIdx;
  if (sequenceIdx < 1) {
    nextLineIdx++;
  } else {
    if (sequenceIdx >= textLines[0].length()) {
      sequenceIdx = 1;  // перемотать на начало последовательности
    }
    char c = textLines[0].charAt(sequenceIdx);
    if (c == '#') {
      byte size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
      byte arr[size], cnt = 0;
      memset(arr, '\0', size + 1);
      for (int i = 0; i < size; i++) {
        String text = textLines[i];
        bool disabled = (text.length() == 0) || (i == 0 && text[0] == '#') || (text[0] == '-') || (text.indexOf("{-}") >= 0);
        if (disabled) continue;
        arr[cnt++] = i;
      }
      // Выбрать индексы строк, которые не отключены;
      if (cnt == 0)
        nextLineIdx = -1;
      else {
        byte att = 0;
        byte idx = random8(0,cnt - 1);
        while (arr[idx] == nextLineIdx && att < cnt) {
          att++; idx++;
          if (idx >= cnt) idx = 0;
        }        
        nextLineIdx = arr[idx];
      }
    } else {
      nextLineIdx = getTextIndex(c);
      sequenceIdx++;
    }
  }
  
  return nextLineIdx;
}

// Выполнить разбор строки на наличие макросов, применить указанные настройки
String processMacrosInText(String textLine) {  

  /*   
     Общие правила:
     - Макрос - последовательность управляющих конструкций, каждая из которых заключена в фигурные скобки, например "{Exx}" 
     - Если строка textLines[0] начинается с '#'  - строка содержит последовательность строк к отображению, например "#1234ZYX"
     - Если строка textLines[0] начинается с '##' - отображать строки из массива в случайном порядке
     - Если строка textLines[0] НЕ начинается с '#' или '##' - это обычная строка для отображения
     - Если строка начинается с '-' или пустая или содержит макрос '{-}' - строка отключена и не отображается 
     
     Строка, извлеченная из массива может содержать в себе следующие макросы:
      "{-}"  - в любом месте строки означает, что строка временно отключена, аналогично "-" в начале строки
      "{#N}" - в любом месте строки означает,что после отображения строки, следующей строкой отображать строку с номером N, где N - 1..9,A..Z
               Настройка из п.0 (через какое время показывать следующую строку) игнорируется, строка показывается сразу
               после завершения отображения "родительской" строки
      "{Exx}"- эту строку отображать на фоне эффекта xx - где xx - номер эффекта. Эффект не меняется пока отображается строка      
      "{TS}" - отображать строку указанное количество секунд S
      "{NX}" - отображать строку указанное количество раз X
               если S или N не указаны - строка прокручивается 1 раз;
               Если указаны оба - работает модификатор показа по времени
      "{CC} "- отображать строку указанным цветом С; Цвет - в виде #AA77FE; Специальные значения - #000001 - радуга;  - #000002 - каждая буква свой цвет;
      "{BC} "- отображать строку на однотонном фоне указанного цвета С; Цвет - в виде #337700;
      "{WS} "- отображать вместо {WS} состояние текущей погоды - "Пасмурно", "Ясно", "Дождь", "Гроза" и т.д
      "{WT} "- отображать вместо {WT} текущую температуру водуха, например "+26", "-31"
      "{D:F}" - где F - один из форматов даты / времени
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
        пример: "Красноярское время {DHH:mm}" - бегущая строка "Красноярское время 07:15"  
                "Сегодня {D:DD MMMM YYYY} года" - бегущая строка "Сегодня 26 июля 2020 года"
     "{D}"                         - просто часы вида "21:00" в виде беугщей строки
     "{R01.01.2021#N}" 
       - где после R указана дата до которой нужно отсчитывать оставшееся время, выводя строку остатка в формате:
           X дней X часов X минут; 
         Если дней не осталось - выводится только X часов X минут; 
         Если минут тоже уже не осталось - выводится X минут
     
       - где после даты может быть указан
         - #N - если осталось указанная дата уже наступила - вместо этой строки выводится строка с номером N
                если строка замены не указана или отключена (символ "-" вначале строки) - просроченная строка не выводится
                берется следующая строка
  */

  // Выполнять цикл поиска подходящей к отображению строки
  // Если ни одной строки не найдено - возвратить false

  boolean found = false;
  uint8_t attempt = 0;
  int16_t idx, idx2;
  char c;
  String tmp;

  // Размер массива строк
  byte sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк

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
    // Оставить эти форматы как есть в строке - они будут обрабатываться на каждом проходе, подставляя текeщее время
    // Сейчас просто выставить флаг, что строка содержит макросы, зависимые от даты
    //    "{DF}" - где F - один из форматов даты / времени если формата даты нет - аналогично {D}
    //    "{D}"  - просто часы вида "21:00" в виде беугщей строки
    //    "{R01.01.2021#N}" 
    // -------------------------------------------------------------

    textHasDateTime = textLine.indexOf("{D") >= 0 || textLine.indexOf("{R") >= 0;  

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
    //  "{Exx} - эту строку отображать на фоне эффекта xx - где xx - номер эффекта. Эффект не меняется пока отображается строка      
    // -------------------------------------------------------------

    specialTextEffect = -1;
    idx = textLine.indexOf("{E");
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

      // Преобразовать строку в число
      idx = tmp.length() > 0 ? tmp.toInt() : -1;
      specialTextEffect = idx >= 0 && idx < MAX_EFFECT ? idx : -1;
      
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

    // {WS} - отображать текущую погоду - "Ясно", "Облачно" и т.д
    idx = textLine.indexOf("{WS}");
    if (idx >= 0) {
      textLine.replace("{WS}", weather);
    }

    // {WT} - отображать текущую температурв в виде "+26" или "-26"
    // Если включено отображение температуры цветом - добавить макрос цвета перед температурой 
    idx = textLine.indexOf("{WT}");
    if (idx >= 0) {
      // Подготовить строку текущего времени HH:mm и заменить все вхождения {D} на эту строку
      String s_temperature = (temperature == 0 ? "" : (temperature > 0 ? "+" : "-")) + String(temperature);
      String s_color = "";

      if (useTemperatureColor) {
        s_color = "{C" + getTemperatureColor(temperature) + "}";
      }
      
      textLine.replace("{WT}", s_color + s_temperature);
    }

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
String processDateMacrosInText(String textLine) {
  
  /* -------------------------------------------------------------
   Эти форматы содержат строку, зависящую от текущего времени.
   Оставить эти форматы как есть в строке - они будут обрабатываться на каждом проходе, подставляя текцщее время
   Сейчас просто выставить флаг, что строка содержит макросы, зависимые от даты
      "{D}"  - просто часы вида "21:00" в виде беугщей строки
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
         - где после R указана дата до которой нужно отсчитывать оставшееся время, выводя строку остатка в формате:
             X дней X часов X минут; 
           Если дней не осталось - выводится только X часов X минут; 
           Если минут тоже уже не осталось - выводится X минут
       
         - где после даты может быть указан
           - #N - если осталось указанная дата уже наступила - вместо этой строки выводится строка с номером N
                  если строка замены не указана или отключена (символ "-" вначале строки) - просроченная строка не выводится
                  берется следующая строка
     ------------------------------------------------------------- 
  */

  String   str, tmp, outText;
  uint8_t  idx, idx2, iDay, iMonth;
  uint16_t iYear;
  uint8_t  aday = day();
  uint8_t  amnth = month();
  uint16_t ayear = year();
  uint8_t  hrs = hour();
  uint8_t  mins = minute();
  uint8_t  secs = second();
  bool     am = isAM();
  bool     pm = isPM();

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
    while (idx >= 0) {
  
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
  
      //  dddd - полное название дня недели            (допускается DDDD)
      str = getWeekdayString(wd);  // DDDD  
      sFmtProcess.replace("DDDD", str);
      sFmtProcess.replace("dddd", str);
      
      //  ddd  - сокращенное название дня недели       (допускается DDD)
      str = getWeekdayShortString(wd);  // DDD
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
      sFmtProcess.replace("MMMM", str);
      
      //  MMM  - месяц прописью (янв..дек)
      str = getMonthShortString(amnth);
      sFmtProcess.replace("MMM", str);
  
      //  MM   - месяц от 01 до 12
      str = String(amnth);
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("MM", str);
  
      //  M    - месяц от 1 до 12
      str = String(amnth);
      sFmtProcess.replace("M", str);
  
  
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
      if (hrs > 12) hrs = hrs - 12;
      if (hrs == 0) hrs = 12;
      str = String(hrs);
      if (str.length() < 2) str = "0" + str;    
      sFmtProcess.replace("hh", str);
  
      //  h    - час в 12-часовом формате от 1 до 12
      str = String(hrs);
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
  
      // Заменяем в строке макрос с исходной форматной строкой на обработанную строку с готовой текущей датой
      textLine.replace("{D:" + sFormat + "}", sFmtProcess);
      
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{D:");
  
      break;
    }

    // "{R01.01.2021#N}" 
    idx = textLine.indexOf("{R");
    if (idx >= 0) {
      // Если время  соббытия уже наступило и в строке указана строка для отображения ПОСЛЕ наступления события - показывать эту строку
      // Если замены строки после наступления события нет - textLine будет пустой - отображать нечего
      // Строка замены снова может содержать метки времени - поэтому отправить проверку / замену на второй круг

      // Закрывающая скобка
      // Если ее нет - ошибка, ничего со строкой не делаем, отдаем как есть
      idx2 = textLine.indexOf("}", idx);        
      if (idx2 < 0) break;

      // Извлечь дату события и строку замены ПОСЛЕ события (если есть)
      str = "";
      if (idx2 - idx > 1) {
        str = textLine.substring(idx+2, idx2);
      }

      // удаляем макрос; точка вставки строки остатка будет будет в позицию idx
      textLine.remove(idx, idx2 - idx + 1);

      // Здесь str имеет вид '01.01.2020', если строки замены ПОСЛЕ события нет или '01.01.2020#J' или '01.01.2020#19' если строка замены указана (J=19 - индекс в массиве)
      if (str.length() > 0) {

        // Точка вставки строки остатка
        uint16_t insertPoint = idx;
        uint8_t  afterEventIdx = -1;

        // Есть индекс строки замены? Если есть - отделить ее от даты события
        idx = str.indexOf("#");
        String s_nn = "";
        if (idx>=0) {
          s_nn = idx>=0 ? str.substring(idx+1) : "";
          str = str.substring(0,idx);
        }

        // Здесь - str - дата события, s_nn - номер строки замены или пустая строка, если строки замены нет
        // Получить дату наступления события из строки 'ДД.ММ.ГГГГ'
        time_t t_now = now();
        time_t t_event = now();
        time_t t_diff = 0;
        
        // Корректная дата - 10 символов, точки в позициях 2 и 5
        if (str.length() == 10 && str.charAt(2) == '.' && str.charAt(5) == '.') {
           int8_t iDay   = (str[0] - '0') * 10 + (str[1] - '0');
           int8_t iMonth = (str[3] - '0') * 10 + (str[4] - '0');
           str = str.substring(6);
           int16_t iYear = str.toInt();

           tmElements_t tm = {0, 0, 0, 0, iDay, iMonth, CalendarYrToTm(iYear)};
           t_event = makeTime(tm);
        }

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
      
            byte sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
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

          // Получить текст строки замены. Он может содержать макросы, в т.ч и макросы даты. Их нужно обработать.
          // Для того, чтобы указанная строка замены не отображалась при обычном переборе - она должна быть помечена как отключенная - '-' в начале строки или '{-}' в любом месте
          // Когда же пришло время отобразить строку - перед обработкой макросов ее нужно "включить"
          textLine = textLines[afterEventIdx];

          if (textLine.length() == 0) return "";              
          if (textLine[0] == '-') textLine = textLine.substring(1);
          if (textLine.indexOf("{-}") >= 0) textLine.replace("{-}", "");
          if (textLine.length() == 0) return "";    
          
          textLine = processMacrosInText(textLine);
          
          // Строка замены содержит в себе макросы даты? Если да - вычислить всё снова для новой строки                   
          if (textLine.indexOf("{D") >= 0 || textLine.indexOf("{R") >= 0) continue;

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
    
          byte sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
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
                
        if (textLine.indexOf("{D") >= 0 || textLine.indexOf("{R") >= 0) continue;
      }
    }

    if (textHasMultiColor) {                                // Если при разборе строка помечена как многоцветная - обработать макросы цвета  
      textLine = processColorMacros(textLine);
    }
    
    break;
  }

  return textLine;
}

// Обработать макросы цвета в строке, в случае если строка многоцветная
String processColorMacros(String text) {
  
  // Обнулить массивы позиций цвета и самого цвета
  for (byte i = 0; i<MAX_COLORS; i++) {
    textColorPos[i] = 0;
    textColor[i] = 0xFFFFFF;
  }

  // Если макрос цвета указан не с начала строки - начало строки отображать цветом globalTextColor
  byte cnt = 0;
  int8_t idx, idx2;  
  
  idx = text.indexOf("{C");
  if (idx > 0) {
    textColorPos[idx] = 0;
    textColor[idx] = globalTextColor;   
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

    // Преобразовать строку цвета в число, сохранить прозицию и цвет
    if (cnt < MAX_COLORS) {
      textColorPos[cnt] = idx;
      textColor[cnt] = (uint32_t)HEXtoInt(tmp);
      cnt++;
    }

    // Последний элемент массива - длмна строки, чтобы все буквы до конца строки показывались правильным цветом
    textColorPos[cnt] = text.length();;
    textColor[cnt] = globalTextColor;
    
    // Есть еще вхождения макроса?
    idx = text.indexOf("{C");  
  }
  
  return text;
}

// Проверка содержит ли эта строка множественное задание цвета
boolean checkIsTextMultiColor(String text) {

  // Строка не содержит макроса цвета
  int16_t idx = text.indexOf("{C"), idx_first = idx;
  if (idx < 0) {
    return false;
  }
  
  // Строка отображается одним (указанным) цветом, если цвет указан только один раз в самом начале или в самом конце строки
  // Если цвет в середине строки - значит начало строки отображается цветом globalTextColor, а с позиции макроса и до конца - указанным в макросе цветом
  byte cnt = 0;
  while (idx>=0 && cnt < 2) {
    cnt++;
    idx = text.indexOf("{C", idx + 1);  
  }


  if (cnt == 1 && (idx_first == 0 || idx_first == (text.length() - 10))) {  // text{C#0000FF} поз макр - 4, длина строки - 14, длина макроса - 10
    return false;
  }

  return true;  
}

// получить строку из массива строк текстов бегущей строки по индексу '0'..'9','A'..'Z'
int8_t getTextIndex(char c) {
  byte size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  int8_t idx = -1;
  if (c >= '0' && c <= '9') 
    idx = (int8_t)(c - '0');
  else if (c >= 'A' && c <= 'Z') 
    idx = 10 + (int8_t)(c - 'A');        
  return (idx < 0 || idx >= size) ? -1 : idx;
}

// получить строку из массива строк текстов бегущей строки по индексу '0'..'9','A'..'Z'
char getAZIndex(byte idx) {
  byte size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  char c;
  if (idx >= 0 && idx <= 9)             
     c = char('0' + idx);
  else if (idx >= 10 && idx < size)               
     c = char('A' + idx - 10);
  return c;
}

// получить строку из массива строк текстов бегущей строки по индексу '0'..'9','A'..'Z'
String getTextByAZIndex(char c) {
  byte size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  int8_t idx = getTextIndex(c);
  return (idx < 0 || idx >= size) ? "" : textLines[idx];
}
