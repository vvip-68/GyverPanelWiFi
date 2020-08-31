#include "patterns.h"

int8_t patternIdx = -1;
int8_t lineIdx = 0;

// Заполнение матрицы указанным паттерном
// ptrn - индекс узора в массив узоров patterns[] в patterns.h
// X   - позиция X вывода узора
// Y   - позиция Y вывода узора
// W   - ширина паттерна
// H   - высота паттерна
// dir - рисовать 'u' снизу, сдвигая вверх; 'd' - сверху, сдвигая вниз
void drawPattern(uint8_t ptrn, uint8_t X, uint8_t Y, uint8_t W, uint8_t H, char dir) {
  
  if (dir == 'd') 
    shiftDown();
  else if (dir == 'u')
    shiftUp();

  uint8_t y = dir == 'd' ? (HEIGHT - 1) : 0;

  // Если ширина паттерна не кратна ширине матрицы - отрисовывать со сдвигом? чтобы рисунок был максимально по центру
  int8_t offset_x = -((WIDTH % W) / 2) + 1;
  
  byte effectBrightness = getBrightnessCalculated(globalBrightness, effectContrast[thisMode]);
  
  for (uint8_t x = 0; x < WIDTH + W; x++) {
    uint8_t in = (uint8_t)pgm_read_byte(&(patterns[ptrn][lineIdx][x % 10])); 
    CHSV color = colorMR[in];
    CHSV color2 = color.v != 0 ? CHSV(color.h, color.s, effectBrightness) : color;
    int8_t xx = offset_x + x;
    if (xx >= 0 && xx < WIDTH) {
      drawPixelXY(xx, y, color2); 
    }
  }

  if (dir == 'd') {
    lineIdx = (lineIdx > 0) ? (lineIdx - 1) : (H - 1);  
  } else {
    lineIdx = (lineIdx < H - 1) ? (lineIdx + 1) : 0;
  }
}

// Отрисовка указанной картинки с размерами WxH в позиции XY
void drawPicture_XY(uint8_t iconIdx, uint8_t X, uint8_t Y, uint8_t W, uint8_t H) {
  if (loadingFlag) {
    loadingFlag = false;
  }

  byte effectBrightness = getBrightnessCalculated(globalBrightness, effectContrast[thisMode]);

  for (byte x = 0; x < W; x++) {
    for (byte y = 0; y < H; y++) {
      uint8_t in = (uint8_t)pgm_read_byte(&(patterns[iconIdx][y][x])); 
      if (in != 0) {
        CHSV color = colorMR[in];        
        CHSV color2 = color.v != 0 ? CHSV(color.h, color.s, effectBrightness) : color;
        drawPixelXY(X+x,Y+H-y, color2); 
      }
    }
  }
}

void patternRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    patternIdx = effectScaleParam2[MC_PATTERNS] - 1;
    if (patternIdx < 0) {
      patternIdx = random8(0,MAX_PATTERN);
    }
    if (dir == 'd') 
      lineIdx = 9;         // Картинка спускается сверху вниз - отрисовка с нижней строки паттерна (паттерн 10x10)
    else 
      lineIdx = 0;         // Картинка поднимается сверху вниз - отрисовка с верхней строки паттерна
    // Цвета с индексом 6 и 7 - случайные, определяются в момент настройки эффекта
    colorMR[6] = CHSV(random8(), 255,255);
    if (random8() % 10 == 0) {
      colorMR[7] = CHSV(0,0,255);
    } else {
      colorMR[7] = CHSV(random8(), 255,255);
      while (abs(colorMR[7].h - colorMR[6].h) < 32) {
        colorMR[7] = CHSV(random8(), 255,255);
      }
    }
  }  

  drawPattern(patternIdx, 0, 0, 10, 10, 'd');  
}
