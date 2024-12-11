// ********************* Фейерверк ******************
// Предложено пользователем Zordog

#include "palettes.h"

typedef struct  {
  bool     the_figState;   // true - фигура отображается; false - фигура не отображается
  uint8_t  the_figMode;    // тип фигуры 0 - круг; 1 - снежинка;
  uint8_t  the_figX;
  uint8_t  the_figY;
  uint8_t  the_figSize;    // текущий размер фигуры
  uint8_t  the_figSizeMax; // максимальный размер фигуры
  uint8_t  the_figHue;
  uint16_t the_figStart;
  uint16_t the_figPos;
} Firework;

CRGBPalette16 myRainbow_p = my_rainbow_with_white_gp;
uint8_t  the_figCount = 0; // количество фигур в вейерверке - от 4 до (pWIDTH / 4) * (pHEIGHT / 4)

Firework *firework = NULL;
  
void fireworksRoutine() {

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(MC_FIREWORKS));
  uint8_t the_sizeMax = 0;

  if      (pWIDTH  > pHEIGHT) the_sizeMax = pWIDTH / 2;
  else if (pHEIGHT > pWIDTH)  the_sizeMax = pHEIGHT / 2;
  else                        the_sizeMax = pWIDTH / 2;
  
  if (loadingFlag) {
    // modeCode = MC_FIREWORKS;

    loadingFlag = false;
    fireworksRoutineRelease();    
    
    the_figCount = map8(getEffectScaleParamValue2(MC_FIREWORKS),4,(pWIDTH / 4) * (pHEIGHT / 4));
    if (firework == NULL) { firework = new Firework[the_figCount]; }
    if (firework == NULL) { 
      // Если недостаточно памяти под эффект - перейти к другому эффекту;
      setRandomMode();
      return;      
    }
    
    for (uint8_t i = 0; i < the_figCount; i++ ) {
      firework[i].the_figState   = false;
      firework[i].the_figMode    = (random8(0, 99) % 4) == 0 ? 0 : 1;   // Соотношение круговых фейерверков к звездочным - 1:3
      firework[i].the_figX       = random8(1, pWIDTH - 1);
      firework[i].the_figY       = random8(1, pHEIGHT - 1);
      firework[i].the_figSize    = 0;
      firework[i].the_figSizeMax = random8(the_sizeMax / 2 ,the_sizeMax);
      firework[i].the_figHue     = random8(0, 224);
      firework[i].the_figStart   = random16(0, the_sizeMax * 2);
      firework[i].the_figPos     = 0;
    }

    FastLED.clear();  // очистить
  }

  fadeToBlackBy(leds, NUM_LEDS, map8(getEffectScaleParamValue(MC_FIREWORKS), 10, 192));

  for (uint8_t i = 0; i < the_figCount; i++ ) {

    firework[i].the_figPos++;
    if (!firework[i].the_figState && firework[i].the_figPos >= firework[i].the_figStart) {
      firework[i].the_figState = !firework[i].the_figState;
    }

    if (firework[i].the_figState) {

      uint16_t effectBrightnessFaded = (firework[i].the_figSize - firework[i].the_figSizeMax / 2) * (effectBrightness / firework[i].the_figSizeMax * 2);

      if (effectBrightnessFaded >= effectBrightness) effectBrightnessFaded = effectBrightness;

      #ifndef LINEARBLEND_NOWRAP
      #define LINEARBLEND_NOWRAP LINEARBLEND
      #endif

      if      (firework[i].the_figMode == 0) drawCircleBlend    (firework[i].the_figX, firework[i].the_figY, firework[i].the_figSize, ColorFromPalette( myRainbow_p, firework[i].the_figHue, (firework[i].the_figSize > firework[i].the_figSizeMax / 2) ? effectBrightness - effectBrightnessFaded : effectBrightness, LINEARBLEND_NOWRAP));
      else if (firework[i].the_figMode == 1) drawSnowflakesBlend(firework[i].the_figX, firework[i].the_figY, firework[i].the_figSize, ColorFromPalette( myRainbow_p, firework[i].the_figHue, (firework[i].the_figSize > firework[i].the_figSizeMax / 2) ? effectBrightness - effectBrightnessFaded : effectBrightness, LINEARBLEND_NOWRAP));

      firework[i].the_figSize++;
      if (firework[i].the_figSize > firework[i].the_figSizeMax) {
        firework[i].the_figStart   = random16(0, the_sizeMax * 2);
        firework[i].the_figMode    = (random8(0, 99) % 4) == 0 ? 0 : 1;   // Соотношение круговых фейерверков к звездочным - 1:3
        firework[i].the_figX       = random8(1, pWIDTH - 1);
        firework[i].the_figY       = random8(1, pHEIGHT - 1);
        firework[i].the_figSize    = 0;
        firework[i].the_figSizeMax = random8(the_sizeMax/2,the_sizeMax);
        firework[i].the_figHue     = random8(0, 224);
        firework[i].the_figPos     = 0;         
        firework[i].the_figState   = !firework[i].the_figState;
      }
    }
    
  }
  
}

void fireworksRoutineRelease() {

  if (firework != NULL) { free(firework); firework = NULL; }

}

void drawSnowflakesBlend(int x0, int y0, int figSize, CRGB color) {

  int16_t pn;
  
  if (figSize == 0) {
    pn = getPixelNumber(x0, y0); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    return;
  }

  pn = getPixelNumber(x0, y0 - figSize); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color,128); // луч вверх
  pn = getPixelNumber(x0 + figSize, y0); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color,128); // луч вправо
  pn = getPixelNumber(x0, y0 + figSize); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color,128); // луч вниз
  pn = getPixelNumber(x0 - figSize, y0); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color,128); // луч влево

  if (figSize > 1) {

    figSize--;
    pn = getPixelNumber(x0, y0 - figSize);           if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч вверх
    pn = getPixelNumber(x0 + figSize, y0);           if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч вправо
    pn = getPixelNumber(x0, y0 + figSize);           if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч вниз
    pn = getPixelNumber(x0 - figSize, y0);           if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч влево
    pn = getPixelNumber(x0 - figSize, y0 - figSize); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч вверх & влево
    pn = getPixelNumber(x0 + figSize, y0 - figSize); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч вверх & вправо
    pn = getPixelNumber(x0 + figSize, y0 + figSize); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч вниз & вправо
    pn = getPixelNumber(x0 - figSize, y0 + figSize); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128); // луч вниз & влево

  }
}

void drawCircleBlend(int x0, int y0, int figSize, CRGB color) {

  int16_t pn;
  int a = figSize, b = 0;
  int radiusError = 1 - a;

  if (figSize == 0) {
    pn = getPixelNumber(x0, y0); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    return;
  }

  while (a >= b) {

    pn = getPixelNumber(a + x0, b + y0);   if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    pn = getPixelNumber(b + x0, a + y0);   if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    pn = getPixelNumber(-a + x0, b + y0);  if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    pn = getPixelNumber(-b + x0, a + y0);  if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    pn = getPixelNumber(-a + x0, -b + y0); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    pn = getPixelNumber(-b + x0, -a + y0); if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    pn = getPixelNumber(a + x0, -b + y0);  if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);
    pn = getPixelNumber(b + x0, -a + y0);  if (pn >= 0 && pn < NUM_LEDS) nblend(leds[pn], color, 128);

    b++;
    if (radiusError < 0) {
      radiusError += 2 * b + 1;
    } else {
      a--;
      radiusError += 2 * (b - a + 1);
    }

  }
}

// *************************** Поток / Полосы **************************

// По мотивам предложенного пользователем Zordog  

uint8_t gCurrentGradientPaletteNumber = 0;

CRGBPalette16 gCurrentGradientPalette; 
CRGBPalette16 gTargetGradientPalette;

CRGB draw_color = CRGB::White;

void prizmata2Routine() {
  processStreamRoutine(1);
}

void shadows2Routine() {
  processStreamRoutine(2);
}

void processStreamRoutine(uint8_t aType) {
  
  if (loadingFlag) {
    // modeCode = MC_PRIZMATA2;
    // modeCode = MC_SHADOWS2;
    loadingFlag = false;
    switch (aType) {
      case 1:
        gCurrentGradientPaletteNumber = random8(gGradientPrizmataPaletteCount);
        gCurrentGradientPalette = gGradientPrizmataPalettes[ gCurrentGradientPaletteNumber ]; // CRGB::Black
        gCurrentGradientPaletteNumber = random8(gGradientPrizmataPaletteCount);
        gTargetGradientPalette = gGradientPrizmataPalettes[ gCurrentGradientPaletteNumber ];
        break;
      case 2:
        gCurrentGradientPaletteNumber = random8(gGradientShadowsPaletteCount);
        gCurrentGradientPalette = gGradientShadowsPalettes[ gCurrentGradientPaletteNumber ]; // CRGB::Black
        gCurrentGradientPaletteNumber = random8(gGradientShadowsPaletteCount);
        gTargetGradientPalette = gGradientShadowsPalettes[ gCurrentGradientPaletteNumber ];
        break;
    }        
    FastLED.clear();
  }

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  uint8_t sinbpm = thisMode == MC_PRIZMATA2 ? map8(getEffectScaleParamValue(thisMode),1,242) : map8(getEffectScaleParamValue(thisMode),1,212);
    
  EVERY_N_SECONDS(12) { chooseNextPalette(aType); }
  EVERY_N_MILLISECONDS(10) { nblendPaletteTowardPalette( gCurrentGradientPalette, gTargetGradientPalette, 12); }
  
  // Сдвигаем всю матрицу на одну строку вниз
  shiftDown();

  // Изменяем узор эффекта
  for (uint8_t x = 0; x < pWIDTH; x++) {
    switch (aType) {
      case 1: {
          // "Поток"
          uint8_t y = beatsin8(sinbpm + x, 0, pWIDTH - 1);
          drawPixelXY(y, pHEIGHT - 1, ColorFromPalette(gCurrentGradientPalette, x * pWIDTH / 2, effectBrightness));
        }
        break;
      case 2: {
          // Тени-2
          drawPixelXY(x, pHEIGHT - 1, ColorFromPalette(gCurrentGradientPalette, beatsin8(sinbpm + x, 0, 242), effectBrightness));
        }
        break;
    }
  }

}

void chooseNextPalette(uint8_t aType) {
    switch (aType) {
      case 1:
        gCurrentGradientPaletteNumber = random8(gGradientPrizmataPaletteCount);
        gTargetGradientPalette = gGradientPrizmataPalettes[ gCurrentGradientPaletteNumber ];
        break;
      case 2:
        gCurrentGradientPaletteNumber = random8(gGradientShadowsPaletteCount);
        gTargetGradientPalette = gGradientShadowsPalettes[ gCurrentGradientPaletteNumber ];
        break;
    }
}
