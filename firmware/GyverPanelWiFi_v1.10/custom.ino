
// ----------------------------------------------------

byte lastOverlayX, lastOverlayY, lastOverlayW, lastOverlayH;
unsigned long xxx;

void customRoutine(byte aMode) {
  doEffectWithOverlay(aMode); 
}

void doEffectWithOverlay(byte aMode) {

  bool clockReady = clockTimer.isReady();
  bool textReady = textTimer.isReady();
  
  bool effectReady = aMode == MC_IMAGE || effectTimer.isReady(); // "Анимация" использует собственные "таймеры" для отрисовки - отрисовка без задержек; Здесь таймер опрашивать нельзя - он после опроса сбросится. 
                                                                 // А должен читаться в эффекте анимации, проверяя не пришло ли время отрисовать эффект фона

  if (!(effectReady || (clockReady && !showTextNow) || (textReady && (showTextNow || thisMode == MC_TEXT)))) return;

  // В прошлой итерации часы / текст были наложены с оверлеем?
  // Если да - восстановить пиксели эффекта сохраненные перед наложением часов / текста
  if (overlayDelayed && thisMode != MC_DRAW && thisMode != MC_LOADIMAGE) {
    overlayUnwrap();
    overlayDelayed = false;
  }  

  // Проверить есть ли активное событие текста? 
  // Если нет - после проверки  momentTextIdx = -1 и momentIdx = -1
  // Если есть - momentTextIdx - индекс текста для вывода в зависимости от ДО или ПОСЛЕ события текущее время; momentIdx - активная позиция в массиве событий moments[] 
  if (init_time) {
    checkMomentText();
    if (momentTextIdx >= 0 && currentTextLineIdx != momentTextIdx) {
      // В момент смены стоки с ДО на ПОСЛЕ - строка ПОСЛЕ извлеченная из массива содержит признак отключенности - '-' в начале или "{-}" в любом месте
      // Также строка может содержать другие макросы, которые нужно обработать processMacrosInText()
      // Если передать строку с макросом отключения - processMacrosInText() вернет любую другую строку вместо отключенной
      // Чтобы это не произошло - нужно удалить признак отключенности
      currentTextLineIdx = momentTextIdx;
      String text = textLines[currentTextLineIdx];
      if (text.length() > 0 && text[0] == '-') text = text.substring(1);
      while (text.indexOf("{-}") >= 0) text.replace("{-}","");
      currentText = processMacrosInText(text);      
      ignoreTextOverlaySettingforEffect = textOverlayEnabled;
      loadingTextFlag = true;
    }

    #if (USE_MQTT == 1)
    // Если подошло время отправки uptime на MQTT-сервер - отправить
    if (init_time && upTime > 0 && upTimeSendInterval > 0 && (millis() - uptime_send_last > upTimeSendInterval * 1000L )) {      
      // Некоторые сервера пропускают самый первый отправленный "online" сразу после соединения с брокером
      // Подключающиеся устройства получают статус "offline" и приложение не может соединиться с устройством, считая, что оно выключено.
      // Посылаем статус "online" по таймеру вместе с периодической отправкой времени uptime (времени бесперебойной работы)
      putOutQueue(mqtt_topic(TOPIC_PWR).c_str(), "online", true);
      addKeyToChanged("UP");
    }    
    #endif    
  }

  // Оверлей нужен для всех эффектов, иначе при малой скорости эффекта и большой скорости часов поверх эффекта буквы-цифры "смазываются"
  bool textOvEn  = ((textOverlayEnabled && (getEffectTextOverlayUsage(aMode))) || ignoreTextOverlaySettingforEffect) && !isTurnedOff && !isNightClock && thisMode < MAX_EFFECT && thisMode != MC_CLOCK;
  bool clockOvEn = clockOverlayEnabled && getEffectClockOverlayUsage(aMode) && thisMode != MC_CLOCK && thisMode != MC_DRAW && thisMode != MC_LOADIMAGE;
  bool needStopText = false;
  String out;

  needStopText = false;

  // Если команда отображения текущей строки передана из приложения или
  // Если есть активная строка, связанная с текущим отслеживаемым по времени событием или
  // Если пришло время отображения очередной бегущей строки поверх эффекта
  // Переключиться в режим бегущей строки оверлеем
  if (!showTextNow && textOvEn && thisMode != MC_TEXT && (momentTextIdx >= 0 || ignoreTextOverlaySettingforEffect || ((millis() - textLastTime) > (TEXT_INTERVAL  * 1000L)))) {

    // Обработать следующую строку для отображения, установить параметры;
    // Если нет строк к отображению - продолжать отображать оверлей часов
    if (prepareNextText(currentText)) {
      moment_active = momentTextIdx >= 0;
      fullTextFlag = false;
      loadingTextFlag = false;
      showTextNow = true;                  // Флаг переключения в режим текста бегущей строки 
      textCurrentCount = 0;                // Сбросить счетчик количества раз, сколько строка показана на матрице;
      textStartTime = millis();            // Запомнить время начала отображения бегущей строки

      #if (USE_MQTT == 1)
        String outText;
        DynamicJsonDocument doc(256);
        doc["act"] = F("TEXT");
        doc["run"] = true;
        if (textHasDateTime) {
          outText = processDateMacrosInText(currentText);         // Обработать строку, превратив макросы даты в отображаемые значения
          if (outText.length() == 0) {                            // Если дата еще не инициализирована - вернет другую строку, не требующую даты
            outText = currentText;
          }      
        } else {
          outText = currentText;
        }
        doc["text"] = outText;
        serializeJson(doc, out);    
        SendMQTT(out, TOPIC_TXT);
      #endif

      #if (USE_MP3 == 1)
      if (runTextSound >= 0) {
        if (isDfPlayerOk && noteSoundsCount > 0) {
          dfPlayer.stop();
          delay(10);
          dfPlayer.volume(constrain(maxAlarmVolume,1,30));
          dfPlayer.playFolder(3, runTextSound);
          /*
          if (runTextSoundRepeat)
            dfPlayer.enableLoop(); // Не срабатывае т :( Повтор перенесен на событие "Проигрывание файла завершено)
          else    
            dfPlayer.disableLoop();
          */  
        } else {
          runTextSound = -1;
          runTextSoundRepeat = false;
        }
      }        
      #endif      
    }

    // Если указано, что строка должна отображаться на фоне конкретного эффекта - его надо инициализировать    
    if (specialTextEffect >= 0) {
      saveEffectBeforeText = thisMode;   // сохранить текущий эффект
      setTimersForMode(specialTextEffect);
      // Если заказанный эффект не тот же, что сейчас воспроизводится или если эффект имеет вариант - выполнить инициализацию эффекта
      loadingFlag = (specialTextEffect != saveEffectBeforeText) || (specialTextEffectParam >=0 && getParam2ForMode(specialTextEffect).charAt(0) != 'X');
    }
  } else

  // Если строка отображается, но флаг разрешения сняли - прекратить отображение
  if (showTextNow && !textOvEn) {
    ignoreTextOverlaySettingforEffect = false;
    needStopText = true;
  } else 
  
  // Если находимся в режиме бегущей строки и строка полностью "прокручена" или сняли разрешение на отображение бегущей строки - переключиться в режим отображения часов оверлеем
  // С учетом того, показана ли строка нужное количество раз или указанную продолжительность по времени
  if (showTextNow && fullTextFlag && !moment_active) {  
    if (textShowTime > 0) {
      // Показ строки ограничен по времени
      if ((millis() - textStartTime) > (textShowTime * 1000UL)) {
        needStopText = true;
      }
    } else {
      // Ограничение по количеству раз отображения строки
      // Увеличить счетчик показа бегущей строки; Сколько раз нужно показать - инициализируется в textShowCount во время получения и обработки очередной бегущей строки
      textCurrentCount++;                  
      // Если текст уже показан нужное количество раз - переключиться в режим отображения часов поверх эффектов
      if (textCurrentCount >= textShowCount) {
        needStopText = true;
      } else {
        fullTextFlag = false;
      }
    }
  }

  // Нет активного события? но флаг что оно отображается стоит
  if (moment_active && momentTextIdx < 0) {
    moment_active = false;
    needStopText = true;
    rescanTextEvents();
  }

  // Нужно прекратить показ текста бегущей строки
  if (needStopText) {    
    showTextNow = false; 
    currentText = "";
    ignoreTextOverlaySettingforEffect = nextTextLineIdx >= 0;
    specialTextEffectParam = -1;

    // Если строка показывалась на фоне специального эффекта для строки или специальной однотонной заливки - восстановить эффект, который был до отображения строки
    if (saveEffectBeforeText >= 0 || useSpecialBackColor) {
      loadingFlag = specialTextEffect != saveEffectBeforeText || useSpecialBackColor;  // Восстановленный эффект надо будет перезагрузить, т.к. иначе эффекты с оверлеем будут использовать оставшийся от спецэффекта/спеццвета фон
      saveEffectBeforeText = -1;                                                       // Сбросить сохраненный / спецэффект
      specialTextEffect = -1;      
      useSpecialBackColor = false;
    }

    // Текст мог быть на фоне другого эффекта, у которого свой таймер.
    // После остановки отображения текста на фоне эффекта, установить таймер текущего эффекта
    setTimersForMode(thisMode);
    
    // Если к показы задана следующая строка - установить время показа предыдущей в 0, чтобы
    // следующая строка начала показываться немедленно, иначе - запомнить время окончания показа строки,
    // от которого отсчитывается когда начинать следующий показ
    textLastTime = nextTextLineIdx >= 0 ? 0 : millis();

    #if (USE_MP3 == 1)
      // Если воспроизводился звук указанный для строки - остановить его
      if (runTextSound >= 0) {
        runTextSound = -1;
        runTextSoundRepeat = false;
        dfPlayer.stop();
      }
    #endif

    #if (USE_MQTT == 1)
      DynamicJsonDocument doc(256);
      doc["act"] = F("TEXT");
      doc["run"] = false;
      serializeJson(doc, out);    
      SendMQTT(out, TOPIC_TXT);
    #endif

    // Если показ завершен и к отображению задана следующая строка - не нужно рисовать эффекты и все прочее - иначе экран мелькает
    // Завершить обработку - на следующем цикле будет выполнен показ следующей строки
    if (nextTextLineIdx >= 0) {
      return;
    }
  }

  // Нужно сохранять оверлей эффекта до отрисовки часов или бегущей строки поверх эффекта?
  bool needOverlay  = 
       (aMode == MC_CLOCK) ||                                                         // Если включен режим "Часы" (ночные часы)
       (aMode == MC_TEXT) ||                                                          // Если включен режим "Бегущая строка" (show IP address)       
      (!showTextNow && clockOvEn) || 
       (showTextNow && textOvEn);
    
  if (effectReady) {
    if (showTextNow) {
      // Если указан другой эффект, поверх которого бежит строка - отобразить его
      if (specialTextEffect >= 0) {
        processEffect(specialTextEffect);
      } else if (useSpecialBackColor) {
        // Задана отрисовка строки поверх однотонной заливки указанным цветом
        fillAll(specialBackColor);
        overlayDelayed = false;
      } else {
        // Отобразить текущий эффект, поверх которого будет нарисована строка
        processEffect(aMode);
      }
    } else {
      // Иначе отрисовать текущий эффект
      processEffect(aMode);
    }
  }

  // Смещение бегущей строки
  if (textReady) {
    // Сдвинуть позицию отображения бегущей строки
    shiftTextPosition();
  }

  // Смещение движущихся часов 
  if (clockReady) {    
    CLOCK_MOVE_CNT--;
    if (CLOCK_MOVE_CNT <= 0) {
      CLOCK_MOVE_CNT = CLOCK_MOVE_DIVIDER;
      if (showDateInClock && showDateState && !showWeatherState) {
        CALENDAR_XC--;
        if (CALENDAR_XC < -CALENDAR_W) {
         #if (DEVICE_TYPE == 0 && CLOCK_W < WIDTH)
           CALENDAR_XC = WIDTH - CALENDAR_W - 1 ;
         #else
           CALENDAR_XC = WIDTH - 1;
         #endif  
        }     
        CLOCK_XC = CALENDAR_XC + (CLOCK_W - CALENDAR_W) / 2;
      } else {
        CLOCK_XC--;
        if (getClockX(CLOCK_XC + CLOCK_LX) < 0) {
         #if (DEVICE_TYPE == 0 && CLOCK_W < WIDTH)
           CLOCK_XC = WIDTH - CLOCK_W - 1;
         #else
           CLOCK_XC = WIDTH - CLOCK_FX - 1;
         #endif  
        }     
        CALENDAR_XC = CLOCK_XC + (CALENDAR_W - CLOCK_W) / 2;
      }
    }
  }

  // Пришло время отобразить дату (календарь) в малых часах или температуру / календарь в больших?
  checkCalendarState();
  
  // Если время инициализировали и пришло время его показать - нарисовать часы поверх эффекта
  if (init_time && ((clockOvEn && !showTextNow && aMode != MC_TEXT && thisMode != MC_DRAW && thisMode != MC_LOADIMAGE) || aMode == MC_CLOCK)) {    
    overlayDelayed = needOverlay;
    setOverlayColors();
    if (needOverlay) {
      // Размер календаря по высоте имеет максимальный оверлей, в который входит и бегущая строка и часы с температурой в две строки и сам календарь
      y_overlay_low  = CALENDAR_Y - 1;
      y_overlay_high = y_overlay_low + CALENDAR_H;
      while (y_overlay_low < 0) y_overlay_low++;
      while (y_overlay_high >= HEIGHT) y_overlay_high--;
      overlayWrap();
    }      

    // Время отрисовки календаря или температуры
    bool cal_or_temp_processed = false;
    if (showDateState && (showDateInClock || (!allow_two_row && (init_weather && showWeatherInClock && showWeatherState)))) {
      if (showDateInClock && showDateState && !showWeatherState) {
        // Календарь
        drawCalendar(aday, amnth, ayear, dotFlag, CALENDAR_XC, CALENDAR_Y);
        cal_or_temp_processed = true;
      } else {
        // Температура, когда чередуется с часами - только при горизонтальной ориентации часов и если она по высоте не входит в отображение ВМЕСТЕ с часами
        #if (USE_WEATHER == 1)       
          if (init_weather && showWeatherInClock && showDateState && showWeatherState && CLOCK_ORIENT == 0 && !allow_two_row) {
            CLOCK_WY = CLOCK_Y;
            drawTemperature(CLOCK_XC);
            cal_or_temp_processed = true;
          } else {   
            // Если показ календаря в часах включен - показать кадлендарь, иначе - вместо календаря снова показать температуру, если она включена         
            if (showDateInClock || !init_weather) {
              drawCalendar(aday, amnth, ayear, dotFlag, CLOCK_XC, CALENDAR_Y);  // В больших часах календарь и температура показываются в той же позиции, что и часы и совпадают по формату - ЧЧ:MM и ДД.MM - одинаковый размер
              cal_or_temp_processed = true;
            } else if (showWeatherInClock && !allow_two_row) {
              CLOCK_WY = CLOCK_Y;
              drawTemperature(CLOCK_XC);  
              cal_or_temp_processed = true;
            }
          }
        #else
           drawCalendar(aday, amnth, ayear, dotFlag, CLOCK_XC, CALENDAR_Y);  // В больших часах календарь и температура показываются в той же позиции, что и часы
           cal_or_temp_processed = true;
        #endif
      }
    } 

    // Если календарь или температура по условиям не могут быть нарисованы - рисовать часы
    if (!cal_or_temp_processed) {

      byte CLK_Y = CLOCK_Y;

      #if (USE_WEATHER == 1)       
        // Если температура отрисовывается вместе с часами - позийия рисования такая же как у двухстрочного календаря
        bool draw_temp = init_weather && showWeatherInClock && allow_two_row && CLOCK_ORIENT == 0;
        if (draw_temp) {
          CLK_Y = CALENDAR_Y + (c_size == 1 ? 6 : 9);
          while (CLK_Y + (c_size == 1 ? 5 : 7) > HEIGHT) CLK_Y--;
        }
      #endif
      
      drawClock(hrs, mins, dotFlag, CLOCK_XC, CLK_Y);

      #if (USE_WEATHER == 1)       
        if (draw_temp) {
          CLOCK_WY = CALENDAR_Y - 1;
          while (CLOCK_WY < 0) CLOCK_WY++;
          drawTemperature(CLOCK_XC);
        }
      #endif
    }
    
  } else if (showTextNow && aMode != MC_CLOCK && aMode != MC_TEXT) {   // MC_CLOCK - ночные/дневные часы; MC_TEXT - показ IP адреса - всё на черном фоне
    // Нарисовать оверлеем текст бегущей строки
    // Нарисовать текст в текущей позиции
    overlayDelayed = needOverlay;
    if (needOverlay) {
      y_overlay_low  = getTextY() - 2;                      // Нижняя строка вывода строки текста -2 строки на подстрочные диакритич символы
      y_overlay_high = y_overlay_low + LET_HEIGHT + 4;      // Высота букв +3 символа на диакритичексие надстрочныесимволы
      if (y_overlay_low < 0) y_overlay_low = 0;
      if (y_overlay_high >= HEIGHT) y_overlay_high = HEIGHT - 1;
      overlayWrap();
    }
    runningText();
  }

  FastLED.show();
}

void processEffect(byte aMode) {
  switch (aMode) {
    case MC_NOISE_MADNESS:       madnessNoise(); break;
    case MC_NOISE_CLOUD:         cloudNoise(); break;
    case MC_NOISE_LAVA:          lavaNoise(); break;
    case MC_NOISE_PLASMA:        plasmaNoise(); break;
    case MC_NOISE_RAINBOW:       rainbowNoise(); break;
    case MC_PAINTBALL:           lightBallsRoutine(); break;
    case MC_NOISE_RAINBOW_STRIP: rainbowStripeNoise(); break;
    case MC_NOISE_ZEBRA:         zebraNoise(); break;
    case MC_NOISE_FOREST:        forestNoise(); break;
    case MC_NOISE_OCEAN:         oceanNoise(); break;
    case MC_SNOW:                snowRoutine(); break;
    case MC_SPARKLES:            sparklesRoutine(); break;
    case MC_CYCLON:              cyclonRoutine(); break;
    case MC_FLICKER:             flickerRoutine(); break;
    case MC_PACIFICA:            pacificaRoutine(); break;
    case MC_SHADOWS:             shadowsRoutine(); break;
    case MC_MATRIX:              matrixRoutine(); break;
    case MC_STARFALL:            starfallRoutine(); break;
    case MC_BALL:                ballRoutine(); break;
    case MC_BALLS:               ballsRoutine(); break;
    case MC_RAINBOW:             rainbowRoutine(); break;      // rainbowHorizontal(); // rainbowVertical(); // rainbowDiagonal(); // rainbowRotate();
    case MC_FIRE:                fireRoutine(); break;
    case MC_FILL_COLOR:          fillColorProcedure(); break;
    case MC_COLORS:              colorsRoutine(); break;
    case MC_LIGHTERS:            lightersRoutine(); break;
    case MC_SWIRL:               swirlRoutine(); break;
    case MC_MAZE:                mazeRoutine(); break;
    case MC_SNAKE:               snakeRoutine(); break;
    case MC_TETRIS:              tetrisRoutine(); break;
    case MC_ARKANOID:            arkanoidRoutine(); break;
    case MC_PALETTE:             paletteRoutine(); break;
    case MC_MUNCH:               munchRoutine(); break;
    case MC_ANALYZER:            analyzerRoutine(); break;
    case MC_PRIZMATA:            prizmataRoutine(); break;
    case MC_RAIN:                rainRoutine(); break;
    case MC_FIRE2:               fire2Routine(); break;
    case MC_WATERFALL:           waterfallRoutine(); break;
    case MC_IMAGE:               animationRoutine(); break;
    case MC_ARROWS:              arrowsRoutine(); break;
    case MC_WEATHER:             weatherRoutine(); break;
    case MC_LIFE:                lifeRoutine(); break;
    case MC_TEXT:                runningText(); break;
    case MC_CLOCK:               clockRoutine(); break;
    case MC_DAWN_ALARM:          dawnProcedure(); break;
    case MC_PATTERNS:            patternRoutine(); break;

    #if (USE_SD == 1)
    case MC_SDCARD:              sdcardRoutine(); break;
    #endif

    // Спец.режимы так же как и обычные вызываются в customModes (MC_DAWN_ALARM_SPIRAL и MC_DAWN_ALARM_SQUARE)
    case MC_DAWN_ALARM_SPIRAL:   dawnLampSpiral(); break;
    case MC_DAWN_ALARM_SQUARE:   dawnLampSquare(); break;
  }
}

// ********************* ОСНОВНОЙ ЦИКЛ РЕЖИМОВ *******************

static void nextMode() {
#if (SMOOTH_CHANGE == 1)
  fadeMode = 0;
  modeDir = true;
#else
  nextModeHandler();
#endif
}

static void prevMode() {
#if (SMOOTH_CHANGE == 1)
  fadeMode = 0;
  modeDir = false;
#else
  prevModeHandler();
#endif
}

void nextModeHandler() {

  if (useRandomSequence) {
    setRandomMode2();
    return;
  }

  byte aCnt = 0;
  int8_t curMode = thisMode, newMode = thisMode;

  while (aCnt < MAX_EFFECT) {
    // Берем следующий режим по циклу режимов
    // Если режим - SD-карта и установлено последовательное воспроизведение файлов - брать следующий файл с SD-карты
    #if (USE_SD == 1)
      if (newMode == MC_SDCARD && effectScaleParam2[MC_SDCARD] == 1) {
        if (sf_file_idx == -2 || sf_file_idx == (MAX_FILES + 1)) sf_file_idx = 0;
        else sf_file_idx++;
        if (sf_file_idx >= countFiles) {
          sf_file_idx = MAX_FILES + 1;
          aCnt++;
          newMode++;
        }
      } else {
        aCnt++;
        newMode++;
        if (newMode >= MAX_EFFECT) newMode = 0;  
        if (newMode == MC_SDCARD && getEffectUsage(newMode) && effectScaleParam2[MC_SDCARD] == 1) {
          if (sf_file_idx == -2 || sf_file_idx == (MAX_FILES + 1)) sf_file_idx = 0;
          else sf_file_idx++;
          if (sf_file_idx >= countFiles) sf_file_idx = 0;
        }        
      }
    #else
      aCnt++;
      newMode++;
    #endif
    if (newMode >= MAX_EFFECT) newMode = 0;    
    // Если новый режим отмечен флагом "использовать" - используем его, иначе берем следующий (и проверяем его)
    if (getEffectUsage(newMode)) break;    
    // Если перебрали все и ни у одного нет флага "использовать" - не обращаем внимание на флаг, используем следующий
    if (aCnt >= MAX_EFFECT) {
      newMode = curMode++;
      if (newMode >= MAX_EFFECT) newMode = 0;
      break;
    }
  }

  set_thisMode(newMode);  
  
  loadingFlag = true;
  autoplayTimer = millis();
  setTimersForMode(thisMode);
  
  FastLED.clear();
  FastLED.setBrightness(globalBrightness);
}

void prevModeHandler() {

  if (useRandomSequence) {
    setRandomMode2();
    return;
  }

  byte aCnt = 0;
  int8_t curMode = thisMode, newMode = thisMode;

  while (aCnt < MAX_EFFECT) {
    // Берем предыдущий режим по циклу режимов
    // Если режим - SD-карта и установлено последовательное воспроизведение файлов - брать предыдущий файл с SD-карты
    #if (USE_SD == 1)
      if (newMode == MC_SDCARD && effectScaleParam2[MC_SDCARD] == 1) {
        if (sf_file_idx == -2 || sf_file_idx == (MAX_FILES + 1)) sf_file_idx = countFiles - 1;
        else sf_file_idx--;
        if (sf_file_idx < 0) {
          sf_file_idx = -2;
          aCnt++;
          newMode--;
        }
      } else {
        aCnt++;
        newMode--;
        if (newMode < 0) newMode = MAX_EFFECT - 1;
        if (newMode == MC_SDCARD && getEffectUsage(newMode) && effectScaleParam2[MC_SDCARD] == 1) {
          if (sf_file_idx == -2 || sf_file_idx == (MAX_FILES + 1)) sf_file_idx = countFiles - 1;
          else sf_file_idx--;
          if (sf_file_idx < 0) sf_file_idx = countFiles - 1;
        }        
      }
    #else
      aCnt++;
      newMode--;
    #endif
    if (newMode < 0) newMode = MAX_EFFECT - 1;
    // Если новый режим отмечен флагом "использовать" - используем его, иначе берем следующий (и проверяем его)
    if (getEffectUsage(newMode)) break;    
    // Если перебрали все и ни у одного нет флага "использовать" - не обращаем внимание на флаг, используем предыдущий
    if (aCnt >= MAX_EFFECT) {
      newMode = curMode--;
      if (newMode < 0) newMode = MAX_EFFECT - 1;
      break;
    }
  }
  
  set_thisMode(newMode);  

  loadingFlag = true;
  autoplayTimer = millis();
  setTimersForMode(thisMode);
  
  FastLED.clear();
}

void setTimersForMode(byte aMode) {

  if (!(aMode == MC_TEXT || aMode == MC_CLOCK)) {
    effectSpeed = getEffectSpeed(aMode);
    if (effectSpeed == 0) effectSpeed = 1;
    // Эти режимы смотрятся (работают) только на максимальной скорости;
    if (aMode == MC_PAINTBALL || aMode == MC_SWIRL || aMode == MC_FLICKER || aMode == MC_PACIFICA || 
        aMode == MC_SHADOWS || aMode == MC_PRIZMATA || aMode == MC_FIRE2 || aMode == MC_WATERFALL || 
        aMode == MC_IMAGE || aMode == MC_WEATHER || aMode == MC_LIFE || aMode == MC_ARKANOID || aMode == MC_TETRIS) {      
      if (aMode == MC_TETRIS) {
        effectTimer.setInterval(50);
        gameTimer.setInterval(200 + 4 * effectSpeed);
      } else
      if (aMode == MC_ARKANOID) {
        effectTimer.setInterval(50);
        gameTimer.setInterval(effectSpeed);  
      } else {
        effectTimer.setInterval(10);
      }
    } else if (aMode == MC_MAZE) {
      effectTimer.setInterval(50 + 3 * effectSpeed);              
    }
    else
      effectTimer.setInterval(effectSpeed);
  } else if (aMode == MC_CLOCK) {
      effectTimer.setInterval(250);
  }
    
  set_clockScrollSpeed(getClockScrollSpeed());
  if (clockScrollSpeed < D_CLOCK_SPEED_MIN) set_clockScrollSpeed(D_CLOCK_SPEED_MIN); // Если clockScrollSpeed == 0 - бегущая строка начинает дергаться.
  if (clockScrollSpeed > D_CLOCK_SPEED_MAX) set_clockScrollSpeed(D_CLOCK_SPEED_MAX);
  if (clockScrollSpeed >= 240) {
    clockTimer.setInterval(4294967295);
    checkClockOrigin();
  } else {
    clockTimer.setInterval(clockScrollSpeed);
  }

  set_textScrollSpeed(getTextScrollSpeed());
  if (textScrollSpeed < D_TEXT_SPEED_MIN) set_textScrollSpeed(D_TEXT_SPEED_MIN); // Если textScrollSpeed == 0 - бегущая строка начинает дергаться.
  if (textScrollSpeed > D_TEXT_SPEED_MAX) set_textScrollSpeed(D_TEXT_SPEED_MAX);
  textTimer.setInterval(textScrollSpeed);
}

int fadeBrightness;
int fadeStepCount = 10;     // За сколько шагов убирать/добавлять яркость при смене режимов
int fadeStepValue = 5;      // Шаг убавления яркости

#if (SMOOTH_CHANGE == 1)
void modeFader() {
  if (fadeMode == 0) {
    fadeMode = 1;
    fadeStepValue = fadeBrightness / fadeStepCount;
    if (fadeStepValue < 1) fadeStepValue = 1;
  } else if (fadeMode == 1) {
    if (changeTimer.isReady()) {
      fadeBrightness -= fadeStepValue;
      if (fadeBrightness < 0) {
        fadeBrightness = 0;
        fadeMode = 2;
      }
      FastLED.setBrightness(fadeBrightness);
    }
  } else if (fadeMode == 2) {
    fadeMode = 3;
    if (modeDir) nextModeHandler();
    else prevModeHandler();
  } else if (fadeMode == 3) {
    if (changeTimer.isReady()) {
      fadeBrightness += fadeStepValue;
      if (fadeBrightness > globalBrightness) {
        fadeBrightness = globalBrightness;
        fadeMode = 4;
      }
      FastLED.setBrightness(fadeBrightness);
    }
  }
}
#endif

void checkIdleState() {

#if (SMOOTH_CHANGE == 1)
  modeFader();
#endif
  
  if (idleState) {
    unsigned long ms = millis();
    if ((ms - autoplayTimer > autoplayTime) && !manualMode) {    // таймер смены режима
      bool ok = true;
      if (
         (thisMode == MC_TEXT     && !fullTextFlag) ||   // Эффект "Бегущая строка" (показать IP адрес) не сменится на другой, пока вся строка не будет показана полностью
      // (showTextNow && !fullTextFlag)             ||   // Если нужно чтобы эффект не менялся, пока не пробежит вся строка оверлеем - раскомментарить эту строку
         (thisMode == MC_MAZE     && !gameOverFlag) ||   // Лабиринт не меняем на другой эффект, пока игра не закончится (не выйдем из лабиринта)
      // (thisMode == MC_SNAKE    && !gameOverFlag) ||   // Змейка долгая игра - не нужно дожидаться окончания, можно прервать
         (thisMode == MC_TETRIS   && !gameOverFlag) ||   // Тетрис не меняем на другой эффект, пока игра не закончится (стакан не переполнится)
      // (thisMode == MC_ARKANOID && !gameOverFlag) ||   // Арканоид долгая игра - не нужно дожидаться окончания, можно прервать
         (showTextNow && (specialTextEffect >= 0))       // Воспроизводится бегущая строка на фоне указанного эффекта
         #if (USE_SD == 1)
         // Для файла с SD-карты - если указан режим ожидания проигрывания aaqkf до концв, а файл еще не проигрался - не менять эффект
         || (thisMode == MC_SDCARD && wait_play_finished && !play_file_finished)
         #endif
      )
      {        
        // Если бегущая строка или игра не завершены - смены режима не делать
        ok = false;
      } 

      // Смена режима разрешена
      if (ok) {
        // Если режим не игра и не бегущая строка или один из этих режимов и есть флаг завершения режима -
        // перейти к следующему режиму; если режим был - бегущая строка - зафиксировать время окончания отображения последней бегущей строки        
        autoplayTimer = millis();
        nextMode();
      }
    }
  } else {
    if (idleTimer.isReady()) {      // таймер холостого режима. Если время наступило - включить автосмену режимов 
      setManualModeTo(false);
      nextMode();
    }
  }  
}
