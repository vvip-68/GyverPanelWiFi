// *************************** КАРТИНКИ (СЛАЙДЫ) ****************************
//
// Поиск картинок, пригодных для эффекта "Слайды":
//   1. Если доступна SD-карта - ищем картинки на ней, сначала в папке с размерами WxH
//   2. Если картинки не найдены - ищем картинки в файловой системе МК, сначала в папке с размерами WxH
//   3. Если картинки не найдены и доступна SD-карта - ищем картинки на ней, в папке с размерами 16x16
//   4. Если картинки не найдены - ищем картинки в файловой системе МК,  в папке с размерами 16x16
//   5. Если картинки не найдены - слайды 16x16 будем брать из эффекта анимации "Погода" - там 20 фреймов - 20 картинок 16x16
//
// Картинки для эффекта "Слайды" могут быть подготовлены в приложении JinxFramer или нарисованы на самой матрице 
// используя приложение управления матрицей WiFiPlayer
//
// Подготовленные картинки должны быть помещены а папку 'data' скетча, внутрь папки "WpH" - где W - ширина матрицы и H - высота матрицы.
// Так, для полного размера матрицы 24 x 18 имя папки должно быть "24p18" и в ней - подготовленные картинки размером 24x18
// 
// Для примера в папке дата размещена папка 16p16 и в ней набор картинок "от автора".
// Если папка с размерами картинок под вашу матрицу не будет найдена микроконтроллером - для эффекта будут смпользованы картинки 16x16
// из папки с примерами "16p16";
//
// Если и папка папка с примерами "16p16" также не будет найдена, для эффекта будут смпользованы картинки 16x16 из анимации "Погода".
//
// Для загрузки файлов в файловую систему микроконтроллера используется плагин к Arduino IDE, который находится в папке проекта tools/LittleFS_Uploader
// Инструкция по установки плагина в IDE и подсказки к использованию - в файле readme.txt в той же папке
//
// Если размеры картинки слайда меньше размера матрицы на 6 пикселей - она будет плавно перемещаться по полю матрицы.
// Скорость перемещения задается слайдером "Вариант". В крайнем левом положении слайдера картинка не перемещается и выводится по центру поля матрицы
//

CRGB     *picture;         // Буфер для загрузки кадра
int8_t   pic_offset_x;     // Для движущихся картинок - смещение левого верхнего угла
int8_t   pic_offset_y;
int8_t   dir_x;            // Направление движения по x
int8_t   dir_y;            // Направление движения по y
uint16_t move_delay_x;     // коли-во миллисекунд 5-200 * 10 через которое смещение по x
uint16_t move_delay_y;     // коли-во миллисекунд 5-200 * 10 через которое смещение по y
int8_t   pic_fade;         // способ появления / скрытия картинки


void initialisePictures() {

  uint8_t saveWidth = pWIDTH;
  uint8_t saveHeight = pHEIGHT;
  uint8_t tokenCount;
  
  // Для поиска картинок в папке используем процедуру getStoredImages() из animation.ini
  // Поскольку она ищет картинки в папке, соответствующей текущим pWIDTH и pHEIGHT - их нужно будет временно переопределить, в конце процедуры - восстановить
  // Результат из процедуры getStoredImages() - строка с именами файлов, разделенные запятыми
  #if (USE_SD == 1 && FS_AS_SD == 0)
    DEBUG(F("Поиск слайдов на SD в папке '"));
    DEBUGLN(String(pWIDTH) + "p" + String(pHEIGHT) + "'...");
    pictureStorage = "SD";
    pictureList = getStoredImages(pictureStorage);
    if (pictureList.length() != 0) {
      pictureWidth = pWIDTH;
      pictureHeight = pHEIGHT;
    }
  #endif

  if (pictureList.length() == 0) {
    DEBUG(F("Поиск слайдов на FS в папке '"));
    DEBUGLN(String(pWIDTH) + "p" + String(pHEIGHT) + "'...");
    pictureStorage = "FS";
    pictureList = getStoredImages(pictureStorage);
    if (pictureList.length() != 0) {
      pictureWidth = pWIDTH;
      pictureHeight = pHEIGHT;
    }    
  }

  if (pWIDTH != 16 || pHEIGHT != 16) {
    pWIDTH = 16;
    pHEIGHT = 16;
    
    #if (USE_SD == 1 && FS_AS_SD == 0)
      if (pictureList.length() == 0) {
        DEBUGLN(F("Поиск слайдов на SD в папке '16p16'..."));
        pictureStorage = "SD";
        pictureList = getStoredImages(pictureStorage);
        if (pictureList.length() != 0) {
          pictureWidth = pWIDTH;
          pictureHeight = pHEIGHT;
        }    
      }
    #endif
  
    if (pictureList.length() == 0) {
      DEBUGLN(F("Поиск слайдов на FS в папке '16p16'..."));
      pictureStorage = "FS";
      pictureList = getStoredImages(pictureStorage);
      if (pictureList.length() != 0) {
        pictureWidth = pWIDTH;
        pictureHeight = pHEIGHT;
      }    
    }
  }

  if (pictureList.length() == 0) {
    // Если подходящих картинок не найдено - будем брать картинки 16x16 - фреймы из эффекта "Погода" 
    pictureStorage = "";
    pictureWidth = 16;
    pictureHeight = 16;
    // Найти анимацию "Погода", посчитать сколько фреймов в анимации (макс MAX_FRAMES_COUNT)
    String animList = String(IMAGE_LIST);
    tokenCount = CountTokens(animList, ',');
    FOR_i(0,tokenCount) {
      String animName = GetToken(animList, i + 1, ',');
      if (animName == "Погода") {        
        weatherIndex = i;
        animation_t weather = animations[weatherIndex];
        pictureWidth  = weather.frame_width;
        pictureHeight = weather.frame_height;        
        FOR_j(j, MAX_FRAMES_COUNT) {
          if (weather.frames[j] == NULL) break;
          pictureIndexMax = j + 1;
        }
        break;
      }
    }
    if (pictureIndexMax == 0) {
      DEBUGLN(F("Слайды не найдены"));  
    } else {
      DEBUGLN(F("Слайды не найдены. Будут использованы картинки эффекта 'Погода'"));  
    }
  } else {
    // Печатаем список найденных файлов картинок
    pictureIndexMax = CountTokens(pictureList, ',');
    DEBUG(String(F("Найдено ")) + String(pictureIndexMax) + String(F(" слайд(oв) размером '")));
    DEBUGLN(String(pictureWidth) + "x" + String(pictureHeight) + ":");
    FOR_i(0,pictureIndexMax) {
      DEBUGLN("   " + GetToken(pictureList, i + 1, ',') + ".p");  
    }
  }
  DEBUGLN();

  // Восстановить текущие размеры
  pWIDTH = saveWidth;
  pHEIGHT = saveHeight;
}

bool getNextSlide() {
  bool res = true;
  pictureIndex = random8(0, pictureIndexMax);

  if (pictureStorage.length() == 0) {
    // Работаем с фреймами из эффекта "Погода", т.к не нашли картинок в файловой системе
    animation_t weather = animations[weatherIndex];
    const uint16_t *frame = weather.frames[pictureIndex];
    // Читаем фрейм из анимации "Погода". Фрейм сохранен в виде 16-битных цветов RGB565 - нужно привести к 24-битному RGB888
    FOR_y(0, weather.frame_height) {
      FOR_x (0, weather.frame_width) {                    
        uint16_t clr = (pgm_read_word(&(frame[x + (weather.frame_height - y - 1) * weather.frame_width])));
        CRGB color = gammaCorrection(expandColor(clr));
        picture[x + y * weather.frame_width] = color;
      }
    }      
  } else {
    // Загрузка картинки из файла. В файле цвета уже в 24-битном формате RGB888 - расширения не требуют
    String picFile = GetToken(pictureList, pictureIndex + 1, ',') + ".p";      
    DEBUG(F("Загрузка слайда '"));
    DEBUGLN(picFile + "'");      
    String file = "/" + String(pictureWidth) + "p" + String(pictureHeight) + "/" + picFile;
    String error = openImage(pictureStorage, file, picture, true);
    // Была ошибка при загрузке файла?
    res = error.length() == 0;
  }

  return res;
}

void DrawSlide() {
  CRGB     color, color1, color2;  
  bool     drawBlack, allBlack;
  int8_t   lim, counter;
  
  uint8_t  effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  FastLED.clear();

  // Показ полной картинки
  if (phase == 0) {    
    FOR_y(0, pictureHeight) {
      FOR_x (0, pictureWidth) {      
        idx = x + y * pictureWidth;
        color = picture[idx];
        color = color.nscale8_video(effectBrightness);    
        drawPixelXY(pic_offset_x + x, pic_offset_y + y, color);
      }
    }
    return;    
  }  

  // Фаза 2 (появление) или 4 (скрытие) картинки; loopCounter -- число оставшихся шагов; 
  // Когда loopCounter == 0 - появление или скрытие акартинки завершено
  allBlack = true;

  switch(pic_fade) {

    case 1:  // гориз от центра к краям: Height / 2 шагов для движения
    case 2:  // гориз от края к центру:  Height / 2 шагов для движения
    case 3:  // гориз от верха к низу:   Height шагов
    case 4:  // гориз от низа у верху:   Height шагов
    
      counter = (pic_fade == 1 || pic_fade == 2) ? pictureHeight / 2 : pictureHeight;
      FOR_y(0, counter) {
        FOR_x (0, pictureWidth) {      
          int16_t idx1 = x + y * pictureWidth;
          int16_t idx2 = x + (pictureHeight - y - 1) * pictureWidth;
          if (pic_fade == 2 || pic_fade == 4) {
            lim = counter - loopCounter;
            drawBlack = y >= lim;
          } else {
            // 1 или 3
            lim = loopCounter;
            drawBlack = y < lim;            
          }          
          if (phase == 4) drawBlack = !drawBlack;          
          allBlack &= drawBlack;
          if (y == lim) {
            color1 = CHSV(0,0,128);
            color2 = CHSV(0,0,128);
          } else
          if (drawBlack) {
            color1 = CRGB(0, 0, 0); 
            color2 = CRGB(0, 0, 0); 
          } else {
            color1 = picture[idx1];
            color2 = picture[idx2];
            color1 = color1.nscale8_video(effectBrightness);    
            color2 = color2.nscale8_video(effectBrightness);    
          }
          drawPixelXY(pic_offset_x + x, pic_offset_y + y, color1);
          if (pic_fade == 1 || pic_fade == 2) {
            drawPixelXY(pic_offset_x + x, pic_offset_y + pictureHeight - y - 1, color2);
          }
        }
      }       
      break;

    case 5:  // верт от краев к центру:  Width / 2 шагов для движения от края к центру
    case 6:  // верт от центра к краям:  Width / 2 шагов для движения от края к центру
    case 7:  // верт слева направо -     Width шагов
    case 8:  // верт справа налево -     Width шагов
    
      counter = (pic_fade == 5 || pic_fade == 6) ? pictureWidth / 2 : pictureWidth;
      FOR_x(0, counter) {
        FOR_y (0, pictureHeight) {      
          int16_t idx1 = x + y * pictureWidth;
          int16_t idx2 = pictureWidth - x - 1 + y * pictureWidth;
          if (pic_fade == 6 || pic_fade == 8) {
            lim = counter - loopCounter;
            drawBlack = x >= lim;
          } else {
            // 5 или 7
            lim = loopCounter;
            drawBlack = x < lim;            
          }          
          if (phase == 4) drawBlack = !drawBlack;          
          allBlack &= drawBlack;
          if (x == lim) {
            color1 = CHSV(0,0,128);
            color2 = CHSV(0,0,128);
          } else
          if (drawBlack) {
            color1 = CRGB(0, 0, 0); 
            color2 = CRGB(0, 0, 0); 
          } else {
            color1 = picture[idx1];
            color2 = picture[idx2];
            color1 = color1.nscale8_video(effectBrightness);    
            color2 = color2.nscale8_video(effectBrightness);    
          }
          drawPixelXY(pic_offset_x + x, pic_offset_y + y, color1);
          if (pic_fade == 5 || pic_fade == 6) {
            drawPixelXY(pic_offset_x + pictureWidth - x - 1, pic_offset_y + y, color2);
          }
        }
      }       
      break;    
  }
  if (phase == 4 && allBlack) phase = 1;
}

void slideRoutine() {

  // Если нет картинок для слайдов - переключаемся на режим "Анимация"
  if (pictureIndexMax == 0) {
    DEBUGLN(F("Нет слайдов для отображения."));
    DEBUGLN(F("Переход в режим 'Анимация'."));
    setEffect(MC_IMAGE);
    return;
  }

  //  pic_fade  1 - гориз. от центра к краям; 2 - от краев к центру; 3 - от верха к низу; 4 - от низа к верху
  //            5 - верт. от центра к краям;  6 - от краев к центру; 7 - слева направо; 8 - справа налево
  
  CRGB color;
  
  if (loadingFlag) {
    // modeCode = MC_SLIDE;
    if (picture != NULL) { delete [] picture; }    
    picture = new CRGB[pictureWidth * pictureHeight]; 
    
    // Инициализация эффекта завершилась без ошибок
    loadingFlag = false;
    lastMillis  = millis();
    lastMillisX = millis();
    lastMillisY = millis();
    
    phase = 1;                                     // фаза эффекта 0 -  показ картинки; 1 - настройка "появления" картинки;  2 - появление картинки 3 - настройка скрытия картинки 4 - "скрытие" картинки
    pic_fade = random8(0,100) % 8 + 1;             // 1 - гориз. от центра к краям; 2 - от краев к центру; 3 - от верха к низу; 4 - от низа к верху
                                                   // 5 - верт. от центра к краям;  6 - от краев к центру; 7 - слева направо; 8 - справа налево    
    FastLED.clear();
  }  

  uint8_t changeDelay = map8(255 - getEffectScaleParamValue(thisMode),1,30);
  if (abs((long long)(millis() - lastMillis)) > 1000UL * changeDelay) {
    lastMillis = millis();
    // Подошло время смены слайда и мы находимся в фазе "покоя" - перейти к фазе подготовки скрытия картинки
    phase = phase == 0 ? 3 : 1;
  }  
  bool nextSlide = phase == 1;  // Фаза 1 - начать отображение нового слайда
  bool changeSlide = nextSlide;
  
  // Время показа слайда вышло - заменить на следующий
  // Если слайд загрузить не удалось - перейти в режим "Анимация"
  if (nextSlide) {
    if (!getNextSlide()) {
      DEBUGLN(F("Переход в режим 'Анимация'."));
      setEffect(MC_IMAGE);
      return;
    }
  }
  
  if (phase == 1) {
    pic_fade = random8(0,100) % 8 + 1;
    switch(pic_fade) {
      case 1:  loopCounter = pictureHeight / 2;  break;      // гориз от краев к центру: Height / 2 шагов для движения от края к центру
      case 2:  loopCounter = pictureHeight / 2;  break;      // гориз от цетра к краям:  Height / 2 шагов для движения от края к центру
      case 3:  loopCounter = pictureHeight;      break;      // гориз от верха к низу:   Height шагов
      case 4:  loopCounter = pictureHeight;      break;      // гориз от низа у верху:   Height шагов
      case 5:  loopCounter = pictureWidth / 2;   break;      // верт от краев к центру:  Width / 2 шагов для движения от края к центру
      case 6:  loopCounter = pictureWidth / 2;   break;      // верт от центра к краям:  Width / 2 шагов для движения от края к центру
      case 7:  loopCounter = pictureWidth;       break;      // верт слева направо -     Width шагов
      case 8:  loopCounter = pictureWidth;       break;      // верт справа налево -     Width шагов
    }    
    phase = 2;
  }
  
  if (phase == 2) {
    // Фаза появления картинки
    changeSlide = true;
    loopCounter--;    
    
    // Появление картинки завершено. Перейти к файзе покоя картинки
    if (loopCounter < 0) {
      loopCounter = 0;
      phase = 0;
    }
  }
  
  if (phase == 3) {
    // настройка скрытия картинки - выбрать способ
    // pic_fade = random8(0,100) % 9;
    pic_fade = random8(0,100) % 8 + 1;
    switch(pic_fade) {
      case 1:  loopCounter = pictureHeight / 2;  break;  // гориз от краев к центру: Height / 2 шагов для движения от края к центру
      case 2:  loopCounter = pictureHeight / 2;  break;  // гориз от цетра к краям:  Height / 2 шагов для движения от края к центру
      case 3:  loopCounter = pictureHeight;      break;  // гориз от верха к низу:   Height шагов
      case 4:  loopCounter = pictureHeight;      break;  // гориз от низа у верху:   Height шагов
      case 5:  loopCounter = pictureWidth / 2;   break;  // верт от краев к центру:  Width / 2 шагов для движения от края к центру
      case 6:  loopCounter = pictureWidth / 2;   break;  // верт от центра к краям:  Width / 2 шагов для движения от края к центру
      case 7:  loopCounter = pictureWidth;       break;  // верт слева направо -     Width шагов
      case 8:  loopCounter = pictureWidth;       break;  // верт справа налево -     Width шагов
    }    
    phase = 4;
  }
  
  if (phase == 4) {
    // фаза скрытия картинки
    changeSlide = true;
    loopCounter--;
    
    // Скрытие картинки завершено. Перейти к файзе показа следующей картинки
    if (loopCounter < 0) {
      loopCounter = 0;
      phase = 1;
    }
  } 

  // Отисовка полного кадра и смещение по полю матрицы
  // Ползунок "Вариант" регулирует скорость перемещения картинки слайда по матрице, если размер слайда меньше размера матрицы
  
  // Ползунок "Скорость" регулирует скорость перемещения картинки слайда по матрице, если размер слайда меньше размера матрицы
  // Если ползунок в крайнем левом положении - картинка не движется, стоит по центру матрицы.
  uint8_t moveDelay = getEffectSpeedValue(thisMode);
  if (moveDelay > 240) {
    pic_offset_x = (pWIDTH - pictureWidth) / 2;
    pic_offset_y = (pHEIGHT - pictureHeight) / 2; 
    dir_x = 0;
    dir_y = 0;   
    changeSlide = true;
  } else {
    if (pictureWidth >= pWIDTH) {
      pic_offset_x = (pWIDTH - pictureWidth) / 2;
      dir_x = 0;
    }
    if (pictureHeight >= pHEIGHT) {
      pic_offset_y = (pHEIGHT - pictureHeight) / 2; 
      dir_y = 0;   
    }
    moveDelay = map8(moveDelay, 5, 200);
    move_delay_x = pWIDTH >= pHEIGHT ? moveDelay * 10 : moveDelay * 10 / 3 * 2;
    move_delay_y = pWIDTH >= pHEIGHT ? moveDelay * 10 / 3 * 2 : moveDelay * 10 ;    
    if (pWIDTH - pictureWidth > 6 && dir_x == 0) dir_x = 1;
    if (pHEIGHT - pictureHeight > 6 && dir_y == 0) dir_y = 1;
    if (dir_x != 0 && millis() - lastMillisX > move_delay_x) {
      lastMillisX = millis();
      changeSlide = true;
      pic_offset_x += dir_x;
      if (dir_x > 0 && pic_offset_x + pictureWidth >= pWIDTH) {
        pic_offset_x = pWIDTH - pictureWidth;
        dir_x = -1;
      } else
      if (dir_x < 0 && pic_offset_x < 0) {
        pic_offset_x = 0;
        dir_x = 1;
      }
    }
    if (dir_y != 0 && millis() - lastMillisY > move_delay_y) {
      lastMillisY = millis();
      changeSlide = true;
      pic_offset_y += dir_y;
      if (dir_y > 0 && pic_offset_y + pictureHeight >= pHEIGHT) {
        pic_offset_y = pHEIGHT - pictureHeight;
        dir_y = -1;
      } else
      if (dir_y < 0 && pic_offset_y < 0) {
        pic_offset_y = 0;
        dir_y = 1;
      }
    }
  }

  // Параметр "Вариант"
  // getEffectScaleParamValue2(thisMode)
  
  // Если сменился слайд или позиция отображения слайда - перерисовать его на матрице
  if (changeSlide) {    
    DrawSlide();
    if (phase != 0) delay(20);
  }
}

void slideRoutineRelease() {
  if (picture   != NULL) { delete [] picture; picture = NULL; }  
}
