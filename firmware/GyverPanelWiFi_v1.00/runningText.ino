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
    if (textHasDateTime) 
      text = processDateMacrosInText(currentText);   // Обработать строку, превратив макросы даты в отображаемые значения
    else 
      text = currentText;                       // Строка не содержит изменяемых во времени компонентов - отобразить ее как есть
  }

  // 0 - монохром: 1 - радуга; 2 -  каждая буква свой цвет
  uint32_t color = COLOR_TEXT_MODE;

  // Задан ли специальный цвет отображения строки?
  // Если режим цвета - монохром (0) или задан неверно (>2) - использовать глобальный или специальный цвет
  if (color == 0 || color > 2) {
    color = useSpecialTextColor ? specialTextColor : globalTextColor;
  }

  fillString(text, color);
}

String processDateMacrosInText(String textLine) {
  /* -------------------------------------------------------------
   Эти форматы содержат строку, зависящую от текущего времени.
   Оставить эти форматы как есть в строке - они будут обрабатываться на каждом проходе, подставляя текцщее время
   Сейчас просто выставить флаг, что строка содержит макросы, зависимые от даты
      "{DF}" - где F - один из форматов даты / времени если формата даты нет - аналогично {D}
      "{D}"  - просто часы вида "21:00" в виде беугщей строки
       Формат даты поддерживает следующие спецификаторы       
          d    - день месяца, в диапазоне от 1 до 31.  (допускается D)
          dd   - день месяца, в диапазоне от 01 до 31. (допускается DD)
          ddd  - сокращенное название дня недели       (допускается DDD)
          dddd - полное название дня недели            (допускается DDDD)
          M    - месяц от 1 до 12
          MM   - месяц от 01 до 12
          MMM  - месяц прописью (янв..дек)
          MMMМ - месяц прописью (января..декабря)
          Y    - год в диапазоне от 0 до 99
          YY   - год в диапазоне от 00 до 99
          YYYY - год в виде четырехзначного числа
          h    - час в 12-часовом формате от 1 до 12
          hh   - час в 12-часовом формате от 01 до 12
          H    - час в 23-часовом формате от 0 до 23
          HH   - час в 23-часовом формате от 00 до 23
          m    - минуты в диапазоне от 0 до 59
          mm   - минуты в диапазоне от 00 до 59
          s    - секунды в диапазоне от 0 до 59
          ss   - секунды в диапазоне от 00 до 59
          t    - Первый символ указателя AM/PM
          tt   - Указатель AM/PM
  
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

  uint8_t  idx, idx2;
  uint8_t  aday = day();
  uint8_t  amnth = month();
  uint16_t ayear = year();
  uint8_t  wd = weekday();  // day of the week, Sunday is day 0 
  uint8_t  hrs = hour();
  uint8_t  mins = minute();
  uint8_t  secs = second();
  bool     am = isAM();
  bool     pm = isPM();

  if (wd == 0) wd = 7;      // Sunday is day 7

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
    String sFormat = "$", sFmtProcess = "#", str;

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

    //  YY   - год в диапазоне от 00 до 99
    str = str.substring(2);
    sFmtProcess.replace("YY", str);

    //  Y    - год в диапазоне от 0 до 99
    if (str[0] == '0') str = str.substring(1);
    sFmtProcess.replace("Y", str);

    
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
    sFmtProcess.replace("ss", str);

    //  s    - секунды в диапазоне от 0 до 59
    str = String(secs);
    sFmtProcess.replace("s", str);

    //  tt   - Указатель AM/PM
    str = am ? "AM" : (pm ? "PM" : "");
    sFmtProcess.replace("tt", str);

    //  t    - Первый символ указателя AM/PM
    str = am ? "A" : (pm ? "P" : "");
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
    textLine = getRestDaysString(textLine);
  }

  return textLine;
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
  currentText  = "";              // Текст текущей отображаемаой строки
  ignoreTextOverlaySettingforEffect = false;  // Сброс флага "отображать строку на эффекте, даже если эффектом это запрещено"

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

  currentText = processMacrosInText(textLines[currentTextLineIdx]);

  return currentText.length() > 0;
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
      "{D:F}" - где F - один из форматов даты / времени
        d    - день месяца, в диапазоне от 1 до 31.  (допускается D)
        dd   - день месяца, в диапазоне от 01 до 31. (допускается DD)
        ddd  - сокращенное название дня недели       (допускается DDD)
        dddd - полное название дня недели            (допускается DDDD)
        M    - месяц от 1 до 12
        MM   - месяц от 01 до 12
        MMM  - месяц прописью (янв..дек)
        MMMМ - месяц прописью (января..декабря)
        Y    - год в диапазоне от 0 до 99
        YY   - год в диапазоне от 00 до 99
        YYYY - год в виде четырехзначного числа
        h    - час в 12-часовом формате от 1 до 12
        hh   - час в 12-часовом формате от 01 до 12
        H    - час в 23-часовом формате от 0 до 23
        HH   - час в 23-часовом формате от 00 до 23
        m    - минуты в диапазоне от 0 до 59
        mm   - минуты в диапазоне от 00 до 59
        s    - секунды в диапазоне от 0 до 59
        ss   - секунды в диапазоне от 00 до 59
        t    - Первый символ указателя AM/PM
        tt   - Указатель AM/PM
        
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
    // -------------------------------------------------------------    

    if (textLine.length() == 0 || textLine.charAt(0) == '-' || textLine.indexOf("{-}") >= 0) {
      attempt++;  
      currentTextLineIdx = getNextLine(currentTextLineIdx);
      textLine = textLines[currentTextLineIdx];
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
        if (c >= '1' && c <= '9') 
          nextTextLineIdx = (int8_t)(c - '0');
        else if (c >= 'A' && c <= 'Z') 
          nextTextLineIdx = 10 + (int8_t)(c - 'A');        
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
    //  "{СS}" - отображать строку указанное количество секунд S
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
    //  "{NN}" - отображать строку указанное количество раз N
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
    // "{LC}"- отображать строку указанным цветом С; Цвет - в виде #AA77FE
    // -------------------------------------------------------------

    useSpecialTextColor = false;
    idx = textLine.indexOf("{C");
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
      useSpecialTextColor = true;
      specialTextColor = (uint32_t)HEXtoInt(tmp);
      
      // Есть еще вхождения макроса?
      idx = textLine.indexOf("{C");  
    }
      
    // -------------------------------------------------------------
    // Эти форматы содержат строку, зависящую от текущего времени.
    // Оставить эти форматы как есть в строке - они будут обрабатываться на каждом проходе, подставляя текцщее время
    // Сейчас просто выставить флаг, что строка содержит макросы, зависимые от даты
    //    "{DF}" - где F - один из форматов даты / времени если формата даты нет - аналогично {D}
    //    "{D}"  - просто часы вида "21:00" в виде беугщей строки
    //    "{R01.01.2021#N}" 
    // -------------------------------------------------------------

    textHasDateTime = textLine.indexOf("{D") >= 0 || textLine.indexOf("{R") >= 0;  

    attempt++;
  }

  return found ? textLine : "";
}

// Получить индекс строки для отображения
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
    byte sizeOfTextsArray = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
    char c = textLines[0].charAt(sequenceIdx);
    if (c == '#') {
      nextLineIdx = random8(1,sizeOfTextsArray-1);
    } else {
      if (c >= '1' && c <= '9') 
        nextLineIdx = (int8_t)(c - '0');
      else if (c >= 'A' && c <= 'Z') 
        nextLineIdx = 10 + (int8_t)(c - 'A');        
      sequenceIdx++;
    }
  }
  return nextLineIdx;
}

// Сдвинуть позицию отображения строки
void shiftTextPosition() {
  offset--;
}

void fillString(String text, uint32_t color) {
  if (loadingTextFlag) {
    offset = WIDTH;   // перемотка в правый край
    // modeCode = MC_TEXT;
    loadingTextFlag = false;    
#if (SMOOTH_CHANGE == 1)
    loadingTextFlag = thisMode == MC_TEXT && fadeMode < 2 ;
#else
    loadingTextFlag = false;        
#endif
    loadingTextFlag = false;
  }
  
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
