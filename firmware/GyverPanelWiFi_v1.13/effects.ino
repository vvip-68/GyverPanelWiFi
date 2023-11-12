
// Эта функция в FastLED объявлена как forward;
// линкуется с библиотекой FastLed, которая использует её для определения индекса светодиода в массиве leds[]
// ври вызове функций типа blur2d() и т.п.
uint16_t XY(uint8_t x, uint8_t y) { 
  return (uint16_t)getPixelNumber(x, y);
}

// ---------------------------------------------

uint8_t getEffectSpeedValue(int8_t eff) {
  #if (USE_E131 == 1)
  if (workMode == SLAVE && e131_streaming) {
    return syncEffectSpeed;  
  }
  #endif
  return effectSpeed[eff];
}
uint8_t getEffectContrastValue(int8_t eff) {
  #if (USE_E131 == 1)
  if (workMode == SLAVE && e131_streaming) {
    return syncEffectContrast;  
  }
  #endif
  return effectContrast[eff];
}
uint8_t getEffectScaleParamValue(int8_t eff) {
  #if (USE_E131 == 1)
  if (workMode == SLAVE && e131_streaming) {
    return syncEffectParam1;  
  }
  #endif
  return effectScaleParam[eff];  
}
uint8_t getEffectScaleParamValue2(int8_t eff) {
  #if (USE_E131 == 1)
  if (workMode == SLAVE && e131_streaming) {
    return syncEffectParam2;  
  }
  #endif
  return effectScaleParam2[eff];  
}

// ---------------------------------------------

// *********** снегопад 2.0 ***********

void snowRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_SNOW;
    FastLED.clear();  // очистить
  }

  // сдвигаем всё вниз
  shiftDown();

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  for (uint8_t x = 0; x < pWIDTH; x++) {
    // заполняем случайно верхнюю строку
    // а также не даём двум блокам по вертикали вместе быть
    if (getPixColorXY(x, pHEIGHT - 2) == 0 && (random8(0, map8(255 - getEffectScaleParamValue(MC_SNOW),5,15)) == 0)) {
      CRGB color = CRGB(effectBrightness,effectBrightness,effectBrightness); /*0xE0FFFF*/
      if (color.r > 0x20 && random8(0, 4) == 0) color = color - CRGB(0x10, 0x10, 0x10);
      drawPixelXY(x, pHEIGHT - 1, color);
    } else {
      drawPixelXY(x, pHEIGHT - 1, 0x000000);
    }
  }
}

// ------------- ПЕЙНТБОЛ -------------

uint8_t USE_SEGMENTS_PAINTBALL = 0;
uint8_t BorderWidth = 0;
uint8_t dir_mx, seg_num, seg_size, seg_offset, seg_offset_x, seg_offset_y;
int16_t idx;

void lightBallsRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_PAINTBALL;
    FastLED.clear();  // очистить
    dir_mx = pWIDTH > pHEIGHT ? 0 : 1;                                   // 0 - квадратные сегменты расположены горизонтально, 1 - вертикально
    seg_num = dir_mx == 0 ? (pWIDTH / pHEIGHT) : (pHEIGHT / pWIDTH);     // вычисляем количество сегментов, умещающихся на матрице
    seg_size = dir_mx == 0 ? pHEIGHT : pWIDTH;                           // Размер квадратного сегмента (высота и ширина равны)
    seg_offset = ((dir_mx == 0 ? pWIDTH : pHEIGHT) - seg_size * seg_num) / (seg_num + 1); // смещение от края матрицы и между сегментами    
    BorderWidth = 0;
    USE_SEGMENTS_PAINTBALL = getEffectScaleParamValue2(MC_PAINTBALL);
  }
  
  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);
  
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  // Apply some blurring to whatever's already on the matrix
  // Note that we never actually clear the matrix, we just constantly
  // blur it repeatedly.  Since the blurring is 'lossy', there's
  // an automatic trend toward black -- by design.
  uint8_t blurAmount = map(effectBrightness, 32,255, 65,91);
  uint8_t actualBrightness = map(effectBrightness, 32,255, 125,250);
  blur2d(leds, pWIDTH, pHEIGHT, blurAmount);

  // The color of each point shifts over time, each at a different speed.
  uint32_t ms = millis();

  uint8_t  cnt = map8(255-getEffectScaleParamValue(MC_PAINTBALL),1,4);  // 1..4 шариков
  float spd = (map8(255-getEffectSpeedValue(MC_PAINTBALL), 50, 100) / 100.0) / (USE_SEGMENTS_PAINTBALL != 0 ? 1 : (float)seg_num);

  // Отрисовка режима происходит на максимальной скорости. Значение effectSpeed влияет на параметр BPM функции beatsin8
  // The easiest way to construct this is to multiply a floating point BPM value (e.g. 120.3) by 256, (e.g. resulting in 30796 in this case), and pass that as the 16-bit BPM argument.
  uint8_t m1 = ( 91.0 * spd) + 0.51;
  uint8_t m2 = (109.0 * spd) + 0.51;
  uint8_t m3 = ( 73.0 * spd) + 0.51;
  uint8_t m4 = (123.0 * spd) + 0.51;

  // Для неквадратных - вычленяем квадратные сегменты, которые равномерно распределяем по ширине / высоте матрицы 

  if (USE_SEGMENTS_PAINTBALL != 0) {
    uint8_t  i = beatsin8(m1, 0, seg_size - BorderWidth - 1);
    uint8_t  j = beatsin8(m2, 0, seg_size - BorderWidth - 1);
    uint8_t  k = beatsin8(m3, 0, seg_size - BorderWidth - 1);
    uint8_t  m = beatsin8(m4, 0, seg_size - BorderWidth - 1);

    uint8_t d1 = ms / 29;
    uint8_t d2 = ms / 41;
    uint8_t d3 = ms / 73;
    uint8_t d4 = ms / 97;
    
    for (uint8_t ii = 0; ii < seg_num; ii++) {
      delay(0); // Для предотвращения ESP8266 Watchdog Timer      
      uint8_t cx = dir_mx == 0 ? (seg_offset * (ii + 1) + seg_size * ii) : 0;
      uint8_t cy = dir_mx == 0 ? 0 : (seg_offset * (ii + 1) + seg_size * ii);
      uint8_t color_shift = ii * 50;
      if (cnt <= 1) { idx = getPixelNumber(i+cx, j+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d1, 200, actualBrightness); }
      if (cnt <= 2) { idx = getPixelNumber(j+cx, k+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d2, 200, actualBrightness); }
      if (cnt <= 3) { idx = getPixelNumber(k+cx, m+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d3, 200, actualBrightness); }
      if (cnt <= 4) { idx = getPixelNumber(m+cx, i+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d4, 200, actualBrightness); }
      
      // При соединении матрицы из угла вверх или вниз почему-то слева и справа узора остаются полосы, которые 
      // не гаснут обычным blur - гасим полоски левой и правой стороны дополнительно.
      // При соединении из угла влево или вправо или на неквадратных матрицах такого эффекта не наблюдается
      uint8_t fade_step = map8(effectBrightness, 1, 15);
      for (uint8_t i2 = cy; i2 < cy + seg_size; i2++) { 
        fadePixel(cx + BorderWidth, i2, fade_step);
        fadePixel(cx + seg_size - BorderWidth - 1, i2, fade_step);
      }
    }
  }
  else 
  {
    uint8_t  i = beatsin8(m1, BorderWidth, pWIDTH - BorderWidth - 1);
    uint8_t  j = beatsin8(m1, BorderWidth, pHEIGHT - BorderWidth - 1);
    uint8_t  k = beatsin8(m3, BorderWidth, pWIDTH - BorderWidth - 1);
    uint8_t  m = beatsin8(m4, BorderWidth, pHEIGHT - BorderWidth - 1);
    
    if (cnt <= 1) { idx = getPixelNumber(i, j); if (idx >= 0) leds[idx] += CHSV( ms / 29, 200, actualBrightness); }
    if (cnt <= 2) { idx = getPixelNumber(k, j); if (idx >= 0) leds[idx] += CHSV( ms / 41, 200, actualBrightness); }
    if (cnt <= 3) { idx = getPixelNumber(k, m); if (idx >= 0) leds[idx] += CHSV( ms / 73, 200, actualBrightness); }
    if (cnt <= 4) { idx = getPixelNumber(i, m); if (idx >= 0) leds[idx] += CHSV( ms / 97, 200, actualBrightness); }
  
    if (pWIDTH == pHEIGHT) {
      // При соединении матрицы из угла вверх или вниз почему-то слева и справа узора остаются полосы, которые 
      // не гаснут обычным blur - гасим полоски левой и правой стороны дополнительно.
      // При соединении из угла влево или вправо или на неквадратных матрицах такого эффекта не наблюдается
      uint8_t fade_step = map8(effectBrightness, 1, 15);
      for (uint8_t i = 0; i < pHEIGHT; i++) { 
        fadePixel(0, i, fade_step);
        fadePixel(pWIDTH-1, i, fade_step);
      }
    } 
  }
}

// ------------- ВОДОВОРОТ -------------

uint8_t USE_SEGMENTS_SWIRL = 0;

void swirlRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_SWIRL;
    FastLED.clear();  // очистить
    dir_mx = pWIDTH > pHEIGHT ? 0 : 1;                                   // 0 - квадратные сегменты расположены горизонтально, 1 - вертикально
    seg_num = dir_mx == 0 ? (pWIDTH / pHEIGHT) : (pHEIGHT / pWIDTH);     // вычисляем количество сегментов, умещающихся на матрице
    seg_size = dir_mx == 0 ? pHEIGHT : pWIDTH;                           // Размер квадратного сегмента (высота и ширина равны)
    seg_offset = ((dir_mx == 0 ? pWIDTH : pHEIGHT) - seg_size * seg_num) / (seg_num + 1); // смещение от края матрицы и между сегментами    
    BorderWidth = seg_num == 1 ? 0 : 1;
    USE_SEGMENTS_SWIRL = getEffectScaleParamValue2(MC_SWIRL);
  }

  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  // Apply some blurring to whatever's already on the matrix
  // Note that we never actually clear the matrix, we just constantly
  // blur it repeatedly.  Since the blurring is 'lossy', there's
  // an automatic trend toward black -- by design.

  uint8_t blurAmount = map(effectBrightness, 32,255, 65,91);
  uint8_t actualBrightness = map(effectBrightness, 32,255, 125,250);
  blur2d(leds, pWIDTH, pHEIGHT, blurAmount);

  uint32_t ms = millis();  
  float spd = (map8(255-getEffectSpeedValue(MC_SWIRL), 50, 100) / 100.0) / (USE_SEGMENTS_SWIRL != 0 ? 1 : (float)seg_num);

  // Отрисовка режима происходит на максимальной скорости. Значение effectSpeed влияет на параметр BPM функции beatsin8
  // The easiest way to construct this is to multiply a floating point BPM value (e.g. 120.3) by 256, (e.g. resulting in 30796 in this case), and pass that as the 16-bit BPM argument.
  uint8_t m1 = (41.0 * spd) + 0.51;
  uint8_t m2 = (27.0 * spd) + 0.51;

  if (USE_SEGMENTS_SWIRL != 0) {
    // Use two out-of-sync sine waves
    uint8_t  i = beatsin8(m1, 0, seg_size - BorderWidth - 1);
    uint8_t  j = beatsin8(m2, 0, seg_size - BorderWidth - 1);

    // Also calculate some reflections
    uint8_t ni = (seg_size-1)-i;
    uint8_t nj = (seg_size-1)-j;

    uint8_t d1 = ms / 11;
    uint8_t d2 = ms / 13;
    uint8_t d3 = ms / 17;
    uint8_t d4 = ms / 29;
    uint8_t d5 = ms / 37;
    uint8_t d6 = ms / 41;
    
    for (uint8_t ii = 0; ii < seg_num; ii++) {
      delay(0); // Для предотвращения ESP8266 Watchdog Timer      
      uint8_t cx = dir_mx == 0 ? (seg_offset * (ii + 1) + seg_size * ii) : 0;
      uint8_t cy = dir_mx == 0 ? 0 : (seg_offset * (ii + 1) + seg_size * ii);
      uint8_t color_shift = ii * 50;
    
      // The color of each point shifts over time, each at a different speed.
      idx = getPixelNumber( i+cx, j+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d1, 200, actualBrightness);
      idx = getPixelNumber(ni+cx,nj+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d2, 200, actualBrightness);
      idx = getPixelNumber( i+cx,nj+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d3, 200, actualBrightness);
      idx = getPixelNumber(ni+cx, j+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d4, 200, actualBrightness);
      idx = getPixelNumber( j+cx, i+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d5, 200, actualBrightness);
      idx = getPixelNumber(nj+cx,ni+cy); if (idx >= 0) leds[idx] += CHSV( color_shift + d6, 200, actualBrightness);
      
      // При соединении матрицы из угла вверх или вниз почему-то слева и справа узора остаются полосы, которые 
      // не гаснут обычным blur - гасим полоски левой и правой стороны дополнительно.
      // При соединении из угла влево или вправо или на неквадратных матрицах такого эффекта не наблюдается
      uint8_t fade_step = map8(effectBrightness, 1, 15);
      for (uint8_t i2 = cy; i2 < cy + seg_size; i2++) { 
        fadePixel(cx, i2, fade_step);
        fadePixel(cx + BorderWidth, i2, fade_step);
        fadePixel(cx + seg_size - 1, i2, fade_step);
        fadePixel(cx + seg_size - BorderWidth - 1, i2, fade_step);
      }
    }
  } 
  else 
  {
    // Use two out-of-sync sine waves
    uint8_t  i = beatsin8(m1, BorderWidth, pWIDTH - BorderWidth - 1);
    uint8_t  j = beatsin8(m2, BorderWidth, pHEIGHT - BorderWidth - 1);

    // Also calculate some reflections
    uint8_t ni = (pWIDTH-1)-i;
    uint8_t nj = (pHEIGHT-1)-j;

    // The color of each point shifts over time, each at a different speed.
    idx = getPixelNumber( i, j); if (idx >= 0) leds[idx] += CHSV( ms / 11, 200, actualBrightness);
    idx = getPixelNumber(ni,nj); if (idx >= 0) leds[idx] += CHSV( ms / 13, 200, actualBrightness);
    idx = getPixelNumber( i,nj); if (idx >= 0) leds[idx] += CHSV( ms / 17, 200, actualBrightness);
    idx = getPixelNumber(ni, j); if (idx >= 0) leds[idx] += CHSV( ms / 29, 200, actualBrightness);
    
    if (pHEIGHT == pWIDTH) {
      // для квадратных матриц - 6 точек создают более красивую картину
      idx = getPixelNumber( j, i); if (idx >= 0) leds[idx] += CHSV( ms / 37, 200, actualBrightness);
      idx = getPixelNumber(nj,ni); if (idx >= 0) leds[idx] += CHSV( ms / 41, 200, actualBrightness);
      
      // При соединении матрицы из угла вверх или вниз почему-то слева и справа узора остаются полосы, которые 
      // не гаснут обычным blur - гасим полоски левой и правой стороны дополнительно.
      // При соединении из угла влево или вправо или на неквадратных матрицах такого эффекта не наблюдается
      uint8_t fade_step = map8(effectBrightness, 1, 15);
      for (uint8_t i = 0; i < pHEIGHT; i++) { 
        fadePixel(0, i, fade_step);
        fadePixel(pWIDTH-1, i, fade_step);
      }
    }  
  }
}

// ***************************** БЛУДНЫЙ КУБИК *****************************

#define RANDOM_COLOR 1    // случайный цвет при отскоке

int16_t coordB[2];
int8_t  vectorB[2];
int8_t  ballSize;
CRGB    ballColor;

void ballRoutine() {

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  
  if (loadingFlag) {
    for (uint8_t i = 0; i < 2; i++) {
      coordB[i] = pWIDTH / 2 * 10;
      vectorB[i] = random8(8, 20);
      ballColor = CHSV(random8(0, 9) * 28, 255, effectBrightness);
    }
    // modeCode = MC_BALL;
    loadingFlag = false;
  }

  ballSize = map8(getEffectScaleParamValue(MC_BALL),2, max(minDim / 3, 2));

  for (uint8_t i = 0; i < 2; i++) {
    coordB[i] += vectorB[i];
    if (coordB[i] < 0) {
      coordB[i] = 0;
      vectorB[i] = -vectorB[i];
      if (RANDOM_COLOR) ballColor = CHSV(random8(0, 9) * 28, 255, effectBrightness);
      //vectorB[i] += random8(0, 6) - 3;
    }
  }

  if (coordB[0] > (pWIDTH - ballSize) * 10) {
    coordB[0] = (pWIDTH - ballSize) * 10;
    vectorB[0] = -vectorB[0];
    if (RANDOM_COLOR) ballColor = CHSV(random8(0, 9) * 28, 255, effectBrightness);
    //vectorB[0] += random8(0, 6) - 3;
  }

  if (coordB[1] > (pHEIGHT - ballSize) * 10) {
    coordB[1] = (pHEIGHT - ballSize) * 10;
    vectorB[1] = -vectorB[1];
    if (RANDOM_COLOR) ballColor = CHSV(random8(0, 9) * 28, 255, effectBrightness);
    //vectorB[1] += random8(0, 6) - 3;
  }

  FastLED.clear();

  for (uint8_t i = 0; i < ballSize; i++) {
    for (uint8_t j = 0; j < ballSize; j++) {
      idx = getPixelNumber(coordB[0] / 10 + i, coordB[1] / 10 + j);
      if (idx >= 0) {
        leds[idx] = ballColor;
      }
    }
  }
}

// ***************************** РАДУГА *****************************

uint8_t rainbow_type = 0;

void rainbowRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_RAINBOW;
    rainbow_type = (specialTextEffectParam >= 0) ? specialTextEffectParam : getEffectScaleParamValue2(MC_RAINBOW);
    // Если авто - генерировать один из типов - 1-Вертикальная радуга, 2-Горизонтальная радуга, 3-Диагональная радуга, 4-Вращающаяся радуга
    if (rainbow_type == 0 || rainbow_type > 4) {
      rainbow_type = random8(1,4);
    }     
    FastLED.clear();  // очистить
  }

  switch (rainbow_type) {
    case 1:  rainbowVertical(); break;
    case 2:  rainbowHorizontal(); break;
    case 3:  rainbowDiagonal(); break;
    default: rainbowRotate(); break;
  }
}

// *********** радуга диагональная ***********

void rainbowDiagonal() {
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  hue += 2;
  for (uint8_t x = 0; x < pWIDTH; x++) {
    for (uint8_t y = 0; y < pHEIGHT; y++) {
      float dx = (pWIDTH >= pHEIGHT)
         ? (float)(pWIDTH / pHEIGHT * x + y)
         : (float)(pHEIGHT / pWIDTH * y + x);
      CRGB thisColor = CHSV((uint8_t)(hue + dx * map8(getEffectScaleParamValue(MC_RAINBOW),1, maxDim)), 255, effectBrightness);
      drawPixelXY(x, y, thisColor); 
    }
  }
}

// *********** радуга горизонтальная ***********

void rainbowHorizontal() {
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  hue += 2;
  for (uint8_t j = 0; j < pHEIGHT; j++) {
    CHSV thisColor = CHSV((uint8_t)(hue + j * map8(getEffectScaleParamValue(MC_RAINBOW),1,pWIDTH)), 255, effectBrightness);
    for (uint8_t i = 0; i < pWIDTH; i++)
      drawPixelXY(i, j, thisColor);
  }
}

// *********** радуга вертикальная ***********

void rainbowVertical() {
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  hue += 2;
  for (uint8_t i = 0; i < pWIDTH; i++) {
    CHSV thisColor = CHSV((uint8_t)(hue + i * map8(getEffectScaleParamValue(MC_RAINBOW),1,pHEIGHT)), 255, effectBrightness);
    for (uint8_t j = 0; j < pHEIGHT; j++)
      drawPixelXY(i, j, thisColor);
  }
}

// *********** радуга вращающаяся ***********

void rainbowRotate() {
  uint32_t ms = millis();
  int32_t  yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / pWIDTH));
  int32_t  xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / pHEIGHT));

  uint8_t  lineStartHue = ms / 65536;
  int8_t   yHueDelta8   = yHueDelta32 / 32768;
  int8_t   xHueDelta8   = xHueDelta32 / 32768;
  
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  for(uint8_t y = 0; y < pHEIGHT; y++) {
    lineStartHue += yHueDelta8;
    uint8_t pixelHue = lineStartHue;      
    for(uint8_t x = 0; x < pWIDTH; x++) {
      pixelHue += xHueDelta8;
      idx = getPixelNumber(x, y);
      if (idx >= 0) leds[idx]  = CHSV( pixelHue, 255, effectBrightness);
    }
  }
}

// ---------------------------------------- ЦВЕТА ------------------------------------------

void colorsRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_COLORS;
    FastLED.clear();  // очистить
  }
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  hue += map8(getEffectScaleParamValue(MC_COLORS),1,10);
  CHSV hueColor = CHSV(hue, 255, effectBrightness);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = hueColor;
  }
}

// ---------------------------------------- ЦИКЛОН ------------------------------------------

int16_t cycle_x, cycle_y; // могут уходить в минус при смене направления
uint8_t move_dir, fade_divider, inc_cnt, USE_SEGMENTS_CYCLON;

void cyclonRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_CYCLON;
    USE_SEGMENTS_CYCLON = getEffectScaleParamValue2(MC_CYCLON);
    dir_mx = pWIDTH > pHEIGHT ? 0 : 1;                                                                      // 0 - сегменты расположены горизонтально, 1 - вертикально
    seg_num = dir_mx == 0 ? (pWIDTH / pHEIGHT) : (pHEIGHT / pWIDTH);                                        // вычисляем количество сегментов, умещающихся на матрице, в режиме без сигментов ширина одной полоски будет равна кол-ву сегментов
    seg_size = dir_mx == 0 ? pHEIGHT : pWIDTH;                                                              // Размер квадратного сегмента (высота и ширина равны)
    seg_offset_y = USE_SEGMENTS_CYCLON == 1 ? (dir_mx == 1 ? pHEIGHT - seg_size * seg_num : 0) / 2 : 0;     // смещение от низа/верха матрицы
    seg_offset_x = USE_SEGMENTS_CYCLON == 1 ? (dir_mx == 0 ? pWIDTH - seg_size * seg_num : 0) / 2 : 0;      // смещение от левого/правого края матрицы
    hue = 0;
    cycle_x = USE_SEGMENTS_CYCLON == 1 ? (seg_offset_x + seg_size - 1) : pWIDTH - 1; // начало - от правого края к левому
    cycle_y = USE_SEGMENTS_CYCLON == 1 ?  seg_offset_y : 0;
    move_dir = 1;
    fade_divider = 0;
    inc_cnt = NUM_LEDS / 312;
    if (inc_cnt == 0) inc_cnt = 1;
    FastLED.clear();  // очистить
  }

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  uint8_t actualBrightness = map(effectBrightness, 32,255, 125,250);
  
  // Использовать отрисовку по сегментам
  // Если сегменты не используется - ширина одной полоски - кол-во сегментов
  for (uint8_t i=0; i < seg_num; i++) {  
    for (uint8_t k=0; k < inc_cnt; k++) { 
      if (USE_SEGMENTS_CYCLON == 1) {
        if (cycle_y + k - seg_offset_y >= seg_size) continue;
        idx = dir_mx == 0
           ? getPixelNumber(cycle_x + i * seg_size, cycle_y + k)
           : getPixelNumber(cycle_x, cycle_y + i * seg_size + k);
      } else {
        if (cycle_y + k  >= pHEIGHT) continue;
        idx = getPixelNumber(cycle_x + i, cycle_y + k);
      }
      if (idx >= 0 && idx < NUM_LEDS) 
          leds[idx] = CHSV(hue + k + (USE_SEGMENTS_CYCLON == 1 ? i * 85 : 0), 255, actualBrightness);              
    }
  }  

  hue += inc_cnt;
  
  // Затухание - не на каждый цикл, а регулируется параметром эффекта
  uint8_t fader_param = map8(255 - getEffectScaleParamValue(MC_CYCLON),0,5);
  fade_divider++;
  if (fade_divider > fader_param) {
    fade_divider = 0;
    fader(5);
  }

  cycle_y += inc_cnt;

  if (USE_SEGMENTS_CYCLON) {
    
    if (cycle_y - seg_offset_y >= seg_size) {
      cycle_y = seg_offset_y;
  
      if (move_dir == 0) {
        // Слева направо
        cycle_x++;     
        if (cycle_x - seg_offset_x >= seg_size) {
            move_dir = 1;
            cycle_x = seg_size - 1 + seg_offset_x;
        }
      } else {
        // Справа налево
        cycle_x--;     
        if (cycle_x < seg_offset_x) {
            move_dir = 0;
            cycle_x = seg_offset_x;
        }
      }    
    }
    
  } else {
    
    if (cycle_y >= pHEIGHT) {
      cycle_y = 0;
  
      if (move_dir == 0) {
        // Слева направо
        cycle_x += seg_num;     
        if (cycle_x >= pWIDTH) {
            move_dir = 1;
            cycle_x = pWIDTH - 1;
        }
      } else {
        // Справа налева
        cycle_x -= seg_num;     
        if (cycle_x < 0) {
            move_dir = 0;
            cycle_x = 0;
        }
      }    
    }
    
  }
  
}

// ********************** огонь **********************

#define SPARKLES 1        // вылетающие угольки вкл выкл

uint8_t matrixValue[8][16];
uint8_t *line;
uint8_t pcnt = 0;

//these values are substracetd from the generated values to give a shape to the animation
const uint8_t valueMask[8][16] PROGMEM = {
  {32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 },
  {64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 },
  {96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 },
  {128, 64 , 32 , 0  , 0  , 32 , 64 , 128, 128, 64 , 32 , 0  , 0  , 32 , 64 , 128},
  {160, 96 , 64 , 32 , 32 , 64 , 96 , 160, 160, 96 , 64 , 32 , 32 , 64 , 96 , 160},
  {192, 128, 96 , 64 , 64 , 96 , 128, 192, 192, 128, 96 , 64 , 64 , 96 , 128, 192},
  {255, 160, 128, 96 , 96 , 128, 160, 255, 255, 160, 128, 96 , 96 , 128, 160, 255},
  {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const uint8_t hueMask[8][16] PROGMEM = {
  {1 , 11, 19, 25, 25, 22, 11, 1 , 1 , 11, 19, 25, 25, 22, 11, 1 },
  {1 , 8 , 13, 19, 25, 19, 8 , 1 , 1 , 8 , 13, 19, 25, 19, 8 , 1 },
  {1 , 8 , 13, 16, 19, 16, 8 , 1 , 1 , 8 , 13, 16, 19, 16, 8 , 1 },
  {1 , 5 , 11, 13, 13, 13, 5 , 1 , 1 , 5 , 11, 13, 13, 13, 5 , 1 },
  {1 , 5 , 11, 11, 11, 11, 5 , 1 , 1 , 5 , 11, 11, 11, 11, 5 , 1 },
  {0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 , 0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 },
  {0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 , 0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 },
  {0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 }
};

void fireRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_FIRE;
    FastLED.clear();
    if (line == NULL) line = new uint8_t[pWIDTH];
    generateLine();
    memset(matrixValue, 0, sizeof(matrixValue));
  }

  if (pcnt >= 100) {
    shiftFireUp();
    generateLine();
    pcnt = 0;
  }

  drawFrame(pcnt);

  pcnt += 30;
}

void fireRoutineRelease() {
  if (line != NULL) { delete [] line; line = NULL; }
}

// Randomly generate the next line (matrix row)
void generateLine() {
  for (uint8_t x = 0; x < pWIDTH; x++) {
    line[x] = random8(64, 255);
  }
}

//shift all values in the matrix up one row

void shiftFireUp() {
  for (uint8_t y = pHEIGHT - 1; y > 0; y--) {
    for (uint8_t x = 0; x < pWIDTH; x++) {
      uint8_t newX = x;
      if (x > 15) newX = x%16;
      if (y > 7) continue;
      matrixValue[y][newX] = matrixValue[y - 1][newX];
    }
  }

  for (uint8_t x = 0; x < pWIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x%16;
    matrixValue[0][newX] = line[newX];
  }
}

// draw a frame, interpolating between 2 "key frames"
// @param pcnt percentage of interpolation

void drawFrame(uint8_t pcnt) {
  int nextv;

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  //each row interpolates with the one before it
  for (uint8_t y = pHEIGHT - 1; y > 0; y--) {
    for (uint8_t x = 0; x < pWIDTH; x++) {
      uint8_t newX = x;
      if (x > 15) newX = x%16;
      if (y < 8) {
        nextv =
          (((100.0 - pcnt) * matrixValue[y][newX]
            + pcnt * matrixValue[y - 1][newX]) / 100.0)
            - pgm_read_byte(&(valueMask[y][newX]));

        CRGB color = CHSV(
                       map8(getEffectScaleParamValue(MC_FIRE),0,230) + pgm_read_byte(&(hueMask[y][newX])), // H
                       255, // S
                       (uint8_t)max(0, nextv) // V
                     );
                     
        CRGB color2 = color.nscale8_video(effectBrightness);

        idx = getPixelNumber(x, y);
        if (idx >= 0) leds[idx] = color2;
        
      } else if (y == 8 && SPARKLES) {
        if (random8(0, 20) == 0 && getPixColorXY(x, y - 1) != 0) 
          drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else 
          drawPixelXY(x, y, 0);
      } else if (SPARKLES) {

        // старая версия для яркости
        if (getPixColorXY(x, y - 1) > 0)
          drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else 
          drawPixelXY(x, y, 0);
      }
    }
  }

  //first row interpolates with the "next" line
  for (uint8_t x = 0; x < pWIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x%16;
    CRGB color = CHSV(
                   map8(getEffectScaleParamValue(MC_FIRE),0,230) + pgm_read_byte(&(hueMask[0][newX])), // H
                   255,           // S
                   (uint8_t)(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0) // V
                 );
    CRGB color2 = color.nscale8_video(effectBrightness);

    // idx = getPixelNumber(newX, 0); // На форуме пишут что это ошибка - вместо newX должно быть x, иначе
    idx = getPixelNumber(x, 0);       // на матрицах шире 16 столбцов нижний правый угол неработает
    if (idx >= 0) leds[idx] = color2; 
  }
}

// **************** МАТРИЦА *****************

void matrixRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_MATRIX;
    FastLED.clear();
  }
  
  uint8_t  effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  uint32_t cut_out = pHEIGHT < 10 ? 0x40 : 0x20; // на 0x004000 хвосты матрицы короткие (4 точки), на 0x002000 - длиннее (8 точек)

  for (uint8_t x = 0; x < pWIDTH; x++) {
    // заполняем случайно верхнюю строку
    CRGB thisColor = getPixColorXY(x, pHEIGHT - 1);
    if (thisColor.g == 0) {
      idx = getPixelNumber(x, pHEIGHT - 1); 
      if (idx >= 0) leds[idx] = random8(0, map8(255 - getEffectScaleParamValue(MC_MATRIX),5,15)) == 0 ? CRGB(0, effectBrightness, 0) : CRGB(0,0,0);
    } else if (thisColor.g < cut_out)
      drawPixelXY(x, pHEIGHT - 1, 0);
    else
      drawPixelXY(x, pHEIGHT - 1, thisColor - CRGB(cut_out, cut_out, cut_out));
  }

  // сдвигаем всё вниз
  shiftDown();
}


// **************** ТРАФИК *****************

uint8_t *traficTColors;    // Цвета линий трафика верхней части матрицы
int16_t *traficTIndex;     // Позиция "головы" дорожки верхней части матрицы
uint8_t *traficBColors;    // Цвета линий трафика нижней части матрицы
int16_t *traficBIndex;     // Позиция "головы" дорожки нижней части матрицы
uint8_t *traficLColors;    // Цвета линий трафика левой части матрицы
int16_t *traficLIndex;     // Позиция "головы" дорожки левой части матрицы
uint8_t *traficRColors;    // Цвета линий трафика правой части матрицы
int16_t *traficRIndex;     // Позиция "головы" дорожки правой части матрицы
bool     isColored;

void trafficRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_TRAFFIC;
    hue = random8(0,255);
    
    if (traficTColors == NULL) { traficTColors = new uint8_t [pWIDTH];  }
    if (traficTIndex  == NULL) { traficTIndex  = new int16_t [pWIDTH];  }
    if (traficBColors == NULL) { traficBColors = new uint8_t [pWIDTH];  }
    if (traficBIndex  == NULL) { traficBIndex  = new int16_t [pWIDTH];  }
    if (traficLColors == NULL) { traficLColors = new uint8_t [pHEIGHT]; }
    if (traficLIndex  == NULL) { traficLIndex  = new int16_t [pHEIGHT]; }
    if (traficRColors == NULL) { traficRColors = new uint8_t [pHEIGHT]; }
    if (traficRIndex  == NULL) { traficRIndex  = new int16_t [pHEIGHT]; }

    FOR_x (0, pWIDTH) {
      traficTIndex[x] = -1;
      traficBIndex[x] = -1;
    }
    FOR_y (0, pHEIGHT) {
      traficLIndex[y] = -1;
      traficTIndex[y] = -1;
    }
    
    uint8_t variant = getEffectScaleParamValue2(thisMode);  // 0 - Случайный, 1 - цветной, 2 - Монохром 
    if (variant == 0) variant = random8() % 2;
    isColored = variant == 1;  
    
    FastLED.clear();
  }

  uint8_t  cnt;
  uint16_t density;
  uint8_t  effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  // Сгенерировать начало новых дорожек для верха и низа
  density = map8(255 - getEffectScaleParamValue(thisMode), 1, pWIDTH * 2) * 5;  
  FOR_x (0, pWIDTH) {
    // Все элементы массива изначально -1 - что означает "не активно"
    // В случайном порядке назначить дорожку активной, если она зще не активна
    if (random16(0, density) == 0 && traficTIndex[x] < 0) {
      traficTIndex[x] = 0;
      traficTColors[x] = random8();
    }
    if (random16(0, density) == 0 && traficBIndex[x] < 0) {
      traficBIndex[x] = 0;
      traficBColors[x] = random8();
    }
  }

  // Сгенерировать начало новых дорожек для левой и правой стороны
  density = map8(255 - getEffectScaleParamValue(thisMode), 1, pHEIGHT * 2) * 5;  
  FOR_y (0, pHEIGHT) {
    // Все элементы массива изначально -1 - что означает "не активно"
    // В случайном порядке назначить дорожку активной, если она зще не активна
    if (random16(0, density) == 0 && traficLIndex[y] < 0) {
      traficLIndex[y] = 0;
      traficLColors[y] = random8();
    }
    if (random16(0, density) == 0 && traficRIndex[y] < 0) {
      traficRIndex[y] = 0;
      traficRColors[y] = random8();
    }
  }

  // Максимальная длина горизонтальной и вертикальной дорожек - половина от ширины / высоты
  uint8_t max_width = pWIDTH / 3 * 2;
  uint8_t max_height = pHEIGHT / 3 * 2;

  // За длину дорожки от головы до "хвоста" яркость должна упасть от максимальной до нуля.
  // Рассчитать шаг убывания яркости для горизонтали и верикали
  uint8_t step_width = 255 / pWIDTH;
  uint8_t step_height = 255 / pHEIGHT;

  // Сверху вниз
  FOR_x (0, pWIDTH) {
    // traficTIndex - содержит координату по Y (отсчитываемая от верха матрицы) - 0 - голова "дорожки"
    // Для верха голову рисовать от Y и вверх, убывая яркость на шаг step_height до нуля
    // Когда координата Y выйдет за высоту матрицы + длину дорожки - поставить -1 - дорожка "свободна"
    int16_t y = traficTIndex[x];
    int16_t b = effectBrightness;
    if (traficTIndex[x] >= 0) {
      uint8_t c = isColored ? traficTColors[x] : hue;
      cnt = 0;
      for (int16_t i = y; i >= 0; i--) {
        cnt++;
        idx = getPixelNumber(x, pHEIGHT - i - 1);
        if (idx >=0) leds[idx] = CHSV(c, 255, cnt == max_height ? 0 : b);   
        b -= step_height;
        if (b < 0) b = 0;
        if (cnt == max_height) break;
      }
      y++;
      traficTIndex[x] = y > pHEIGHT + max_height ? -1 : y;
      if (traficTIndex[x] < 0) traficTColors[x] = random8();
    }
  }


  // Слева направо
  FOR_y (0, pHEIGHT) {
    // traficLIndex - содержит координату по X (отсчитываемая от левой стороны матрицы) - 0 - голова "дорожки"
    // Для левой стороны голову рисовать от 0 и вправо, убывая яркость на шаг step_width до нуля
    // Когда координата X выйдет за ширину матрицы + длину дорожки - поставить -1 - дорожка "свободна"
    int16_t x = traficLIndex[y];
    int16_t b = effectBrightness;
    if (traficLIndex[y] >= 0) {
      uint8_t c = isColored ? traficLColors[y] : hue;
      cnt = 0;
      for (int16_t i = x; i >= 0; i--) {
        cnt++;
        idx = getPixelNumber(i, y);
        if (idx >=0) leds[idx] = CHSV(c, 255, cnt == max_width ? 0 : b);   
        b -= step_width;
        if (b < 0) b = 0;
        if (cnt == max_width) break;
      }
      x++;
      traficLIndex[y] = x > pWIDTH + max_width ? -1 : x;
      if (traficLIndex[y] < 0) traficLColors[y] = random8();
    }
  }
  

  // Снизу вверх
  FOR_x (0, pWIDTH) {
    // traficBIndex - содержит координату по Y (отсчитываемая от низа матрицы) - 0 - голова "дорожки"
    // Для низа голову рисовать от Y и вниз, убывая яркость на шаг step_height до нуля
    // Когда координата Y выйдет за минус длину дорожки - поставить -1 - дорожка "свободна"
    int16_t y = traficBIndex[x];
    int16_t b = effectBrightness;
    if (traficBIndex[x] >= 0) {
      uint8_t c = isColored ? traficBColors[x] : hue;
      cnt = 0;
      for (int16_t i = y; i >= 0; i--) {        
        cnt++;
        idx = getPixelNumber(x, i);
        if (idx >=0) leds[idx] = CHSV(c, 255, cnt == max_height ? 0 : b);   
        b -= step_height;
        if (b < 0) b = 0;
        if (cnt == max_height) break;
      }
      y++;
      traficBIndex[x] = y > pHEIGHT + max_height ? -1 : y;
      if (traficBIndex[x] < 0) traficBColors[x] = random8();
    }
  }

  // Справа налево
  FOR_y (0, pHEIGHT) {
    // traficRIndex - содержит координату по X (отсчитываемая от правой стороны матрицы) - 0 - голова "дорожки"
    // Для правой стороны голову рисовать от 0 и влево, убывая яркость на шаг step_width до нуля
    // Когда координата X выйдет за ширину матрицы + длину дорожки - поставить -1 - дорожка "свободна"
    int16_t x = traficRIndex[y];
    int16_t b = effectBrightness;
    if (traficRIndex[y] >= 0) {
      uint8_t c = isColored ? traficRColors[y] : hue;
      cnt = 0;
      for (int16_t i = x; i >= 0; i--) {
        cnt++;
        idx = getPixelNumber(pWIDTH - i - 1, y);
        if (idx >=0) leds[idx] = CHSV(c, 255, cnt == max_width ? 0 : b);   
        b -= step_width;
        if (b < 0) b = 0;
        if (cnt == max_width) break;
      }
      x++;
      traficRIndex[y] = x > pWIDTH + max_width ? -1 : x;
      if (traficRIndex[y] < 0) traficRColors[y] = random8();
    }
  }

  hue++;
}

void trafficRoutineRelease() {
  if (traficTColors == NULL) { delete [] traficTColors; traficTColors = NULL; }
  if (traficTIndex  == NULL) { delete [] traficTIndex;  traficTIndex  = NULL; }
  if (traficBColors == NULL) { delete [] traficBColors; traficBColors = NULL; }
  if (traficBIndex  == NULL) { delete [] traficBIndex;  traficBIndex  = NULL; }
  if (traficLColors == NULL) { delete [] traficLColors; traficLColors = NULL; }
  if (traficLIndex  == NULL) { delete [] traficLIndex;  traficLIndex  = NULL; }
  if (traficRColors == NULL) { delete [] traficRColors; traficRColors = NULL; }
  if (traficRIndex  == NULL) { delete [] traficRIndex;  traficRIndex  = NULL; }
}

// ********************************* ШАРИКИ *********************************

#define BALLS_AMOUNT_MAX 6 // максимальное количество "шариков"
#define CLEAR_PATH 1       // очищать путь
#define BALL_TRACK 1       // (0 / 1) - вкл/выкл следы шариков
#define TRACK_STEP 70      // длина хвоста шарика (чем больше цифра, тем хвост короче)

int8_t  BALLS_AMOUNT;
int16_t coord[BALLS_AMOUNT_MAX][2];
int8_t  vector[BALLS_AMOUNT_MAX][2];
uint8_t ballColors[BALLS_AMOUNT_MAX];

void ballsRoutine() {
  if (loadingFlag) {
    // modeCode = MC_BALLS;
    loadingFlag = false;
    FastLED.clear();
    
    // Текущее количество шариков из настроек
    BALLS_AMOUNT = map8(getEffectScaleParamValue(MC_BALLS),3,6); 
    
    for (uint8_t j = 0; j < BALLS_AMOUNT; j++) {
      int8_t sign;

      // забиваем случайными данными
      coord[j][0] = pWIDTH / 2 * 10;
      random8(0, 2) ? sign = 1 : sign = -1;
      vector[j][0] = random8(4, 15) * sign;
      coord[j][1] = pHEIGHT / 2 * 10;
      random8(0, 2) ? sign = 1 : sign = -1;
      vector[j][1] = random8(4, 15) * sign;
      ballColors[j] = random8(0, 255);
    }
  }

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  if (!BALL_TRACK)    // если режим БЕЗ следов шариков
    FastLED.clear();  // очистить
  else {              // режим со следами
    fader(map8(effectBrightness, 4, TRACK_STEP));
  }

  // движение шариков
  for (uint8_t j = 0; j < BALLS_AMOUNT; j++) {

    // движение шариков
    for (uint8_t i = 0; i < 2; i++) {
      coord[j][i] += vector[j][i];
      if (coord[j][i] < 0) {
        coord[j][i] = 0;
        vector[j][i] = -vector[j][i];
      }
    }

    if (coord[j][0] > (pWIDTH - 1) * 10) {
      coord[j][0] = (pWIDTH - 1) * 10;
      vector[j][0] = -vector[j][0];
    }
    if (coord[j][1] > (pHEIGHT - 1) * 10) {
      coord[j][1] = (pHEIGHT - 1) * 10;
      vector[j][1] = -vector[j][1];
    }
    idx = getPixelNumber(coord[j][0] / 10, coord[j][1] / 10);
    if (idx >= 0) leds[idx] =  CHSV(ballColors[j], 255, effectBrightness);
  }
}

// ********************* ЗВЕЗДОПАД ******************

#define TAIL_STEP  80     // длина хвоста кометы (чем больше цифра, тем хвост короче)
#define SATURATION 150    // насыщенность кометы (от 0 до 255)

int8_t STAR_DENSE;     // плотность комет 30..90

void starfallRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_STARFALL;
    FastLED.clear();  // очистить
  }

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  STAR_DENSE = map8(getEffectScaleParamValue(MC_SPARKLES),30,90);
  
  // заполняем головами комет левую и верхнюю линию
  for (uint8_t i = 4; i < pHEIGHT; i++) {
    if (getPixColorXY(0, i) == 0
        && (random8(0, STAR_DENSE) == 0)
        && getPixColorXY(0, i + 1) == 0
        && getPixColorXY(0, i - 1) == 0) {
      idx = getPixelNumber(0, i);
      if (idx >= 0) leds[idx] = CHSV(random8(0, 200), SATURATION, effectBrightness);
    }
  }
  
  for (uint8_t i = 0; i < pWIDTH-4; i++) {
    if (getPixColorXY(i, pHEIGHT - 1) == 0
        && (random8(0, map8(getEffectScaleParamValue(MC_STARFALL),10,120)) == 0)
        && getPixColorXY(i + 1, pHEIGHT - 1) == 0
        && getPixColorXY(i - 1, pHEIGHT - 1) == 0) {
          idx = getPixelNumber(i, pHEIGHT - 1);           
          if (idx >= 0) leds[idx] = CHSV(random8(0, 200), SATURATION, effectBrightness);
    }
  }

  // сдвигаем по диагонали
  shiftDiag();

  // уменьшаем яркость левой и верхней линии, формируем "хвосты"
  for (uint8_t i = 4; i < pHEIGHT; i++) {
    fadePixel(0, i, TAIL_STEP);
  }
  for (uint8_t i = 0; i < pWIDTH-4; i++) {
    fadePixel(i, pHEIGHT - 1, TAIL_STEP);
  }

}

// *********************  КОНФЕТТИ ******************

#define SPARKLES_FADE_STEP 70    // шаг уменьшения яркости

void sparklesRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_SPARKLES;
    FastLED.clear();  // очистить
  }

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  uint8_t sparklesCount = map8(getEffectScaleParamValue(MC_SPARKLES),1,25);
  
  for (uint8_t i = 0; i < sparklesCount; i++) {
    uint8_t x = random8(0, pWIDTH);
    uint8_t y = random8(0, pHEIGHT);
    if (getPixColorXY(x, y) == 0) {
      idx = getPixelNumber(x, y); 
      if (idx >= 0) leds[idx] = CHSV(random8(0, 255), 255, effectBrightness);
    }
  }

  fader(map8(effectBrightness, 4, SPARKLES_FADE_STEP));
}

// ********************* СВЕТЛЯКИ *********************

#define LIGHTERS_AM 100

int8_t  **lightersPos;   // Позиции светляков
int8_t  **lightersSpeed; // Скорость движения светляков
uint8_t *lightersColor;  // Цвета светляков
    
void lightersRoutine() {
  
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_LIGHTERS;

    if (lightersPos == NULL) { lightersPos = new int8_t*[2]; for (uint8_t i = 0; i < 2; i++) { lightersPos[i] = new int8_t [LIGHTERS_AM]; }}
    if (lightersSpeed == NULL) { lightersSpeed = new int8_t*[2]; for (uint8_t i = 0; i < 2; i++) { lightersSpeed[i] = new int8_t [LIGHTERS_AM]; }}
    if (lightersColor == NULL) { lightersColor = new uint8_t [LIGHTERS_AM]; }

    FOR_i (0, LIGHTERS_AM) {
      lightersPos[0][i] = random16(0, pWIDTH);
      lightersPos[1][i] = random16(0, pHEIGHT);
      lightersSpeed[0][i] = random8(0, 4) - 2;
      lightersSpeed[1][i] = random8(0, 4) - 2;
      lightersColor[i] = random8(0, 255);
    }
  }

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  FastLED.clear();

  if (++loopCounter > 20) loopCounter = 0;

   FOR_i (0, map8(getEffectScaleParamValue(MC_LIGHTERS),5,100)) {
    if (loopCounter == 0) {     // меняем скорость каждые 20 отрисовок
      while (lightersSpeed[0][i] == 0 && lightersSpeed[1][i] == 0) {
        lightersSpeed[0][i] += random8(0, 4) - 2;
        lightersSpeed[1][i] += random8(0, 4) - 2;
        lightersSpeed[0][i] = constrain(lightersSpeed[0][i], -5, 5);
        lightersSpeed[1][i] = constrain(lightersSpeed[1][i], -5, 5);
      }
    }

    lightersPos[0][i] += lightersSpeed[0][i];
    lightersPos[1][i] += lightersSpeed[1][i];

    if (lightersPos[0][i] < 0) lightersPos[0][i] = pWIDTH - 1;
    if (lightersPos[0][i] >= pWIDTH) lightersPos[0][i] = 0;

    if (lightersPos[1][i] < 0) {
      lightersPos[1][i] = 0;
      lightersSpeed[1][i] = -lightersSpeed[1][i];
    }
    if (lightersPos[1][i] >= pHEIGHT - 1) {
      lightersPos[1][i] = pHEIGHT - 1;
      lightersSpeed[1][i] = -lightersSpeed[1][i];
    }
    drawPixelXY(lightersPos[0][i], lightersPos[1][i], CHSV(lightersColor[i], 255, effectBrightness));
  }
}

void lighters2RoutineRelease() {
  if (lightersPos   != NULL) { for( uint8_t i = 0; i < 2; i++ ) { delete [] lightersPos[i];}   delete [] lightersPos;   lightersPos   = NULL; }
  if (lightersSpeed != NULL) { for( uint8_t i = 0; i < 2; i++ ) { delete [] lightersSpeed[i];} delete [] lightersSpeed; lightersSpeed = NULL; }
  
  if (lightersColor == NULL) { delete [] lightersColor; lightersColor = NULL; }
}

// ******************* МЕРЦАНИЕ ********************

uint32_t xf,yf,v_time,hue_time,hxy;

// Play with the values of the variables below and see what kinds of effects they
// have!  More octaves will make things slower.

// how many octaves to use for the brightness and hue functions
uint8_t  octaves=3;
uint8_t  hue_octaves=1;

// the 'distance' between points on the x and y axis
int32_t  xscale=57771;
int32_t  yscale=57771;

// the 'distance' between x/y points for the hue noise
int32_t  hue_scale=1;

// how fast we move through time & hue noise
int32_t  time_speed=1111;
uint8_t  hue_speed=10;

// adjust these values to move along the x or y axis between frames
uint16_t x_speed, y_speed;

void flickerRoutine() {
  if (loadingFlag) {
    // modeCode = MC_FLICKER;
    loadingFlag = false;
    x_speed = (pWIDTH > pHEIGHT ? 1111 : 331);
    y_speed = (pWIDTH > pHEIGHT ? 331 : 1111);

    hxy = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
    xf = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
    yf = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
    v_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
    hue_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();    
  }

  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);

  // uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  // adjust the intra-frame time values
  hue_speed = map8(255-getEffectSpeedValue(MC_FLICKER), 1, 5);

  // fill the led array 2/16-bit noise values
  fill_2dnoise16(leds, pWIDTH, pHEIGHT, (sMATRIX_TYPE == 0),
                octaves, xf, xscale, yf, yscale, v_time,
                hue_octaves, hxy, hue_scale, hxy, hue_scale, hue_time, 
                false);
 
  xf += x_speed;
  yf += y_speed;

  v_time += time_speed;
  hue_time += hue_speed;
}

// *********************  ЗВЕЗДОЧКИ ******************

#define STARS_FADE_STEP 5     // шаг уменьшения яркости
uint8_t drawRays = 0;

void starsRoutine() {
  if (loadingFlag) {
    // modeCode = MC_STARS;
    loadingFlag = false;
    loopCounter = 0;
    hue = 46;
    //   0               1         2        3        4
    // ">Случайный выбор,Без лучей,Лучи '+',Лучи 'X',Лучи '+' и 'X'"
    drawRays = getEffectScaleParamValue2(thisMode);  
    if (drawRays == 0) drawRays = random8(1, 4);
    FastLED.clear();  // очистить
  }

  delay(5);  
  fader(STARS_FADE_STEP);

  uint8_t spd = getEffectSpeedValue(thisMode);
  if (spd > 0 && loopCounter++ < map8(spd, 0, 30)) return;
  loopCounter = 0;
    
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  uint8_t fadeBrightness =  effectBrightness / 4 * 3;  
  uint8_t the_color = getEffectScaleParamValue(thisMode);
  uint8_t color = the_color;
  int8_t  delta = random8(0, 12) - 6;

  if (the_color < 2) {
    color = random8(0, 255);
  } else if (the_color > 252) {
    color = hue += (spd == 0 ? 1 : 2);
  } else {
    color += delta;
  }

  uint8_t cnt = 0;
  while (cnt<25) {
    cnt++;
    uint8_t x = random8(1, pWIDTH - 1);
    uint8_t y = random8(1, pHEIGHT - 1);
    bool enable = drawRays == 1 ||
                  getPixColorXY(x,   y  ) == 0 && 
                  getPixColorXY(x+1, y  ) == 0 &&
                  getPixColorXY(x-1, y  ) == 0 &&
                  getPixColorXY(x,   y+1) == 0 &&
                  getPixColorXY(x,   y-1) == 0 &&
                  getPixColorXY(x+1, y+1) == 0 &&
                  getPixColorXY(x+1, y-1) == 0 &&
                  getPixColorXY(x-1, y+1) == 0 &&
                  getPixColorXY(x-1, y-1) == 0;
                  
    if (enable) {
      uint8_t sat = (random8(0, 100) % 10 == 0) ? 32 : 255;   // Одна из 10 звезд - белая
      CHSV star_color = CHSV(color, sat, effectBrightness);  
      // Центр
      idx = getPixelNumber(x, y); 
      if (idx >= 0) leds[idx] = star_color;
      if (drawRays > 1) {
        // Стороны лучей
        star_color = CHSV(color, sat, fadeBrightness);
        bool useXRay = random8(0, 50) % 2 == 0;
        if (drawRays == 3 || drawRays == 4 && useXRay) {
          // Тип - X
          idx = getPixelNumber(x+1, y+1); 
          if (idx >= 0) leds[idx] = star_color;
          idx = getPixelNumber(x-1, y+1); 
          if (idx >= 0) leds[idx] = star_color;
          idx = getPixelNumber(x+1, y-1); 
          if (idx >= 0) leds[idx] = star_color;
          idx = getPixelNumber(x-1, y-1); 
          if (idx >= 0) leds[idx] = star_color;              
        } else if (drawRays == 2 || drawRays == 4 && !useXRay) {
          // Тип - крест +
          idx = getPixelNumber(x+1, y); 
          if (idx >= 0) leds[idx] = star_color;
          idx = getPixelNumber(x-1, y); 
          if (idx >= 0) leds[idx] = star_color;
          idx = getPixelNumber(x, y+1); 
          if (idx >= 0) leds[idx] = star_color;
          idx = getPixelNumber(x, y-1); 
          if (idx >= 0) leds[idx] = star_color;      
        }
      }
      break;
    }
  }
}

// *********************  ЗВЕЗДОЧКИ-2 (Штора) ******************

#define STARS2_FADE_STEP 10   // шаг уменьшения яркости
#define BACK_BRIGHTNESS 20
#define STAR_BRIGHTNESS 36

int8_t  *starState;    // 0 - яркость не меняется 1 - яркость увеличивается -1 - яркость уменьшается
uint8_t *starBright;   // Текущая яркость звезды

uint8_t  numStarsWidth;
uint8_t  numStarsHeight;
uint16_t numStars;

void stars2Routine() {

  drawRays = getEffectScaleParamValue2(thisMode);
  uint8_t contrast = getEffectContrastValue(thisMode);
  uint8_t delta2 = 0;

  uint8_t delta = 255 - globalBrightness;  
  if (drawRays < 2) {
    if (delta > 200) delta2 = delta / 3; else
    if (delta > 150) delta2 = delta / 4; else
    if (delta > 100) delta2 = delta / 5; else
    if (delta > 50 ) delta2 = delta / 6; else
                     delta2 = delta / 7;
  } else {
    if (delta > 200) delta2 = delta / 4; else
    if (delta > 150) delta2 = delta / 5; else
    if (delta > 100) delta2 = delta / 6; else
    if (delta > 50 ) delta2 = delta / 7; else
                     delta2 = delta / 8;
  }
 
  uint8_t backBrightness = BACK_BRIGHTNESS + delta2;
  uint8_t starBrightness = constrain(STAR_BRIGHTNESS + delta2, STAR_BRIGHTNESS, 255);
  uint8_t maxEffectBrightness = constrain(contrast, 2 * starBrightness, 255);
  uint8_t maxFadeBrightness = maxEffectBrightness / 4 * 3;  

  if (loadingFlag) {
    // modeCode = MC_STARS2;
    loadingFlag = false;

    numStarsWidth = pWIDTH / 4;
    numStarsHeight = pHEIGHT / 4;
    numStars = numStarsWidth * numStarsHeight;
    hue = 0;
    loopCounter = 0;

    if (starState  == NULL) { starState  = new int8_t  [numStars]; }
    if (starBright == NULL) { starBright = new uint8_t [numStars]; }

    // Заполнить массив начальной яркости звезд 
    FOR_i(0, numStars) {
      starState[i] = 0;
      starBright[i] = starBrightness;
    }
    FastLED.clear();  // очистить
  }

  // На каждый 5 (или менее в зависимости от размера матрицы) шаг "зажигаем" следующую звезду
  loopCounter++;
  if (loopCounter == 5 - numStars / 256) {
    loopCounter = 0;
    idx = random16(0, numStars);
    if (starState[idx] == 0) {
      starState[idx] = 1;
      // Некоторые звезды зажигаются не плавно, а вспышкой и плавно угасает
      if (random8(0, 100) % 4 == 0) {
        starBright[idx] = maxEffectBrightness - STARS2_FADE_STEP;
      }
    }
  }

  // В режимах без фона крайние положения двиджка "вариант" включают режим прокрутки по радуге
  uint8_t color = getEffectScaleParamValue(thisMode);
  if (drawRays >= 2 && (color < 2 || color > 253)) {
    color = hue;
    loopCounter2++;
    if (loopCounter2 == 10) {
      loopCounter2 = 0;
      hue += 1;
    }
  }

  // Заливка поля цветом фона для режима с фоном или черным для режима без фона
  CHSV back_color = CHSV(color, 255, drawRays < 2 ? backBrightness : 0) ;     
  fillAll(back_color);

  FOR_x(0, numStarsWidth) {
    FOR_y(0, numStarsHeight) {

     // Корректировка яркости (угасание/зажигания) звезды
     idx = x + numStarsWidth * y;
     uint16_t br = starBright[idx];
     br += starState[idx] * STARS2_FADE_STEP;
     if (br >= maxEffectBrightness) {
       // При достижении максимальной яркости - переключить на "угасание"
       starState[idx] = -1;
       br = maxEffectBrightness;
     } else if (br <= starBrightness) {
       // При достижении минимальной яркости - переключить на "ожидание"
       starState[idx] = 0;
       br = starBrightness;
     }
     starBright[idx] = br;

     // Отрисовать звезду      
     uint8_t xp = x * 4 + 1;
     uint8_t yp = y * 4 + x % 2 + 1;
     uint8_t effectBrightness = constrain(starBright[x + y * numStarsWidth], starBrightness, maxEffectBrightness);

      // Центр
      idx = getPixelNumber(xp, yp); 
      CHSV star_color = CHSV(color, 255, effectBrightness);  

      if (idx >= 0) leds[idx] = star_color;
            
      if (drawRays == 1 || drawRays == 3) {
        // Стороны лучей
        uint8_t fadeBrightness = effectBrightness / 4 * 3;
        if (fadeBrightness > maxFadeBrightness) fadeBrightness = maxFadeBrightness;
        if (fadeBrightness < backBrightness) fadeBrightness = backBrightness;
        star_color = CHSV(color, 255, fadeBrightness);
        idx = getPixelNumber(xp+1, yp); 
        if (idx >= 0) leds[idx] = star_color;
        idx = getPixelNumber(xp-1, yp); 
        if (idx >= 0) leds[idx] = star_color;
        idx = getPixelNumber(xp, yp+1); 
        if (idx >= 0) leds[idx] = star_color;
        idx = getPixelNumber(xp, yp-1); 
        if (idx >= 0) leds[idx] = star_color;      
      }            
    }    
  }  
}

void stars2RoutineRelease() {
  if (starState == NULL) { delete [] starState; starState = NULL; }
  if (starBright == NULL) { delete [] starBright; starBright = NULL; }  
}

// ********************* БУДИЛЬНИК-РАССВЕТ *********************

int8_t   row, col;                 // Для эффекта спирали  - точка "головы" змейки, бегающей по спирали (первая змейка для круговой спирали)
int8_t   row2, col2;               // Для эффекта спирали  - точка "головы" змейки, бегающей по спирали (вторая змейка для плоской спирали)
int8_t   dir, dir2;                // Для эффекта спирали на плоскости - направление движения змейки: 0 - вниз; 1 - влево; 2 - вверх; 3 - вправо;
int8_t   range[4], range2[4];      // Для эффекта спирали на плоскости - границы разворачивания спирали; 
uint16_t tail[8], tail2[8];        // Для эффекта спирали на плоскости - позиции хвоста змейки. HiByte = x, LoByte=y
CHSV     tailColor;                // Цвет последней точки "хвоста" змейки. Этот же цвет используется для предварительной заливки всей матрицы
CHSV     tailColor2;               // Предварительная заливка нужна для корректного отображения часов поверх специальных эффектов будильника
bool     firstRowFlag;             // Флаг начала самого первого ряда первого кадра, чтобы не рисовать "хвост" змейки в предыдущем кадре, которого не было.
uint8_t  dawnBrightness;           // Текущая яркость будильника "рассвет"
uint8_t  tailBrightnessStep;       // Шаг приращения яркости будильника "рассвет"
uint8_t  dawnColorIdx;             // Индекс в массиве цвета "заливки" матрицы будильника "рассвет" (голова змейки)
uint8_t  dawnColorPrevIdx;         // Предыдущий индекс - нужен для корректного цвета отрисовки "хвоста" змейки, 
                                   // когда голова начинает новый кадр внизу матрицы, а хвост - вверху от предыдущего кадра
uint8_t  step_cnt;                 // Номер шага эффекта, чтобы определить какой длины "хвост" у змейки

// "Рассвет" - от красного к желтому - белому - голубому с плавным увеличением яркости;
// Яркость меняется по таймеру - на каждое срабатывание таймера - +1 к яркости.
// Диапазон изменения яркости - от MIN_DAWN_BRIGHT до MAX_DAWN_BRIGHT (количество шагов)
// Цветовой тон матрицы меняется каждые 16 шагов яркости 255 / 16 -> дает 16 индексов в массиве цветов
// Время таймера увеличения яркости - время рассвета DAWN_NINUTES на количество шагов приращения яркости
uint8_t dawnColorHue[16]  PROGMEM = {0, 16, 28, 36, 44, 52, 57, 62, 64, 66, 66, 64, 62, 60, 128, 128};              // Цвет заполнения - HUE змейки 1
uint8_t dawnColorSat[16]  PROGMEM = {255, 250, 245, 235, 225, 210, 200, 185, 170, 155, 130, 105, 80, 50, 25, 80};   // Цвет заполнения - SAT змейки 1
uint8_t dawnColorHue2[16] PROGMEM = {0, 16, 28, 36, 44, 52, 57, 62, 64, 66, 66, 64, 62, 60, 128, 128};              // Цвет заполнения - HUE змейки 2
uint8_t dawnColorSat2[16] PROGMEM = {255, 250, 245, 235, 225, 210, 200, 185, 170, 155, 130, 105, 80, 50, 25, 80};   // Цвет заполнения - SAT змейки 2

#define MIN_DAWN_BRIGHT   2        // Минимальное значение яркости будильника (с чего начинается)
#define MAX_DAWN_BRIGHT   255      // Максимальное значение яркости будильника (чем заканчивается)
uint8_t DAWN_NINUTES = 20;            // Продолжительность рассвета в минутах

void dawnProcedure() {

  if (loadingFlag) {
    dawnBrightness = MIN_DAWN_BRIGHT;
    // modeCode = MC_DAWN_ALARM;
    
    FastLED.clear();  // очистить
    FastLED.setBrightness(dawnBrightness);        

    if (realDawnDuration <= 0 || realDawnDuration > dawnDuration) realDawnDuration = dawnDuration;
    uint32_t interval = realDawnDuration * 60000UL / (MAX_DAWN_BRIGHT - MIN_DAWN_BRIGHT);
    dawnTimer.setInterval(interval);
  }

  // Пришло время увеличить яркость рассвета?
  if (dawnTimer.isReady() && dawnBrightness < 255) {
    dawnBrightness++;
    FastLED.setBrightness(dawnBrightness);
  }

  uint8_t effect = isAlarming ? alarmEffect : MC_DAWN_ALARM;
  if (effect == MC_DAWN_ALARM) {
    // Если устройство лампа (DEVICE_TYPE == 0) - матрица свернута в "трубу" - рассвет - огонек, бегущий вкруговую по спирали
    // Если устройство плоская матрица в рамке (DEVICE_TYPE == 1) - рассвет - огонек, бегущий по спирали от центра матрицы к краям на плоскости
    effect = DEVICE_TYPE == 0 ? MC_DAWN_ALARM_SPIRAL : MC_DAWN_ALARM_SQUARE;
  }

  // Если эффект "Лампа" и цвет - черный (остался от "выключено" - выбрать цвет лампы из сохраненных эффектов "Цветная лампа"
  if (effect == MC_FILL_COLOR && globalColor == 0) {
     set_globalColor(getColorInt(CHSV(getEffectSpeedValue(MC_FILL_COLOR), getEffectScaleParamValue(MC_FILL_COLOR), 255)));
  }
  if (effect == MC_FILL_COLOR && globalColor == 0) {
     set_globalColor(0xFFFFFF);
  }

  // Сформировать изображение эффекта
  processEffect(effect);
  
  // Сбрасывать флаг нужно ПОСЛЕ того как инициализированы: И процедура рассвета И применяемый эффект,
  // используемый в качестве рассвета
  loadingFlag = false;
}
  
// "Рассвет" по спирали, для ламп на круговой матрице (свернутой в трубу)
void dawnLampSpiral() {

  if (loadingFlag) {
    row = 0, col = 0;
    
    dawnBrightness = MIN_DAWN_BRIGHT; 
    tailBrightnessStep = 16;
    firstRowFlag = true;
    dawnColorIdx = 0;
    dawnColorPrevIdx = 0;
    
    tailColor = CHSV(0, 255, 255 - 8 * tailBrightnessStep); 
  }

  bool flag = true;
  int8_t x=col, y=row;
  
  if (!firstRowFlag) {
    fillAll(tailColor);
  }

  uint8_t tail_len = min(8, pWIDTH - 1);  
  for (uint8_t i=0; i<tail_len; i++) {
    x--;
    if (x < 0) { x = pWIDTH - 1; y--; }
    if (y < 0) {
      y = pHEIGHT - 1;
      flag = false;
      if (firstRowFlag) break;
    }

    uint8_t idx = y > row ? dawnColorPrevIdx : dawnColorIdx;
    uint8_t dawnHue = pgm_read_byte(&(dawnColorHue[idx]));
    uint8_t dawnSat = pgm_read_byte(&(dawnColorSat[idx]));
        
    tailColor = CHSV(dawnHue, dawnSat, 255 - i * tailBrightnessStep); 
    drawPixelXY(x,y, tailColor);  
  }
  
  if (flag) {
    firstRowFlag = false;
    dawnColorPrevIdx = dawnColorIdx;
  }
  if (dawnBrightness == 255 && tailBrightnessStep > 8) tailBrightnessStep -= 2;
  
  col++;
  if (col >= pWIDTH) {
    col = 0; row++;
  }
  
  if (row >= pHEIGHT) row = 0;  

  if (col == 0 && row == 0) {
    // Кол-во элементов массива - 16; Шагов яркости - 255; Изменение индекса каждые 16 шагов яркости. 
    dawnColorIdx = dawnBrightness >> 4;  
  }
}

// "Рассвет" по спирали на плоскости, для плоских матриц
void dawnLampSquare() {

  if (loadingFlag) {
    dir_mx = pWIDTH > pHEIGHT ? 0 : 1;                                   // 0 - квадратные сегменты расположены горизонтально, 1 - вертикально
    seg_num = dir_mx == 0 ? (pWIDTH / pHEIGHT) : (pHEIGHT / pWIDTH);     // вычисляем количество сегментов, умещающихся на матрице
    seg_size = dir_mx == 0 ? pHEIGHT : pWIDTH;                           // Размер квадратного сегмента (высота и ширина равны)
    seg_offset = ((dir_mx == 0 ? pWIDTH : pHEIGHT) - seg_size * seg_num) / (seg_num + 1); // смещение от края матрицы и между сегментами        

    SetStartPos();
    
    dawnBrightness = MIN_DAWN_BRIGHT; 
    tailBrightnessStep = 16;
    dawnColorIdx = 0;
    step_cnt = 0;

    memset(tail, 0, sizeof(uint16_t) * 8);
    memset(tail2, 0, sizeof(uint16_t) * 8);
    
    tailColor = CHSV(0, 255, 255 - 8 * tailBrightnessStep); 
  }
  
  int8_t x=col, y=row;
  int8_t x2=col2, y2=row2;

  fillAll(tailColor);
  
  step_cnt++;
  
  for (uint8_t i=7; i>0; i--) {
    tail[i]  = tail[i-1];
    tail2[i] = tail2[i-1];
  }
  tail[0]  = (uint)((int)x <<8 | (int)y);
  tail2[0] = (uint)((int)x2<<8 | (int)y2);

  uint8_t dawnHue  = pgm_read_byte(&(dawnColorHue[dawnColorIdx]));
  uint8_t dawnSat  = pgm_read_byte(&(dawnColorSat[dawnColorIdx]));
  uint8_t dawnHue2 = pgm_read_byte(&(dawnColorHue2[dawnColorIdx]));
  uint8_t dawnSat2 = pgm_read_byte(&(dawnColorSat2[dawnColorIdx]));

  for (uint8_t i=0; i < 8; i++) {
    
    tailColor  = CHSV(dawnHue, dawnSat, 255 - i * tailBrightnessStep); 
    tailColor2 = CHSV(dawnHue2, dawnSat2, 255 - i * tailBrightnessStep); 

    if (i<=step_cnt) {
      x  =  tail[i] >>8;  
      y  = tail[i]  & 0xff;
      x2 =  tail2[i]>>8;  
      y2 = tail2[i] & 0xff;
      for (uint8_t n=0; n < seg_num; n++) {
        uint8_t cx = dir_mx == 0 ? (seg_offset * (n + 1) + seg_size * n) : 0;
        uint8_t cy = dir_mx == 0 ? 0 : (seg_offset * (n + 1) + seg_size * n);
        drawPixelXY(x + cx,  y + cy,  tailColor);
        drawPixelXY(x2 + cx, y2 + cy, tailColor2);  
      }
    }
  }
  
  if (dawnBrightness == 255 && tailBrightnessStep > 8) tailBrightnessStep -= 2;

  switch(dir) {
    case 0: // вниз;
      row--;
      if (row <= range[dir]) {
        range[dir] = row - 2;
        dir++;
      }
      break;
    case 1: // влево;
      col--;
      if (col <= range[dir]) {
        range[dir] = col - 2;
        dir++;
      }
      break;
    case 2: // вверх;
      row++;
      if (row >= range[dir]) {
        range[dir] = row + 2;
        dir++;
      }
      break;
    case 3: // вправо;
      col++;
      if (col >= range[dir]) {
        range[dir] = col + 2;
        dir = 0;        
      }
      break;
  }
  
  switch(dir2) {
    case 0: // вниз;
      row2--;
      if (row2 <= range2[dir2]) {
        range2[dir2] = row2 - 2;
        dir2++;
      }
      break;
    case 1: // влево;
      col2--;
      if (col2 <= range2[dir2]) {
        range2[dir2] = col2 - 2;
        dir2++;
      }
      break;
    case 2: // вверх;
      row2++;
      if (row2 >= range2[dir2]) {
        range2[dir2] = row2 + 2;
        dir2++;
      }
      break;
    case 3: // вправо;
      col2++;
      if (col2 >= range2[dir2]) {
        range2[dir2] = col2 + 2;
        dir2 = 0;        
      }
      break;
  }
  
  bool out  = (col  < 0 || col  >= seg_size) && (row  < 0 || row  >= seg_size);
  bool out2 = (col2 < 0 || col2 >= seg_size) && (row2 < 0 || row2 >= seg_size);
  if (out && out2) {
    // Кол-во элементов массива - 16; Шагов яркости - 255; Изменение индекса каждые 16 шагов яркости. 
    dawnColorIdx = dawnBrightness >> 4;  
    SetStartPos();
    step_cnt = 0;
  }
}

void SetStartPos() {
  if (seg_size % 2 == 1) {
    col = seg_size / 2 + 1;
    col2 = col;
    row = seg_size / 2 + 1;
    row2 = row;
  } else {
    col = seg_size / 2 - 1;
    col2 = seg_size - col - 1;
    row = seg_size / 2 - 1;
    row2 = seg_size - row - 1;
  }
  
  dir = 2; dir2 = 0;
  
  // 0 - вниз; 1 - влево; 2 - вверх; 3 - вправо;
  range[0] = row-2; range[1] = col-2; range[2] = row+2; range[3] = col+2;
  range2[0] = row2-2; range2[1] = col2-2; range2[2] = row2+2; range2[3] = col2+2;
}

// ******************* ЛАМПА ********************

void fillColorProcedure() {
  if (loadingFlag) {
    // modeCode = MC_FILL_COLOR;
    loadingFlag = false;
  }
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  uint8_t bright = isAlarming && !isAlarmStopped 
    ? dawnBrightness
    : (specialMode ? specialBrightness : effectBrightness);

  if (globalColor == 0) {
    fillAll(CRGB::Black);
  } else {
    CRGB color = globalColor;
    color.nscale8_video(bright);
    fillAll(color);
  }
}

// ******************* PACIFICA ********************

//////////////////////////////////////////////////////////////////////////
//
// In this animation, there are four "layers" of waves of light.  
//
// Each layer moves independently, and each is scaled separately.
//
// All four wave layers are added together on top of each other, and then 
// another filter is applied that adds "whitecaps" of brightness where the 
// waves line up with each other more.  Finally, another pass is taken
// over the led array to 'deepen' (dim) the blues and greens.
//
// The speed and scale and motion each layer varies slowly within independent 
// hand-chosen ranges, which is why the code has a lot of low-speed 'beatsin8' functions
// with a lot of oddly specific numeric ranges.
//
// These three custom blue-green color palettes were inspired by the colors found in
// the waters off the southern coast of California, https://goo.gl/maps/QQgd97jjHesHZVxQ7
//
CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };

void pacificaRoutine()
{
  if (loadingFlag) {
    // modeCode = MC_PACIFICA;
    loadingFlag = false;
  }

  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);

  // uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  
  sLastms = ms;
  
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;

  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    uint8_t px = i % pWIDTH;
    uint8_t py = i / pWIDTH;
    idx = getPixelNumber(px, py);
    if (idx >= 0) leds[idx] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );
  
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t px = i % pWIDTH;
    uint8_t py = i / pWIDTH;
    idx = getPixelNumber(px, py);
    if (idx >= 0) {
      uint8_t l = leds[idx].getAverageLight();
      if( l > threshold) {
        uint8_t overage = l - threshold;
        uint8_t overage2 = qadd8( overage, overage);
        leds[idx] += CRGB( overage, overage2, qadd8( overage2, overage2));
      }
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t px = i % pWIDTH;
    uint8_t py = i / pWIDTH;
    idx = getPixelNumber(px, py);
    if (idx >= 0) {
      leds[idx].blue = scale8( leds[idx].blue,  145); 
      leds[idx].green= scale8( leds[idx].green, 200); 
      leds[idx] |= CRGB( 2, 5, 7);
    }
  }
}

// ********************** SHADOWS ***********************

void shadowsRoutine() {
  if (loadingFlag) {
    // modeCode = MC_SHADOWS;
    loadingFlag = false;
  }

  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);

  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t  sat8 = beatsin88( 87, 220, 250);
  uint8_t  brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t  msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  
  uint8_t  effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, map8(bri8, map(effectBrightness, 32, 255, 32,125), map(effectBrightness, 32,255, 125,250))); 
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    uint8_t px = pixelnumber % pWIDTH;
    uint8_t py = pixelnumber / pWIDTH;
    idx = getPixelNumber(px, py);
    if (idx >= 0) nblend( leds[idx], newcolor, 64);
  }
}

// ***************************** ПАЛИТРА *****************************

#define BLOCK_SIZE 4       // Размер квадратика палитры
#define FADE_IN_STEPS 16   // За сколько шагов плашка появляется на экране    
#define FADE_OUT_STEPS 32  // За сколько шагов плашка убирается с экрана    
#define BLOCK_ON_START 5   // Сколько блоков сразу появлять в начале эффекта

uint8_t num_x, num_y, off_x, off_y;

uint8_t **palette_h; // Н in CHSV
uint8_t **palette_s; // S in CHSV
uint8_t **block_sta; // Block state: // 0 - появление; 1 - исчезновение; 2 - пауза перед появлением 3 - пауза перед удалением
uint8_t **block_dur; // время паузы блока

void paletteRoutine() {

  if (loadingFlag) {
    // modeCode = MC_PALETTE;
    loadingFlag = false;
    
    num_x = pWIDTH / BLOCK_SIZE;
    num_y = pHEIGHT / BLOCK_SIZE;
    off_x = (pWIDTH - BLOCK_SIZE * num_x) / 2;
    off_y = (pHEIGHT - BLOCK_SIZE * num_y) / 2;

    dir_mx = pWIDTH > pHEIGHT ? 0 : 1;                                   // 0 - квадратные сегменты расположены горизонтально, 1 - вертикально
    seg_num = dir_mx == 0 ? (pWIDTH / pHEIGHT) : (pHEIGHT / pWIDTH);     // вычисляем количество сегментов, умещающихся на матрице

    if (palette_h == NULL) { palette_h = new uint8_t*[num_x]; for (uint8_t i = 0; i < num_x; i++) { palette_h[i] = new uint8_t [num_y]; }}
    if (palette_s == NULL) { palette_s = new uint8_t*[num_x]; for (uint8_t i = 0; i < num_x; i++) { palette_s[i] = new uint8_t [num_y]; }}
    if (block_sta == NULL) { block_sta = new uint8_t*[num_x]; for (uint8_t i = 0; i < num_x; i++) { block_sta[i] = new uint8_t [num_y]; }}
    if (block_dur == NULL) { block_dur = new uint8_t*[num_x]; for (uint8_t i = 0; i < num_x; i++) { block_dur[i] = new uint8_t [num_y]; }}

    // Для всех блоков определить состояние - "ожидание появления
    for (uint8_t c = 0; c < num_x; c++) {
      for (uint8_t r = 0; r < num_y; r++) {
        block_sta[c][r] = 2;                // Состояние - пауза перед появлением
        block_dur[c][r] = random8(25,125);  // Длительность паузы
      }
    }

    // Для некоторого количества начальных - установить "За шаг до появления"
    // При первом же проходе состояние переключится на "появление"
    for (uint8_t i = 0; i < BLOCK_ON_START * seg_num; i++) {
      uint8_t c = random8(0, num_x - 1);
      uint8_t r = random8(0, num_y - 1);
      block_dur[c][r] = 1;                  // Счетчик до начала появления
    }
    FastLED.clear();
  }
  
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  for (uint8_t c = 0; c < num_x; c++) {
    uint8_t block_x = off_x + c * BLOCK_SIZE;
    for (uint8_t r = 0; r < num_y; r++) {    
      
      uint8_t block_y = off_y + r * BLOCK_SIZE;
      uint8_t h = palette_h[c][r];      
      uint8_t s = palette_s[c][r];

      // Проверить состояние блока
      if (block_sta[c][r] > 1) {
        
        // Одна из пауз (2 или 3) - пауза перед появлением или перед исчезновением
        // Уменьшить время паузы. Если стало 0 - переключить с паузы на появление / исчезновение
         block_dur[c][r] -= 1;
         if (block_dur[c][r] == 0) {
           block_sta[c][r] -= 2;     // 3->1 - исчезать; 2->0 появлять за указанное количество шагов
           if (block_sta[c][r] == 0) {
             block_dur[c][r] = FADE_IN_STEPS;    // Количество шагов появления блока
             palette_h[c][r] = random8(0,255);   // Цвет нового блока
             palette_s[c][r] = random8(112,254); // Насыщенность цвета нового блока
           } else { 
             block_dur[c][r] = FADE_OUT_STEPS;  // Кол-во шагов убирания блока
           }  
         }
      }
      
      if (block_sta[c][r] < 2) {

        // В процессе появления или исчезновения (0 или 1)
        // Выполнить один шаг появления / исчезновения блока
        uint8_t fade_dir = block_sta[c][r]; // 0 - появляться, 1 - исчезать
        uint8_t fade_step = block_dur[c][r];

        // Яркость блока
        uint8_t bri = fade_dir == 0
           ? map(fade_step, 0,FADE_IN_STEPS,  0,effectBrightness)
           : map(fade_step, 0,FADE_OUT_STEPS, effectBrightness,0);

        // Нарисовать блок   
        for (uint8_t i=0; i<BLOCK_SIZE; i++) {        
          for (uint8_t j=0; j<BLOCK_SIZE; j++) {
            
            //uint8_t k = fade_dir == 0 ? (2 * i*j) : (2 * (BLOCK_SIZE * BLOCK_SIZE - i*j));
            //uint8_t bri2 = (bri > k ? bri - k : 0);
            CHSV color = CHSV(h, s, bri); // bri2

            uint8_t xx = block_x + j;
            uint8_t yy = block_y + BLOCK_SIZE - i - 1;
            if (xx < pWIDTH && yy < pHEIGHT) {
              idx = getPixelNumber(xx, yy);
              if (idx >= 0) leds[idx] = color;
            }
          }
        }

        // Шаг появления - обработан
        block_dur[c][r] -= 1;

        // Весь процесс появления / исчезновения выполнен?
        // Сменить статус блока
        if (block_dur[c][r] == 0) {
           // Появление / исчезновение закончено
           block_sta[c][r] = block_sta[c][r] == 0 ? 3 : 2; // вкл паузу перед исчезновением после появления или паузу перед появлением после исчезновения
           block_dur[c][r] = random8(25,125);              // Длительность паузы (циклов обращения палитры)
        }        
      }      
    }
  }
}

void paletteRoutineRelease() {
  if (block_dur != NULL) { for( uint8_t i = 0; i < num_x; i++ ) { delete [] block_dur[i];} delete [] block_dur; block_dur = NULL; }
  if (block_sta != NULL) { for( uint8_t i = 0; i < num_x; i++ ) { delete [] block_sta[i];} delete [] block_sta; block_sta = NULL; }
  if (palette_s != NULL) { for( uint8_t i = 0; i < num_x; i++ ) { delete [] palette_s[i];} delete [] palette_s; palette_s = NULL; }
  if (palette_h != NULL) { for( uint8_t i = 0; i < num_x; i++ ) { delete [] palette_h[i];} delete [] palette_h; palette_h = NULL; }
}


// ****************************** ANALYZER *****************************

// цвета высоты полос спектра.
#define COLOR1    HUE_GREEN
#define COLOR2    HUE_YELLOW
#define COLOR3    HUE_ORANGE
#define COLOR4    HUE_RED
#define MAX_COLOR HUE_RED // цвет точек максимума

// анимация
#define SMOOTH 0.3        // плавность движения столбиков (0 - 1)

// точки максимума
#define MAX_DOTS 1        // включить/выключить отрисовку точек максимума (1 вкл, 0 выкл)
#define FALL_DELAY 50     // скорость падения точек максимума (задержка, миллисекунды)
#define FALL_PAUSE 700    // пауза перед падением точек максимума, миллисекунды

uint32_t gainTimer, fallTimer;
uint8_t  maxValue;
bool     fallFlag;

uint32_t *timeLevel;
uint8_t  *posOffset;       // Массив данных для отображения на матрице
int16_t  *maxLevel;
uint8_t  *posLevel_old;

uint8_t  st = 0;
    
// -------------------------------------------------------------------------------------

void analyzerRoutine() {

  static int16_t MAX_LEVEL = (pHEIGHT + pHEIGHT / 4);
  static uint8_t SIN_WIDTH = (pWIDTH / 8);

  if (loadingFlag) {
    // modeCode = MC_ANALYZER;
    loadingFlag = false;

    if (timeLevel    == NULL) { timeLevel    = new uint32_t[pWIDTH]; }
    if (posOffset    == NULL) { posOffset    = new uint8_t[pWIDTH]; }
    if (maxLevel     == NULL) { maxLevel     = new int16_t[pWIDTH]; }
    if (posLevel_old == NULL) { posLevel_old = new uint8_t[pWIDTH]; }
    
    for (uint8_t i = 0; i < pWIDTH; i++) {
      maxLevel[i] = 0;
      posLevel_old[i] = 0;
    }

    st = 0;
    phase = 0;
    FastLED.clear();
  }
  
  if (phase == 0) {
    // Движение волны слева направо
    for (uint8_t i = 0; i < pWIDTH; i++) {
      posOffset[i] = (i < st || i >= st + SIN_WIDTH - (SIN_WIDTH / 4))
        ? 0
        : map8(sin8(map(i, st,st + SIN_WIDTH, 0,255)), 1, pHEIGHT + pHEIGHT / 2);
    }
  } else 

  if (phase == 2) {
    // Движение волны справа налево
    for (uint8_t i = 0; i < pWIDTH; i++) {
        posOffset[i] = (i < pWIDTH - st || i > pWIDTH - st + SIN_WIDTH)
          ? 0
          : map8(sin8(map(i, pWIDTH - st, pWIDTH - st + SIN_WIDTH, 0, 255)), 1, pHEIGHT + pHEIGHT / 2);
    }
  } else

  if (phase == 1 || phase == 3) {
    // Пауза, даем "отстояться" пикам
    for (uint8_t i = 0; i < pWIDTH; i++) {
      posOffset[i] = 0;
    }    
  } else
  
  if (phase >= 4) {
    // Случайные движения - "музыка"
    for (uint8_t i = 0; i < pWIDTH; i++) {
      posOffset[i] = random8(1,MAX_LEVEL);    
    }
  }

  st++;
  if (st >= pWIDTH && phase < 4) {    
    phase++;
    st = phase % 2 == 1 ? pWIDTH / 2 : 0;
  }
  
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  
  maxValue = 0;
  FastLED.clear();  // очистить матрицу
  
  for (uint8_t pos = 0; pos < pWIDTH; pos++) {    // для каждого столбца матрицы
    uint8_t posLevel = posOffset[pos];

    // найти максимум из пачки тонов
    if (posLevel > maxValue) maxValue = posLevel;

    // фильтрация длины столбиков, для их плавного движения
    posLevel = posLevel * SMOOTH + posLevel_old[pos] * (1 - SMOOTH);
    posLevel_old[pos] = posLevel;

    // преобразовать значение величины спектра в диапазон 0..HEIGHT с учётом настроек
    posLevel = constrain(posLevel, 1, pHEIGHT - 1);

    if (posLevel > 0) {
      for (uint8_t j = 0; j < posLevel; j++) {
        CHSV color;
        if      (j < map( 5, 0,16, 0,pHEIGHT)) color = CHSV(COLOR1, 255, effectBrightness);
        else if (j < map(10, 0,16, 0,pHEIGHT)) color = CHSV(COLOR2, 255, effectBrightness);
        else if (j < map(13, 0,16, 0,pHEIGHT)) color = CHSV(COLOR3, 255, effectBrightness);
        else if (j < map(15, 0,16, 0,pHEIGHT)) color = CHSV(COLOR4, 255, effectBrightness);

        drawPixelXY(pos, j, color);
      }
    }

    if (posLevel > 0 && posLevel > maxLevel[pos]) {    // если для этой полосы есть максимум, который больше предыдущего
      maxLevel[pos] = posLevel;                        // запомнить его
      timeLevel[pos] = millis();                       // запомнить время
    }

    // если точка максимума выше нуля (или равна ему) - включить пиксель
    if (maxLevel[pos] >= 0 && MAX_DOTS) {
      drawPixelXY(pos, maxLevel[pos], CHSV(MAX_COLOR, 255, effectBrightness));
    }

    if (fallFlag) {                                           // если падаем на шаг
      if ((uint32_t)millis() - timeLevel[pos] > FALL_PAUSE) {     // если максимум держался на своей высоте дольше FALL_PAUSE
        if (maxLevel[pos] >= 0) maxLevel[pos]--;              // уменьшить высоту точки на 1
        // внимание! Принимает минимальное значение -1 !
      }
    }
  }

  fallFlag = 0;                                 // сбросить флаг падения
  if (millis() - fallTimer > FALL_DELAY) {      // если настало время следующего падения
    fallFlag = 1;                               // поднять флаг
    fallTimer = millis();
  }
}

void analyzerRoutineRelease() {
  if (posLevel_old != NULL) { delete[] posLevel_old; posLevel_old = NULL; }
  if (maxLevel != NULL)     { delete[] maxLevel;     maxLevel = NULL; }
  if (posOffset != NULL)    { delete[] posOffset;    posOffset = NULL; }
  if (timeLevel != NULL)    { delete[] timeLevel;    timeLevel = NULL; }
}

// ****************************** СИНУСЫ *****************************

void prizmataRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    dir_mx = pWIDTH >= pHEIGHT ? 0 : 1;                                 // 0 - квадратные сегменты расположены горизонтально, 1 - вертикально
    // modeCode = MC_PRIZMATA;
  }

  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);
  
  EVERY_N_MILLIS(33) {
     hue++;
  }
  
  FastLED.clear();

  // Отрисовка режима происходит на максимальной скорости. Значение effectSpeed влияет на параметр BPM функции beatsin8
  uint8_t spd = map8(255-getEffectSpeedValue(MC_PRIZMATA), 12, 64);   
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  if (dir_mx == 0) {
    for (uint8_t x = 0; x < pWIDTH; x++) {
      uint8_t y = beatsin8(spd + x, 0,pHEIGHT-1);
      drawPixelXY(x, y, ColorFromPalette(RainbowColors_p, x * 7 + hue, effectBrightness));
    }
  } else {
    for (uint8_t y = 0; y < pHEIGHT; y++) {
      uint8_t x = beatsin8(spd + y, 0, pWIDTH-1);
      drawPixelXY(x, y, ColorFromPalette(RainbowColors_p, x * 7 + hue, effectBrightness));
    }
  }
}

// *************************** ВЫШИВАНКА **************************

// ------ Эффект "Вышиванка" 
// (с) проект Aurora "Munch"
// adopted/updated by kostyamat

int8_t  count = 0;
uint8_t flip = 0;
uint8_t generation = 0;
uint8_t rnd = 4; //1-8
uint8_t mic[2];
uint8_t minDimLocal = maxDim > 32 ? 32 : 16;

const uint8_t width_adj = (pWIDTH < pHEIGHT ? (pHEIGHT - pWIDTH) / 2 : 0);
const uint8_t height_adj = (pHEIGHT < pWIDTH ? (pWIDTH - pHEIGHT) / 2 : 0);
const uint8_t maxDim_steps = 256 / maxDim;

void munchRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    //modeCode = MC_MUNCH;
    generation = 0;
    dir = 1;
    count = 0;
    flip = 0;
    FastLED.clear();
  }

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  for (uint8_t x = 0; x < minDimLocal; x++) {
    for (uint8_t y = 0; y < minDimLocal; y++) {
      CRGB color = (x ^ y ^ flip) < count ? ColorFromPalette(RainbowColors_p, ((x ^ y) << rnd) + generation, effectBrightness) : CRGB::Black;
      if (x < pWIDTH and y < pHEIGHT) leds[XY(x, y)] = color;
      if (x + minDimLocal < pWIDTH and y < pHEIGHT) leds[XY(x + minDimLocal, y)] = color;
      if (y + minDimLocal < pHEIGHT and x < pWIDTH) leds[XY(x, y + minDimLocal)] = color;
      if (x + minDimLocal < pWIDTH and y + minDimLocal < pHEIGHT) leds[XY(x + minDimLocal, y + minDimLocal)] = color;
    }
  }

  count += dir;

  if (count <= 0 || count >= mic[0]) {
    dir = -dir;
    if (count <= 0) {
      mic[0] = mic[1];
      if (flip == 0)
        flip = mic[1] - 1;
      else
        flip = 0;
    }
  }
  
  generation++;
  mic[1] = minDimLocal;
}

// *************************** ДОЖДЬ **************************

CRGB rainColor = CRGB(60,80,90);
CRGB lightningColor = CRGB(72,72,80);
CRGBPalette16 rain_p( CRGB::Black, rainColor);
CRGBPalette16 rainClouds_p( CRGB::Black, CRGB(15,24,24), CRGB(9,15,15), CRGB::Black );

uint8_t cloudHeight = pHEIGHT * 0.2 + 1;
uint8_t **noise3d;
uint8_t *cloud;

void rain(uint8_t backgroundDepth, uint8_t spawnFreq, uint8_t tailLength, bool splashes, bool clouds, bool storm) {
  
  static uint16_t noiseX = random16();
  static uint16_t noiseY = random16();
  static uint16_t noiseZ = random16();

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  
  fadeToBlackBy( leds, NUM_LEDS, 255-tailLength);

  // Loop for each column individually
  for (uint8_t x = 0; x < pWIDTH; x++) {
    // Step 1.  Move each dot down one cell
    for (uint8_t i = 0; i < pHEIGHT; i++) {
      if (noise3d[x][i] >= backgroundDepth) {  // Don't move empty cells
        if (i > 0) noise3d[x][wrapY(i-1)] = noise3d[x][i];
        noise3d[x][i] = 0;
      }
    }

    // Step 2.  Randomly spawn new dots at top
    if (random8() < spawnFreq) {
      noise3d[x][pHEIGHT-1] = random(backgroundDepth, effectBrightness);
    }

    // Step 3. Map from tempMatrix cells to LED colors
    for (uint8_t y = 0; y < pHEIGHT; y++) {
      if (noise3d[x][y] >= backgroundDepth) {  // Don't write out empty cells
        leds[XY(x,y)] = ColorFromPalette(rain_p, noise3d[x][y], effectBrightness);
      }
    }

    // Step 4. Add splash if called for
    if (splashes) {
      // FIXME, this is broken
      uint8_t j = line[x];
      uint8_t v = noise3d[x][0];

      if (j >= backgroundDepth) {
        leds[XY(wrapX(x-2),0)] = ColorFromPalette(rain_p, j/3, effectBrightness);
        leds[XY(wrapX(x+2),0)] = ColorFromPalette(rain_p, j/3, effectBrightness);
        line[x] = 0;   // Reset splash
      }

      if (v >= backgroundDepth) {
        leds[XY(wrapX(x-1),1)] = ColorFromPalette(rain_p, v/2, effectBrightness);
        leds[XY(wrapX(x+1),1)] = ColorFromPalette(rain_p, v/2, effectBrightness);
        line[x] = v; // Prep splash for next frame
      }
    }

    // Step 5. Add lightning if called for
    if (storm && random16() < 72) {
      
      uint8_t *lightning = (uint8_t *) malloc(pWIDTH * pHEIGHT);
      
      if (lightning != NULL) { 
        lightning[scale8(random8(), pWIDTH-1) + (pHEIGHT-1) * pWIDTH] = 255;  // Random starting location
        for(uint8_t ly = pHEIGHT-1; ly > 1; ly--) {
          for (uint8_t lx = 1; lx < pWIDTH-1; lx++) {
            if (lightning[lx + ly * pWIDTH] == 255) {
              lightning[lx + ly * pWIDTH] = 0;
              uint8_t dir = random8(4);
              switch (dir) {
                case 0:
                  leds[XY(lx+1,ly-1)] = lightningColor;
                  lightning[(lx+1) + (ly-1) * pWIDTH] = 255; // move down and right
                break;
                case 1:
                  leds[XY(lx,ly-1)] = CRGB(128,128,128);   // я без понятия, почему у верхней молнии один оттенок, а у остальных - другой
                  lightning[lx + (ly-1) * pWIDTH] = 255;    // move down
                break;
                case 2:
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx-1) + (ly-1) * pWIDTH] = 255; // move down and left
                break;
                case 3:
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx-1) + (ly-1) * pWIDTH] = 255; // fork down and left
                  leds[XY(lx-1,ly-1)] = CRGB(128,128,128);
                  lightning[(lx+1) + (ly-1) * pWIDTH] = 255; // fork down and right
                break;
              }
            }
          }
        }
        free(lightning);
      } else {
        DEBUGLN("lightning malloc failed"); 
      }
    }

    // Step 6. Add clouds if called for
    if (clouds) {
      uint16_t noiseScale = 250;  // A value of 1 will be so zoomed in, you'll mostly see solid colors. A value of 4011 will be very zoomed out and shimmery
      
      if (cloud != NULL) {      
        int16_t xoffset = noiseScale * x + hue;  
        for(uint8_t z = 0; z < cloudHeight; z++) {
          int16_t yoffset = noiseScale * z - hue;
          uint8_t dataSmoothing = 192;
          uint8_t noiseData = qsub8(inoise8(noiseX + xoffset,noiseY + yoffset,noiseZ),16);
          noiseData = qadd8(noiseData,scale8(noiseData,39));
          cloud[x * cloudHeight + z] = scale8( cloud[x * cloudHeight + z], dataSmoothing) + scale8( noiseData, 256 - dataSmoothing);
          nblend(leds[XY(x,pHEIGHT-z-1)], ColorFromPalette(rainClouds_p, cloud[x * cloudHeight + z], effectBrightness), (cloudHeight-z)*(250/cloudHeight));
        }
      } else { 
        DEBUGLN("cloud malloc failed"); 
      } 
      noiseZ++;
    }
  }
}

void rainRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    //modeCode = MC_RAIN;
    cloudHeight = pHEIGHT * 0.2 + 1; // это уже 20% с лишним, но на высоких матрицах будет чуть меньше
    
    if (noise3d == NULL) { noise3d = new uint8_t*[pWIDTH]; for (uint8_t i = 0; i < pWIDTH; i++) { noise3d[i] = new uint8_t [pHEIGHT]; }}
    if (line == NULL)    { line = new uint8_t[pWIDTH]; }
    if (cloud == NULL)   { cloud = new uint8_t[pWIDTH * cloudHeight]; }
  }

  uint8_t intensity = beatsin8(map8(getEffectScaleParamValue(MC_RAIN),2,6), 4, 60);
  
  // ( Depth of dots, frequency of new dots, length of tails, splashes, clouds, ligthening )
  if (intensity <= 35) 
    // Lightweight
    rain(60, intensity, 10, true, true, false);
  else
    // Stormy
    rain(0, intensity, 10, true, true, true);
}

void rainRoutineRelease() {
  if (cloud != NULL)   { delete[] cloud; cloud = NULL; }
  if (line != NULL)    { delete[] line; line = NULL; }
  if (noise3d != NULL) { for (uint8_t i = 0; i < pWIDTH; i++) delete[] noise3d[i]; delete[] noise3d; noise3d = NULL; }
}

// ********************** ОГОНЬ-2 (КАМИН) *********************

void fire2Routine() {
  if (loadingFlag) {
    loadingFlag = false;
    //modeCode = MC_FIRE2;
    if (noise3d == NULL) { noise3d = new uint8_t*[pWIDTH]; for (uint8_t i = 0; i < pWIDTH; i++) { noise3d[i] = new uint8_t [pHEIGHT]; } }
  }

  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);

  static uint8_t FIRE_BASE = pHEIGHT/6 > 6 ? 6 : pHEIGHT/6+1;
  
  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.  
  uint8_t cooling = map8(getEffectSpeedValue(MC_FIRE2), 70, 100);     
  
  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  uint8_t sparking = map8(getEffectScaleParamValue(MC_FIRE2), 90, 150);
  
  // SMOOTHING; How much blending should be done between frames
  // Lower = more blending and smoother flames. Higher = less blending and flickery flames
  const uint8_t fireSmoothing = 80;
  
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  uint8_t effectBrightness = map8(getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode)), 32,128);

  // Loop for each column individually
  for (uint8_t x = 0; x < pWIDTH; x++) {
    
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < pHEIGHT; i++) {
      noise3d[x][i] = qsub8(noise3d[x][i], random(0, ((cooling * 10) / pHEIGHT) + 2));
    }

    // Step 2.  Heat from  cell drifts 'up' and diffuses a little
    for (uint8_t k = pHEIGHT; k > 1; k--) {
      noise3d[x][wrapY(k)] = (noise3d[x][k - 1] + noise3d[x][wrapY(k - 2)] + noise3d[x][wrapY(k - 2)]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < sparking) {
      uint8_t j = random8(FIRE_BASE);
      noise3d[x][j] = qadd8(noise3d[x][j], random(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    // Blend new data with previous frame. Average data between neighbouring pixels
    for (uint8_t y = 0; y < pHEIGHT; y++)
      nblend(leds[XY(x,y)], ColorFromPalette(HeatColors_p, ((noise3d[x][y]*0.7) + (noise3d[wrapX(x+1)][y]*0.3)), effectBrightness), fireSmoothing);
  }
}

void fire2RoutineRelease() {
  if (noise3d != NULL) { for (uint8_t i = 0; i < pWIDTH; i++) delete[] noise3d[i]; delete[] noise3d; noise3d = NULL; }
}

// ************************** СТРЕЛКИ *************************
int8_t   arrow_x[4], arrow_y[4], stop_x[4], stop_y[4];
uint8_t  arrow_direction;            // 0x01 - слева направо; 0x02 - снизу вверх; 0х04 - справа налево; 0х08 - сверху вниз
uint8_t  arrow_mode, arrow_mode_orig;// 0 - по очереди все варианты
                                     // 1 - по очереди от края до края экрана; 
                                     // 2 - одновременно по горизонтали навстречу к центру, затем одновременно по вертикали навстречу к центру
                                     // 3 - одновременно все к центру
                                     // 4 - по два (горизонталь / вертикаль) все от своего края к противоположному, стрелки смещены от центра на 1/3
                                     // 5 - одновременно все от своего края к противоположному, стрелки смещены от центра на 1/3
bool     arrow_complete, arrow_change_mode;
uint8_t  arrow_hue[4];
uint8_t  arrow_play_mode_count[6];        // Сколько раз проигрывать полностью каждый режим если вариант 0 - текущий счетчик
uint8_t  arrow_play_mode_count_orig[6];   // Сколько раз проигрывать полностью каждый режим если вариант 0 - исходные настройки

void arrowsRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    //modeCode = MC_ARROWS;
    FastLED.clear();
    arrow_complete = false;
    arrow_mode_orig = (specialTextEffectParam >= 0) ? specialTextEffectParam : getEffectScaleParamValue2(MC_ARROWS);
    
    arrow_mode = (arrow_mode_orig == 0 || arrow_mode_orig > 5) ? random8(1,5) : arrow_mode_orig;
    arrow_play_mode_count_orig[0] = 0;
    arrow_play_mode_count_orig[1] = 4;  // 4 фазы - все стрелки показаны по кругу один раз - переходить к следующему ->
    arrow_play_mode_count_orig[2] = 4;  // 2 фазы - гориз к центру (1), затем верт к центру (2) - обе фазы повторить по 2 раза -> 4
    arrow_play_mode_count_orig[3] = 4;  // 1 фаза - все к центру (1) повторить по 4 раза -> 4
    arrow_play_mode_count_orig[4] = 4;  // 2 фазы - гориз к центру (1), затем верт к центру (2) - обе фазы повторить по 2 раза -> 4
    arrow_play_mode_count_orig[5] = 4;  // 1 фаза - все сразу (1) повторить по 4 раза -> 4
    for (uint8_t i=0; i<6; i++) {
      arrow_play_mode_count[i] = arrow_play_mode_count_orig[i];
    }
    arrowSetupForMode(arrow_mode, true);
  }
  
  uint8_t effectBrightness = map8(getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode)), 32,255);  

  fader(65);
  CHSV color;
  
  // движение стрелки - cлева направо
  if ((arrow_direction & 0x01) > 0) {
    color = CHSV(arrow_hue[0], 255, effectBrightness);
    for (int8_t x = 0; x <= 4; x++) {
      for (int8_t y = 0; y <= x; y++) {    
        if (arrow_x[0] - x >= 0 && arrow_x[0] - x <= stop_x[0]) { 
          CHSV clr = (x < 4 || (x == 4 && y < 2)) ? color : CHSV(0,0,0);
          drawPixelXY(arrow_x[0] - x, arrow_y[0] - y, clr);
          drawPixelXY(arrow_x[0] - x, arrow_y[0] + y, clr);
        }
      }    
    }
    arrow_x[0]++;
  }

  // движение стрелки - cнизу вверх
  if ((arrow_direction & 0x02) > 0) {
    color = CHSV(arrow_hue[1], 255, effectBrightness);
    for (int8_t y = 0; y <= 4; y++) {
      for (int8_t x = 0; x <= y; x++) {    
        if (arrow_y[1] - y >= 0 && arrow_y[1] - y <= stop_y[1]) { 
          CHSV clr = (y < 4 || (y == 4 && x < 2)) ? color : CHSV(0,0,0);
          drawPixelXY(arrow_x[1] - x, arrow_y[1] - y, clr);
          drawPixelXY(arrow_x[1] + x, arrow_y[1] - y, clr);
        }
      }    
    }
    arrow_y[1]++;
  }

  // движение стрелки - справа налево
  if ((arrow_direction & 0x04) > 0) {
    color = CHSV(arrow_hue[2], 255, effectBrightness);
    for (int8_t x = 0; x <= 4; x++) {
      for (int8_t y = 0; y <= x; y++) {    
        if (arrow_x[2] + x >= stop_x[2] && arrow_x[2] + x < pWIDTH) { 
          CHSV clr = (x < 4 || (x == 4 && y < 2)) ? color : CHSV(0,0,0);
          drawPixelXY(arrow_x[2] + x, arrow_y[2] - y, clr);
          drawPixelXY(arrow_x[2] + x, arrow_y[2] + y, clr);
        }
      }    
    }
    arrow_x[2]--;
  }

  // движение стрелки - сверху вниз
  if ((arrow_direction & 0x08) > 0) {
    color = CHSV(arrow_hue[3], 255, effectBrightness);
    for (int8_t y = 0; y <= 4; y++) {
      for (int8_t x = 0; x <= y; x++) {    
        if (arrow_y[3] + y >= stop_y[3] && arrow_y[3] + y < pHEIGHT) { 
          CHSV clr = (y < 4 || (y == 4 && x < 2)) ? color : CHSV(0,0,0);
          drawPixelXY(arrow_x[3] - x, arrow_y[3] + y, clr);
          drawPixelXY(arrow_x[3] + x, arrow_y[3] + y, clr);
        }
      }    
    }
    arrow_y[3]--;
  }

  // Проверка завершения движения стрелки, переход к следующей фазе или режиму
  
  switch (arrow_mode) {

    case 1:
      // Последовательно - слева-направо -> снизу вверх -> справа налево -> сверху вниз и далее по циклу
      // В каждый момент времени активна только одна стрелка, если она дошла до края - переключиться на следующую и задать ее начальные координаты
      arrow_complete = false;
      switch (arrow_direction) {
        case 1: arrow_complete = arrow_x[0] > stop_x[0]; break;
        case 2: arrow_complete = arrow_y[1] > stop_y[1]; break;
        case 4: arrow_complete = arrow_x[2] < stop_x[2]; break;
        case 8: arrow_complete = arrow_y[3] < stop_y[3]; break;
      }

      arrow_change_mode = false;
      if (arrow_complete) {
        arrow_direction = (arrow_direction << 1) & 0x0F;
        if (arrow_direction == 0) arrow_direction = 1;
        if (arrow_mode_orig == 0) {
          arrow_play_mode_count[1]--;
          if (arrow_play_mode_count[1] == 0) {
            arrow_play_mode_count[1] = arrow_play_mode_count_orig[1];
            arrow_mode = random8(1,5);
            arrow_change_mode = true;
          }
        }

        arrowSetupForMode(arrow_mode, arrow_change_mode);
      }
      break;

    case 2:
      // Одновременно горизонтальные навстречу до половины экрана
      // Затем одновременно вертикальные до половины экрана. Далее - повторять
      arrow_complete = false;
      switch (arrow_direction) {
        case  5: arrow_complete = arrow_x[0] > stop_x[0]; break;   // Стрелка слева и справа встречаются в центре одновременно - проверять только стрелку слева
        case 10: arrow_complete = arrow_y[1] > stop_y[1]; break;   // Стрелка снизу и сверху встречаются в центре одновременно - проверять только стрелку снизу
      }

      arrow_change_mode = false;
      if (arrow_complete) {
        arrow_direction = arrow_direction == 5 ? 10 : 5;
        if (arrow_mode_orig == 0) {
          arrow_play_mode_count[2]--;
          if (arrow_play_mode_count[2] == 0) {
            arrow_play_mode_count[2] = arrow_play_mode_count_orig[2];
            arrow_mode = random8(1,5);
            arrow_change_mode = true;
          }
        }
        
        arrowSetupForMode(arrow_mode, arrow_change_mode);
      }
      break;

    case 3:
      // Одновременно со всех сторон к центру
      // Завершение кадра режима - когда все стрелки собрались в центре.
      // Проверять стрелки по самой длинной стороне
      if (pWIDTH >= pHEIGHT)
        arrow_complete = arrow_x[0] > stop_x[0];
      else 
        arrow_complete = arrow_y[1] > stop_y[1];
        
      arrow_change_mode = false;
      if (arrow_complete) {
        if (arrow_mode_orig == 0) {
          arrow_play_mode_count[3]--;
          if (arrow_play_mode_count[3] == 0) {
            arrow_play_mode_count[3] = arrow_play_mode_count_orig[3];
            arrow_mode = random8(1,5);
            arrow_change_mode = true;
          }
        }
        
        arrowSetupForMode(arrow_mode, arrow_change_mode);
      }
      break;

    case 4:
      // Одновременно слева/справа от края до края со смещением горизонтальной оси на 1/3 высоты, далее
      // одновременно снизу/сверху от края до края со смещением вертикальной оси на 1/3 ширины
      // Завершение кадра режима - когда все стрелки собрались в центре.
      // Проверять стрелки по самой длинной стороне
      switch (arrow_direction) {
        case  5: arrow_complete = arrow_x[0] > stop_x[0]; break;   // Стрелка слева и справа движутся и достигают края одновременно - проверять только стрелку слева
        case 10: arrow_complete = arrow_y[1] > stop_y[1]; break;   // Стрелка снизу и сверху движутся и достигают края одновременно - проверять только стрелку снизу
      }

      arrow_change_mode = false;
      if (arrow_complete) {
        arrow_direction = arrow_direction == 5 ? 10 : 5;
        if (arrow_mode_orig == 0) {
          arrow_play_mode_count[4]--;
          if (arrow_play_mode_count[4] == 0) {
            arrow_play_mode_count[4] = arrow_play_mode_count_orig[4];
            arrow_mode = random8(1,5);
            arrow_change_mode = true;
          }
        }
        
        arrowSetupForMode(arrow_mode, arrow_change_mode);
      }
      break;

    case 5:
      // Одновременно со всех сторон от края до края со смещением горизонтальной оси на 1/3 высоты, далее
      // Проверять стрелки по самой длинной стороне
      if (pWIDTH >= pHEIGHT)
        arrow_complete = arrow_x[0] > stop_x[0];
      else 
        arrow_complete = arrow_y[1] > stop_y[1];

      arrow_change_mode = false;
      if (arrow_complete) {
        if (arrow_mode_orig == 0) {
          arrow_play_mode_count[5]--;
          if (arrow_play_mode_count[5] == 0) {
            arrow_play_mode_count[5] = arrow_play_mode_count_orig[5];
            arrow_mode = random8(1,5);
            arrow_change_mode = true;
          }
        }
        
        arrowSetupForMode(arrow_mode, arrow_change_mode);
      }
      break;
  }
}

void arrowSetupForMode(uint8_t mode, bool change) {
    switch (mode) {
      case 1:
        if (change) arrow_direction = 1;
        arrowSetup_mode1();    // От края матрицы к краю, по центру гориз и верт
        break;
      case 2:
        if (change) arrow_direction = 5;
        arrowSetup_mode2();    // По центру матрицы (гориз / верт) - ограничение - центр матрицы
        break;
      case 3:
        if (change) arrow_direction = 15;
        arrowSetup_mode2();    // как и в режиме 2 - по центру матрицы (гориз / верт) - ограничение - центр матрицы
        break;
      case 4:
        if (change) arrow_direction = 5;
        arrowSetup_mode4();    // От края матрицы к краю, верт / гориз
        break;
      case 5:
        if (change) arrow_direction = 15;
        arrowSetup_mode4();    // как и в режиме 4 от края матрицы к краю, на 1/3
        break;
    }
}

void arrowSetup_mode1() {
  // Слева направо
  if ((arrow_direction & 0x01) > 0) {
    arrow_hue[0] = random8();
    arrow_x[0] = 0;
    arrow_y[0] = pHEIGHT / 2;
    stop_x [0] = pWIDTH + 7;      // скрывается за экраном на 7 пикселей
    stop_y [0] = 0;              // неприменимо 
  }    
  // снизу вверх
  if ((arrow_direction & 0x02) > 0) {
    arrow_hue[1] = random8();
    arrow_y[1] = 0;
    arrow_x[1] = pWIDTH / 2;
    stop_y [1] = pHEIGHT + 7;     // скрывается за экраном на 7 пикселей
    stop_x [1] = 0;              // неприменимо 
  }    
  // справа налево
  if ((arrow_direction & 0x04) > 0) {
    arrow_hue[2] = random8();
    arrow_x[2] = pWIDTH - 1;
    arrow_y[2] = pHEIGHT / 2;
    stop_x [2] = -7;             // скрывается за экраном на 7 пикселей
    stop_y [2] = 0;              // неприменимо 
  }
  // сверху вниз
  if ((arrow_direction & 0x08) > 0) {
    arrow_hue[3] = random8();
    arrow_y[3] = pHEIGHT - 1;
    arrow_x[3] = pWIDTH / 2;
    stop_y [3] = -7;             // скрывается за экраном на 7 пикселей
    stop_x [3] = 0;              // неприменимо 
  }
}

void arrowSetup_mode2() {
  // Слева направо до половины экрана
  if ((arrow_direction & 0x01) > 0) {
    arrow_hue[0] = random8();
    arrow_x[0] = 0;
    arrow_y[0] = pHEIGHT / 2;
    stop_x [0] = pWIDTH / 2 - 1;  // до центра экрана
    stop_y [0] = 0;              // неприменимо 
  }    
  // снизу вверх до половины экрана
  if ((arrow_direction & 0x02) > 0) {
    arrow_hue[1] = random8();
    arrow_y[1] = 0;
    arrow_x[1] = pWIDTH / 2;
    stop_y [1] = pHEIGHT / 2 - 1; // до центра экрана
    stop_x [1] = 0;              // неприменимо 
  }    
  // справа налево до половины экрана
  if ((arrow_direction & 0x04) > 0) {
    arrow_hue[2] = random8();
    arrow_x[2] = pWIDTH - 1;
    arrow_y[2] = pHEIGHT / 2;
    stop_x [2] = pWIDTH / 2;      // до центра экрана
    stop_y [2] = 0;              // неприменимо 
  }
  // сверху вниз до половины экрана
  if ((arrow_direction & 0x08) > 0) {
    arrow_hue[3] = random8();
    arrow_y[3] = pHEIGHT - 1;
    arrow_x[3] = pWIDTH / 2;
    stop_y [3] = pHEIGHT / 2;     // до центра экрана
    stop_x [3] = 0;              // неприменимо 
  }
}

void arrowSetup_mode4() {
  // Слева направо
  if ((arrow_direction & 0x01) > 0) {
    arrow_hue[0] = random8();
    arrow_x[0] = 0;
    arrow_y[0] = (pHEIGHT / 3) * 2;
    stop_x [0] = pWIDTH + 7;      // скрывается за экраном на 7 пикселей
    stop_y [0] = 0;              // неприменимо 
  }    
  // снизу вверх
  if ((arrow_direction & 0x02) > 0) {
    arrow_hue[1] = random8();
    arrow_y[1] = 0;
    arrow_x[1] = (pWIDTH / 3) * 2;
    stop_y [1] = pHEIGHT + 7;     // скрывается за экраном на 7 пикселей
    stop_x [1] = 0;              // неприменимо 
  }    
  // справа налево
  if ((arrow_direction & 0x04) > 0) {
    arrow_hue[2] = random8();
    arrow_x[2] = pWIDTH - 1;
    arrow_y[2] = pHEIGHT / 3;
    stop_x [2] = -7;             // скрывается за экраном на 7 пикселей
    stop_y [2] = 0;              // неприменимо 
  }
  // сверху вниз
  if ((arrow_direction & 0x08) > 0) {
    arrow_hue[3] = random8();
    arrow_y[3] = pHEIGHT - 1;
    arrow_x[3] = pWIDTH / 3;
    stop_y [3] = -7;             // скрывается за экраном на 7 пикселей
    stop_x [3] = 0;              // неприменимо 
  }
}

// ***************************** КУБИК РУБИКА *****************************

/*
 *  Эти переменные объявлены для эффекта PALETTE
 *  
uint8_t num_x, num_y, off_x, off_y; 
*/

uint8_t* cube_h   = NULL;  // Цвет плашек поля эффекта
uint8_t* order_h  = NULL;  // Порядок вывода плашек на поле
int16_t* order_mt = NULL;  // Для варианта "Спираль" - массив задержек движения полос

int16_t  cube_idx;         // Индекс выводимой плашки в фазе начального вывода плашек на матрицу
int16_t  cube_black_idx;   // Индекс черной плашки в варианте "Пятнашки"
int16_t  cube_new_idx;     // Индекс плашки в варианте "Пятнашки", куда будет перемещаться черная
int8_t   cube_last_mv;     // Прошлое направление движение цветной плашки на место черной в "Пятнашках" ; 
uint8_t  cube_variant;     // Вариант анимации; 0 - случайный выбор; 1 - сдвиг по одной плашке; 2 - сдвиг всей полосы; 3 - вращение полос; 4 - пятнашки 
uint16_t cube_size;        // Количество плашек на поле 
uint8_t  cube_vh;          // 0 - вертикальное движение; 1 - горизонтальное
uint8_t  cube_rl;          // верт: 0 - вниз, 1 - вверх; гориз: 0 - влево; 1 - вправо
uint8_t  cube_move_cnt;    // На сколько линий в координатах матрицы (не плашек!) выполнять смещение
uint8_t  RUBIK_BLOCK_SIZE; // Размер квадратика палитры

uint8_t  py, px, ppx, ppy;
CHSV     cubeColorFrom;
CHSV     cubeColorTo;

void rubikRoutine() {

  if (loadingFlag) {
    // modeCode = MC_RUBIK;
    loadingFlag = false;

    uint8_t old_RBS = RUBIK_BLOCK_SIZE;
    RUBIK_BLOCK_SIZE = map8(getEffectScaleParamValue(MC_RUBIK),3,8);
    if (RUBIK_BLOCK_SIZE < 3) RUBIK_BLOCK_SIZE = 3;
    if (RUBIK_BLOCK_SIZE > min(pWIDTH, pHEIGHT) / 2) RUBIK_BLOCK_SIZE = min(pWIDTH, pHEIGHT) / 2;
    
    num_x = pWIDTH / RUBIK_BLOCK_SIZE;
    num_y = pHEIGHT / RUBIK_BLOCK_SIZE;
    off_x = (pWIDTH - RUBIK_BLOCK_SIZE * num_x) / 2;
    off_y = (pHEIGHT - RUBIK_BLOCK_SIZE * num_y) / 2;

    cube_last_mv = -1;
    hue = 0;
    cube_size = num_x * num_y;
    uint8_t step = 256 / (cube_size + 1);
    if (step < 10) step = 10;

    if (old_RBS != RUBIK_BLOCK_SIZE || cube_h == NULL) {
      if (cube_h != NULL) { 
        delete [] cube_h;
        delete [] order_h;  
        delete [] order_mt; 
      }  
      cube_h   = new uint8_t[cube_size]; for (uint8_t i = 0; i < cube_size; i++) { cube_h[i]  = hue; hue += step; }
      order_h  = new uint8_t[cube_size]; for (uint8_t i = 0; i < cube_size; i++) { order_h[i] = i; }
      order_mt = new int16_t[max(num_x, num_y)]; 
    }
    
    // Перемешать плашки и их порядок появления на матрице
    FOR_i (0, cube_size) {
      uint16_t idx1 = random16(0, cube_size - 1);
      uint16_t idx2 = random16(0, cube_size - 1);
      hue = cube_h[idx1];
      cube_h[idx1] = cube_h[idx2];
      cube_h[idx2] = hue;
      
      idx1 = random16(0, cube_size - 1);
      idx2 = random16(0, cube_size - 1);
      hue = order_h[idx1];
      order_h[idx1] = order_h[idx2];
      order_h[idx2] = hue;
    }

    // Если в настройках выбран вариант "Случайный" - выбрать любой другой из доступных
    cube_variant = getEffectScaleParamValue2(MC_RUBIK);
    if (cube_variant == 0 || cube_variant > 4) cube_variant = random8(1,4);
    
    cube_idx = 0;
    cube_black_idx = random16(0, cube_size);
    phase = 0;    // Фаза 0 - размещение плашек на матрице
    
    FastLED.clear();
  }
  
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  
  if (phase == 0) {
    // Взять из массива порядка размещения плашек очередную позицию, из массива цветов - цвет  и вывести на матрицу очередную плашку    
    uint16_t idx = order_h[cube_idx];
    CHSV color = cube_variant == 4 && idx == cube_black_idx
      ? CHSV(0, 0, 0)
      : CHSV(cube_h[idx], 255, effectBrightness);
    
    py = idx / num_x;
    px = idx % num_x;
    ppx = off_x + px * RUBIK_BLOCK_SIZE;
    ppy = off_y + py * RUBIK_BLOCK_SIZE;

    for (uint8_t y = 0; y < RUBIK_BLOCK_SIZE; y++) {
      for (uint8_t x = 0; x < RUBIK_BLOCK_SIZE; x++) {        
        drawPixelXY(ppx + x, ppy + y, color);        
      }  
    }
    
    cube_idx++;
    if (cube_idx >= cube_size) {
      // Все плашки выведены? - перейти к следующей фазе. 
      cube_idx = 0;
      phase++;       
    }
    return;
  }

  // Фаза "Настройка движения". Определяем что и в какую сторону нужно сдвигать
  if (phase == 1) {
      cube_idx = 0;
      
      if (cube_variant == 4) {
        // Определяем текущие координаты черной плашки - px,py 
        // Определяем какой из квадратиков вокруг черного будет перемещаться на его место:
        py = cube_black_idx / num_x;          // X,Y координаты черной плашки в сетке размера RUBIK_BLOCK_SIZE
        px = cube_black_idx % num_x;
        ppx = off_x + px * RUBIK_BLOCK_SIZE;  // левый верхний угол черной плашки в координатах матрицы
        ppy = off_y + py * RUBIK_BLOCK_SIZE;        
                
        bool ok = false;
        uint8_t mv = 0;
        // Движение "черной плашки": цветная плашка перемещается на место черной - определить с какой строны плашка будет еремещаться на место черной
        while (!ok) {          
          mv = random8(0, 4);  // 0 - сверху, 1 - справа, 2 - снизу, 3 - слева          
          // Проверить, что новое направление не противоположно предыдущему, чтобы черная плашка не ерзала туда-сюда
          ok = mv < 4 && ((mv == 0 && cube_last_mv != 2) || (mv == 2 && cube_last_mv != 0) || (mv == 1 && cube_last_mv != 3) || (mv == 3 && cube_last_mv != 1));
          // Проверить, что выбранная плашка не выходит за границы поля. Например - черная в нулевой строке, а выбор пал на плашку выше - там ее нет - поле кончилось
          ok &= (mv == 0 && py > 0) || (mv == 2 && py < num_y - 1) || (mv == 1 && px < num_x -1) || (mv  == 3 && px > 0);
        }
        
        switch (mv) {
          case 0: 
            // Плашка сверху от черной
            cube_new_idx = (py - 1) * num_x + px;
            break;
          case 1:
            // Плашка справа от черной
            cube_new_idx = py * num_x + px + 1;
            break;
          case 2:
            // Плашка снизу от черной
            cube_new_idx = (py + 1) * num_x + px;
            break;
          case 3:
            // плашка слева от черной
            cube_new_idx = py * num_x + px - 1;
            break;
        }          

        cube_last_mv = mv;
        cubeColorFrom = CHSV(cube_h[cube_new_idx], 255, effectBrightness);  // Цветная плашка
        cubeColorTo = CHSV(0, 0, 0);                                        // Черная плашка 
      } else 
      
      if (cube_variant == 3) {
        // Определяем какая полоса будет двигаться (индекс кубика), вертикально/горизонтально и в каком направдении вверх/вниз / вправо/влево
        cube_vh = random16(0, cube_size - 1) % 2;           // как - вертикально или горизонтально: 0 - вертикальное движение; 1 - горизонтальное - чередовать
        cube_rl = random16(0, cube_size - 1) % 2;           // куда - верт: 0 - вниз, 1 - вверх; гориз: 0 - влево; 1 - вправо
        uint8_t cube_mt = random16(0, cube_size - 1) % 2;   // начало: верт: 0 с левой до правой, 1 - с правой до левой; гориз: 0 - с верхней до нижней; 1 - с нижней до верхней
        
        // Сдвиг на целую плашку занимает RUBIK_BLOCK_SIZE шагов; 
        // Начало движения каждой следующей полосы задерживается на RUBIK_BLOCK_SIZE / 2 шагов 
        // Сдвиг полной полосы по горизонтали занимает num_x * RUBIK_BLOCK_SIZE шагов
        // Сдвиг полной полосы по вертикали занимает num_y * RUBIK_BLOCK_SIZE шагов
        // Массив с задержками для плашки размером 4x4 пикселя, в поле 4x4 плашки с учетом задержек определяется массивом [-6, -4, -2, 0]
        // При каждом проходе увеличиваем на 1 элемент массива. Пока элемент массива меньше нуля - сдвига нет; Если 0 или больше - выполняем сдвиг        
        // Когда значение элемента массива достигает num_x * RUBIK_BLOCK_SIZE для гориз или num_y * RUBIK_BLOCK_SIZE по верикали - перестаем прокручивать полосу (движение завершено)
        // Когда ВСЕ элементы массива достигают верхнего предела - весь цикл завершен, меняем phase на 1 - возврат к началу формирования цикла эффекта

        uint8_t stp = RUBIK_BLOCK_SIZE + RUBIK_BLOCK_SIZE / 2;
        uint8_t mcnt = cube_vh == 0 ? num_x : num_y;
        int8_t low = -1 * stp * (mcnt - 1);
        for (uint8_t i = 0; i < mcnt; i++) order_mt[i] = low + (cube_mt == 0 ? i : mcnt - i - 1) * stp;          
      } else
      
      {
        // cube_variant == 2, cube_variant == 1
        // Определяем какая полоса будет двигаться (индекс кубика), вертикально/горизонтально и в каком направлении вверх/вниз | вправо/влево
        cube_vh = cube_vh == 0 ? 1 : 0;  // 0 - вертикальное движение; 1 - горизонтальное - чередовать
        cube_rl = random16(0, cube_size - 1) % 2;  // верт: 0 - вниз, 1 - вверх; гориз: 0 - влево; 1 - вправо
        
        // Для вертикального смещения - номер колонки, которая будет двигаться, для горизонтального - номер строки
        if (cube_vh == 0) {
          px = random8(0, num_x);          // Случайная колонка, которая  будет смещаться вниз или вверх
        } else {
          py = random8(0, num_y);          // Случайная строка, которая будет смешаться влево или вправо
        }
        if (px >= num_x) px = num_x - 1;
        if (py >= num_y) py = num_y - 1;

        // смещение на один блок для режима 1 или на всю ширину / высоту для режима 2 и 3
        cube_move_cnt = cube_variant == 1 ? RUBIK_BLOCK_SIZE : (cube_vh == 0 ? num_y : num_x) * RUBIK_BLOCK_SIZE;
      }
      phase++;       
  }
  
  // Отображение следующей фазы зависит от выбранного варианта
  // Перемещение плашек (phase == 2)
  switch (cube_variant) {
    // 1 - Сдвиг на одину плашку всего ряда/колонки
    // 2 - Сдвиг всей полосы (ряд / колонка) на несколько плашек
    case 1:         
    case 2: { 
      cube_idx++;
      bool isEdge = cube_idx >= RUBIK_BLOCK_SIZE;
      if (isEdge) cube_idx = 0;
      rubikMoveLane(px, py, isEdge, effectBrightness);      
      // Если все шаги по перемещению полосы завершены - вернуться к файзе формирования направления движения следующей полосы
      cube_move_cnt--;
      if (cube_move_cnt == 0) {
        phase = 1;
      }        
    }
    break;

    case 3: {
      // Вращение полос
      uint8_t mcnt = cube_vh == 0 ? num_x : num_y; // Количество полос - в зависимости от верт/гориз - это либо ширина, либо высота матрицы
      uint8_t maxx = mcnt * RUBIK_BLOCK_SIZE;      // Максимальное уол-во шагов сдвига полосы - по количеству светодиодов
      bool processed = false;
      for (uint8_t i = 0; i < mcnt; i++) {
        int8_t cnt = order_mt[i] + 1;
        order_mt[i] = cnt;
        if (cnt > 0 && cnt <= maxx) {
          processed = true;
          // сдвиг ровно на одну плашку?
          bool isEdge = cnt > 0 && cnt % RUBIK_BLOCK_SIZE == 0;    
          rubikMoveLane(i, i, isEdge, effectBrightness);      
        }
      }
      if (!processed) {
        phase = 1;
      }              
    }
    break;

    case 4: {
      // Пятнашки
      // Имитируем движение цветной плашки (cube_new_idx) на место черной (cube_black_idx)
      
      if (cube_new_idx - cube_black_idx > 1) {
        // Вертикально. Цветная вверх на место черной; cube_new_idx - индекс цветной плашки, которая движется на место черной 
        for (uint8_t x = 0; x < RUBIK_BLOCK_SIZE; x++) {
          drawPixelXY(ppx + x, ppy + RUBIK_BLOCK_SIZE - cube_idx - 1, cubeColorFrom);
          drawPixelXY(ppx + x, ppy + 2 * RUBIK_BLOCK_SIZE - cube_idx - 1, cubeColorTo);
        }
      } else
      if (cube_new_idx - cube_black_idx < -1) {
        // Вертикально. Цветная вниз на место черной; newIdx - индекс цветной плашки, которая движется на место черной 
        for (uint8_t x = 0; x < RUBIK_BLOCK_SIZE; x++) {
          drawPixelXY(ppx + x, ppy + cube_idx, cubeColorFrom);
          drawPixelXY(ppx + x, ppy - RUBIK_BLOCK_SIZE + cube_idx, cubeColorTo);
        }
      } else
      if (cube_new_idx - cube_black_idx == -1) {
        // Горизонтально. Цветная вправо на место черной; newIdx - индекс цветной плашки, которая движется на место черной 
        for (uint8_t y = 0; y < RUBIK_BLOCK_SIZE; y++) {
          drawPixelXY(ppx + cube_idx, ppy + y, cubeColorFrom);
          drawPixelXY(ppx - RUBIK_BLOCK_SIZE + cube_idx, ppy + y, cubeColorTo);
        }
      } else
      if (cube_new_idx - cube_black_idx == 1) {
        // Горизонтально. Цветная влево на место черной; newIdx - индекс цветной плашки, которая движется на место черной 
        for (uint8_t y = 0; y < RUBIK_BLOCK_SIZE; y++) {
          drawPixelXY(ppx + RUBIK_BLOCK_SIZE - cube_idx - 1, ppy + y, cubeColorFrom);
          drawPixelXY(ppx + 2 * RUBIK_BLOCK_SIZE - cube_idx - 1, ppy + y, cubeColorTo);
        }
      }
      cube_idx++;
      if (cube_idx >= RUBIK_BLOCK_SIZE) {
        cube_h[cube_black_idx] = cube_h[cube_new_idx];
        cube_black_idx = cube_new_idx;
        // Если перемещение блока завершено - фаза движения закончена, перейти к фазе настройки следующего движения
        phase = 1;
      }
    }
    break;    
  }
}

void rubikMoveLane(uint8_t px, uint8_t py, bool isEdge, uint8_t effectBrightness) {
  CHSV cubeColorFrom;
  uint8_t  hue;
  uint16_t idx, idx1, idx2;
  uint32_t color;
  
  if (cube_vh == 0 && cube_rl == 0) {
    // Вертикально. Сдвиг колонки вниз - перерисовать строки с 1 до высоты матрицы сдвинуть вниз? нулевую заполнить цветом нижней в колонке плашки
    ppx = off_x + px * RUBIK_BLOCK_SIZE;
    for (uint8_t i = num_y  * RUBIK_BLOCK_SIZE + off_y - 1; i > off_y; i--) {
      color = getPixColorXY(ppx, i - 1);
      for (uint8_t x = 0; x < RUBIK_BLOCK_SIZE; x++) {
        drawPixelXY(ppx + x, i, color);
      }
    }
    // Поскольку сдвиг вниз -  заполнить освободившуюся верхнюю строку цветом нижней плашки
    idx = (num_y - 1) * num_x + px;
    cubeColorFrom = CHSV(cube_h[idx], 255, effectBrightness);
    for (uint8_t x = 0; x < RUBIK_BLOCK_SIZE; x++) {
      drawPixelXY(ppx + x, off_y, cubeColorFrom);
    }
  } else

  if (cube_vh == 0 && cube_rl == 1) {
    // Вертикально. Сдвиг колонки вверх
    ppx = off_x + px * RUBIK_BLOCK_SIZE;
    for (uint8_t i = off_y; i < num_y * RUBIK_BLOCK_SIZE + off_y - 1; i++) {
      color = getPixColorXY(ppx, i + 1);
      for (uint8_t x = 0; x < RUBIK_BLOCK_SIZE; x++) {
        drawPixelXY(ppx + x, i, color);
      }
    }
    // Поскольку сдвиг вверх - заполнить освободившуюся нижнюю строку цветом верхней плашки
    idx = px;
    cubeColorFrom = CHSV(cube_h[idx], 255, effectBrightness);
    for (uint8_t x = 0; x < RUBIK_BLOCK_SIZE; x++) {
      drawPixelXY(ppx + x, num_y * RUBIK_BLOCK_SIZE + off_y - 1, cubeColorFrom);
    }
  } else
  
  if (cube_vh == 1 && cube_rl == 0) {
    // Горизонтально. Сдвиг строки влево
    ppy = off_y + py * RUBIK_BLOCK_SIZE;
    for (uint8_t i = off_x; i < num_x * RUBIK_BLOCK_SIZE + off_x - 1; i++) {
      color = getPixColorXY(i + 1, ppy);
      for (uint8_t y = 0; y < RUBIK_BLOCK_SIZE; y++) {
        drawPixelXY(i, ppy + y, color);
      }
    }
    // Поскольку сдвиг влeво -  заполнить освободившуюся правую колонку цветом левой плашки
    idx = py *  num_x;
    cubeColorFrom = CHSV(cube_h[idx], 255, effectBrightness);
    for (uint8_t y = 0; y < RUBIK_BLOCK_SIZE; y++) {
      drawPixelXY(num_x * RUBIK_BLOCK_SIZE + off_x - 1 , ppy + y, cubeColorFrom);
    }
  } else
  
  if (cube_vh == 1 && cube_rl == 1) {
    // Горизонтально. Сдвиг строки вправо
    ppy = off_y + py * RUBIK_BLOCK_SIZE;
    for (uint8_t i = num_x  * RUBIK_BLOCK_SIZE + off_x - 1; i > off_x; i--) {
      uint32_t color = getPixColorXY(i - 1, ppy);
      for (uint8_t y = 0; y < RUBIK_BLOCK_SIZE; y++) {
        drawPixelXY(i, ppy + y, color);
      }
    }
    // Поскольку сдвиг вправо -  заполнить освободившуюся левую колонку цветом правой плашки
    idx = py * num_x + num_x - 1;
    cubeColorFrom = CHSV(cube_h[idx], 255, effectBrightness);
    for (uint8_t y = 0; y < RUBIK_BLOCK_SIZE; y++) {
      drawPixelXY(off_x, ppy + y, cubeColorFrom);
    }
  }
                                                             
  // Перемещение на одну плашку закончено?
  if (isEdge) {
    // Сместить индексы цветов в таблице текущих цветов плашек
    if (cube_vh == 0 && cube_rl == 0) {
      // Вертикально. Сдвиг колонки вниз
      idx = (num_y - 1) * num_x + px; // цвет последней строки
      hue = cube_h[idx];
      for (uint8_t i = num_y - 1; i > 0; i--){
        idx1 = (i - 1) * num_x + px;
        idx2 = i * num_x + px;
        cube_h[idx2] = cube_h[idx1];
      }
      idx = px; // Индекс первой строки - поместить туда цвет, который был вытеснен из последней строки при сдвиге плашек вниз
      cube_h[idx] = hue;
    } else
    
    if (cube_vh == 0 && cube_rl == 1) {
      // Вертикально. Сдвиг колонки вверх
      idx = px; // цвет первой строки
      hue = cube_h[idx];
      for (uint8_t i = 0; i < num_y; i++){
        idx1 = (i + 1) * num_x + px;
        idx2 = i * num_x + px;
        cube_h[idx2] = cube_h[idx1];
      }
      idx = (num_y - 1) * num_x + px; // Индекс последней строки - поместить туда цвет, который был вытеснен из первой строки при сдвиге плашек вверх
      cube_h[idx] = hue;
    } else
    
    if (cube_vh == 1 && cube_rl == 0) {
      // Горизонтально. Сдвиг строки влево
      idx = py * num_x; // цвет первой колонки в строке
      hue = cube_h[idx];
      for (uint8_t i = 0; i < num_x; i++){
        idx1 = py * num_x + i + 1;
        idx2 = py * num_x + i;
        cube_h[idx2] = cube_h[idx1];
      }
      idx = py * num_x + num_x - 1; // Индекс последней колонки в строке - поместить туда цвет, который был вытеснен из первой колонки при сдвиге плашек влево
      cube_h[idx] = hue;
    } else
    
    if (cube_vh == 1 && cube_rl == 1) {
      // Горизонтально. Сдвиг строки вправо
      idx = py * num_x + num_x - 1; // цвет последней колонки в строке
      hue = cube_h[idx];
      for (uint8_t i = num_x - 1; i > 0; i--){
        idx1 = py * num_x + i - 1 ;
        idx2 = py * num_x + i;
        cube_h[idx2] = cube_h[idx1];
      }
      idx = py * num_x; // Индекс первой колонки в строкн - поместить туда цвет, который был вытеснен из последней колонки при сдвиге плашек вправр
      cube_h[idx] = hue;
    }    
  } 
}

void rubikRoutineRelease() {
  if (cube_h   != NULL) { delete [] cube_h;   cube_h   = NULL; }
  if (order_h  != NULL) { delete [] order_h;  order_h  = NULL; }
  if (order_mt != NULL) { delete [] order_mt; order_mt = NULL; }
}
