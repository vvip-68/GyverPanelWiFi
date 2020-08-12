// ************************ НАСТРОЙКИ ************************

byte lastOverlayX, lastOverlayY, lastOverlayW, lastOverlayH;
unsigned long xxx;

// ************************* СВОЙ СПИСОК РЕЖИМОВ ************************
// список можно менять, соблюдая его структуру. Можно удалять и добавлять эффекты, ставить их в
// любой последовательности или вообще оставить ОДИН. Удалив остальные case и break. Cтруктура оч простая:
// case <номер>: <эффект>;
//  break;

void customRoutine(byte aMode) {
  doEffectWithOverlay(aMode); 
}

void doEffectWithOverlay(byte aMode) {

  // Оверлей нужен для всех эффектов, иначе при малой скорости эффекта и большой скорости часов поверх эффекта буквы-цифры "смазываются"
  bool textOvEn  = textOverlayEnabled && getEffectTextOverlayUsage(aMode);
  bool clockOvEn = clockOverlayEnabled && getEffectClockOverlayUsage(aMode);
  bool needStopText = false;
  
  // Если пришло время отображения очередной бегущей строки поверх эффекта - переключиться в режим бегущей строки оверлеем
  if (!showTextNow && textOvEn && ((millis() - textLastTime) > (TEXT_INTERVAL  * 1000L))) {
    prepareNextText();                   // Обработать следующую строку для отображения, установить параметры
    fullTextFlag = false;
    loadingTextFlag = false;
    showTextNow = true;                  // Флаг переключения в режим текста бегущей строки 
    textCurrentCount = 0;                // Сбросить счетчик количества раз, сколько строка показана на матрице;
    textStartTime = millis();            // Запомнить время начала отображения бегущей строки

    // Если указано, что строка должна отображаться на фоне конкретного эффекта - его надо инициализировать    
    if (specialTextEffect >= 0) {
      saveEffectBeforeText = thisMode;   // сохранить текущий эффект
      loadingFlag = specialTextEffect != saveEffectBeforeText;
    }
  } else

  // Если строка отображается, но флаг разрешения сняли - прекратить отображение
  if (showTextNow && !textOvEn) {
    needStopText = true;
  } else 
  
  // Если находимся в режиме бегущей строки и строка полностью "прокручена" или сняли разрешение на отображение бегущей строки - переключиться в режим отображения часов оверлеем
  // С учетом того, показана ли строка нужное количество раз или указанную продолжительность по времени
  if (showTextNow && fullTextFlag) {  
    if (textShowTime > 0) {
      // Показ строки ограничен по времени
      if ((millis() - textStartTime) > (textShowTime * 1000L)) {
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

  // Нужно прекратить показ текста бегущей строки
  if (needStopText) {
    showTextNow = false; 
    textLastTime = millis();
    if (saveEffectBeforeText >= 0) {
      loadingFlag = specialTextEffect != saveEffectBeforeText;  // Восстановленный эффект надо будет перезагрузить
      saveEffectBeforeText = -1;                                // Сбросить сохраненный / спецэффект
      specialTextEffect = -1;      
    }
  }

  bool effectReady = effectTimer.isReady();
  bool clockReady = clockTimer.isReady();
  bool textReady = textTimer.isReady();

  if (!(effectReady || (clockReady && !showTextNow) || (textReady && showTextNow))) return;

  // Нужно сохранять оверлей эффекта до отрисовки часов или бегущей строки поверх эффекта?
  bool needOverlay  = 
       (aMode == MC_CLOCK) ||                                                         // Если включен режим "Часы" (ночные часы)
       (aMode == MC_TEXT) ||                                                          // Если включен режим "Бегущая строка" (show IP address)       
      (!showTextNow && clockOvEn) || 
       (showTextNow && textOvEn);

  // В прошлой итерации часы / текст были наложены с оверлеем?
  // Если да - восстановить пиксели эффекта сохраненные перед наложением часов / текста
  if (overlayDelayed && !loadingFlag) {
    overlayUnwrap();
  }  
  
  if (effectReady) {
    if (showTextNow && specialTextEffect >= 0) {
      // Если указан спец-эффект поверх которого бежит строка - отобразить его
      processEffect(specialTextEffect);
    } else {
      // Иначе отрисовать текущий эффект
      processEffect(aMode);
    }
  }

  // Смещение движущихся часов 
  if (clockReady) {
    byte clock_width = CLOCK_ORIENT == 0 ? (c_size == 1 ? 15 : 26) : 7;     // Горизонтальные часы занимают 15/26 колонок (малые/большие), вертикальные - 7
    byte calendar_width = 15;                                               // Календарь занимает 15 колонок (4 цифры 3x5 и 3 пробела между ними)
    CLOCK_MOVE_CNT--;
    if (CLOCK_MOVE_CNT <= 0) {
       CLOCK_MOVE_CNT = CLOCK_MOVE_DIVIDER;
       CLOCK_XC--;
       CALENDAR_XC--;
       if (CLOCK_XC < -clock_width) {
          CLOCK_XC = WIDTH - clock_width - 1;
       }     
       if (CALENDAR_XC < -calendar_width) {
          CALENDAR_XC = WIDTH - calendar_width - 1;
       }     
    }
  }

  if (c_size == 1) {
    checkCalendarState();
  }

  overlayDelayed = needOverlay;
  
  if (needOverlay) {
    overlayWrap();
    // Если время инициализировали и пришло время его показать - нарисовать часы поверх эффекта
    if (init_time && !showTextNow && aMode != MC_TEXT) {
      setOverlayColors();
      if (c_size == 1 && showDateInClock && showDateState) {      
        drawCalendar(aday, amnth, ayear, dotFlag, CALENDAR_XC, CALENDAR_Y);
      } else {
        drawClock(hrs, mins, dotFlag, CLOCK_XC, CLOCK_Y);
      }
    } else if (showTextNow && aMode != MC_CLOCK && aMode != MC_TEXT) {
      // Нарисоватьоверлеем текст бегущей строки
      if (textReady) {
        // Сдвинуть позицию отображения бегущей строки
        shiftTextPosition();
      }
      // Нарисовать текст в текущей позиции
      runningText();
    }
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
    case MC_PALETTE:             paletteRoutine(); break;
    case MC_ANALYZER:            analyzerRoutine(); break;
    case MC_IMAGE:               animationRoutine(); break;
    case MC_TEXT:                runningText(); break;
    case MC_CLOCK:               clockRoutine(); break;
    case MC_DAWN_ALARM:          dawnProcedure(); break;
    
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
  byte curMode = thisMode;

  while (aCnt < MAX_EFFECT) {
    // Берем следующий режим по циклу режимов
    aCnt++; thisMode++;  
    if (thisMode >= MAX_EFFECT) thisMode = 0;

    // Если новый режим отмечен флагом "использовать" - используем его, иначе берем следующий (и проверяем его)
    if (getEffectUsage(thisMode)) break;
    
    // Если перебрали все и ни у адного нет флага "использовать" - не обращаем внимание на флаг, используем следующий
    if (aCnt >= MAX_EFFECT) {
      thisMode = curMode++;
      if (thisMode >= MAX_EFFECT) thisMode = 0;
      break;
    }
  }
  
  loadingFlag = true;
  autoplayTimer = millis();
  setTimersForMode(thisMode);
  
  FastLED.clear();
  FastLED.show();
  FastLED.setBrightness(globalBrightness);
}

void prevModeHandler() {

  if (useRandomSequence) {
    setRandomMode2();
    return;
  }

  byte aCnt = 0;
  byte curMode = thisMode;

  while (aCnt < MAX_EFFECT) {
    // Берем предыдущий режим по циклу режимов
    aCnt++; thisMode--; 
    if (thisMode < 0) thisMode = MAX_EFFECT - 1;

    // Если новый режим отмечен флагом "использовать" - используем его, иначе берем следующий (и проверяем его)
    if (getEffectUsage(thisMode)) break;
    
    // Если перебрали все и ни у одного нет флага "использовать" - не обращаем внимание на флаг, используем предыдцщий
    if (aCnt >= MAX_EFFECT) {
      thisMode = curMode--;
      if (thisMode < 0) thisMode = MAX_EFFECT - 1;
      break;
    }
  }
  
  loadingFlag = true;
  autoplayTimer = millis();
  setTimersForMode(thisMode);
  
  FastLED.clear();
  FastLED.show();
  FastLED.setBrightness(globalBrightness);
}

void setTimersForMode(byte aMode) {

  if (!(aMode == MC_TEXT || aMode == MC_CLOCK)) {
    effectSpeed = getEffectSpeed(aMode);
    if (effectSpeed == 0) effectSpeed = 2;
    // Эти режимы смотрятся (работают) только на максимальной скорости;
    if (aMode == MC_PAINTBALL || aMode == MC_SWIRL || aMode == MC_FLICKER || aMode == MC_PACIFICA || aMode == MC_SHADOWS)
      effectTimer.setInterval(1);        
    else
      effectTimer.setInterval(effectSpeed);
  }
    
  clockSpeed = getClockScrollSpeed();
  if (clockSpeed < D_CLOCK_SPEED_MIN) clockSpeed = D_CLOCK_SPEED_MIN; // Если clockSpeed == 0 - бегущая строка начинает дергаться.
  if (clockSpeed > D_CLOCK_SPEED_MAX) clockSpeed = D_CLOCK_SPEED_MAX;
  if (clockSpeed >= 240) {
    clockTimer.setInterval(4294967295);
    checkClockOrigin();
  } else {
    clockTimer.setInterval(clockSpeed);
  }

  textSpeed = getTextScrollSpeed();
  if (textSpeed < D_TEXT_SPEED_MIN) textSpeed = D_TEXT_SPEED_MIN; // Если textSpeed == 0 - бегущая строка начинает дергаться.
  if (textSpeed > D_TEXT_SPEED_MAX) textSpeed = D_TEXT_SPEED_MAX;
  textTimer.setInterval(textSpeed);
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
    if ((ms - autoplayTimer > autoplayTime) && !manualMode && AUTOPLAY) {    // таймер смены режима
      bool ok = true;
      if (thisMode == MC_TEXT   && !fullTextFlag || 
          thisMode == MC_MAZE   && !gameOverFlag ||
      //  thisMode == MC_SNAKE  && !gameOverFlag ||   // Змейка долгая игра - не нужно дожидаться окончания, можно прервать
          thisMode == MC_TETRIS && !gameOverFlag) {        
        // Если бегущая строка или игра не завершены - смены режима не делать
        ok = false;
      } 

      // Смена режима разрешена
      if (ok) {
        // Если режим не игра и не бегущая строка или один из этих режимов и есть флаг завершения режима -
        // перейти к следующему режимуж если режим был - бегущая строка - зафиксировать время окончания отображения последней бегущей строки        
        autoplayTimer = millis();
        nextMode();
      }
    }
  } else {
    if (idleTimer.isReady()) {      // таймер холостого режима. Если время наступило - включить автосмену режимов 
      idleState = true;                                     
      autoplayTimer = millis();
      loadingFlag = true;
      AUTOPLAY = true;
      manualMode = false;
      FastLED.clear();
      FastLED.show();
    }
  }  
}
