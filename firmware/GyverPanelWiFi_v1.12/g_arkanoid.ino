
// **************** НАСТРОЙКИ ARKAN ****************
#define SHELF_LENGTH 5    // длина полки
#define VELOCITY 5        // скорость шара
#define BALL_SPEED 50     // период интегрирования

#define BLOCKS_H 4        // высота кучи блоков
#define ARK_LINE_NUM 8    // количество "линий" с блоками других уровней
#define ARK_LINE_MAX 4    // макс. длина линии

// цвета блоков по крутости
#define BLOCK_COLOR_1 CRGB::Aqua
#define BLOCK_COLOR_2 CRGB::Amethyst
#define BLOCK_COLOR_3 CRGB::Blue

uint8_t shelfMAX;
int16_t posX_ark, posY_ark, lastSpeed, arkScore;
int8_t  velX_ark, velY_ark, shelf_x;

timerMinim popTimeout(500);
timerMinim shelfTimer(150);

void newGameArkan() {
  arkScore = 0;
  generateBlocks();
  shelf_x = pWIDTH / 2 - SHELF_LENGTH / 2;
  posX_ark = pWIDTH / 2 * 10;
  posY_ark = 15;
  velX_ark = random(1, 4);
  velY_ark = (int32_t)sqrt(sq(VELOCITY) - sq(velX_ark));
  for (uint8_t i = shelf_x; i < shelf_x + SHELF_LENGTH; i++) {
      drawPixelXY(i, 0, GLOBAL_COLOR_2);
  }
}

void arkanoidRoutine() {

  if (loadingFlag) {
    FastLED.clear();
    loadingFlag = false;
    // modeCode = MC_ARKANOID;
    shelfMAX = pWIDTH - SHELF_LENGTH;
    buttons = 4;
    newGameArkan();    
    FastLEDshow();
    delay(500);
  }

  if (gameDemo) {
    if (shelfTimer.isReady()) {
      if (floor(posX_ark / 10) > shelf_x - SHELF_LENGTH / 2 - 1) buttons = 1;
      if (floor(posX_ark / 10) < shelf_x + SHELF_LENGTH / 2 + 1) buttons = 3;
    }
  }

  if (checkButtons()) {
    if (buttons == 3) {   // кнопка нажата
      shelfLeft();
    }
    if (buttons == 1) {   // кнопка нажата
      shelfRight();
    }
    buttons = 4;
  }

  if (!gamePaused && gameTimer.isReady()) {        // главный таймер игры
    drawPixelXY(posX_ark / 10, posY_ark / 10, CRGB::Black);
    posX_ark = posX_ark + velX_ark;
    posY_ark = posY_ark + velY_ark;
    int8_t posX_arkm = posX_ark / 10;
    int8_t posY_arkm = posY_ark / 10;
    if (abs(velY_ark) <= 2) {
      velX_ark = 3;
      velY_ark = (uint32_t)sqrt(sq(VELOCITY) - sq(velX_ark));
    }

    // отскок левый край
    if (posX_arkm < 0) {
      posX_ark = 0;
      velX_ark = -velX_ark;
    }

    // отскок правый край
    if (posX_arkm > pWIDTH - 1) {
      posX_ark = (pWIDTH - 1) * 10;
      velX_ark = -velX_ark;
    }

    // проверка на пробитие дна
    if (posY_ark < 9) {
      gameOverArkan();
      //posY_ark = 0;
      //velY_ark = -velY_ark;
    }

    // проверка на ударение с площадкой
    if (velY_ark < 0 && posY_ark > 10 && posY_ark <= 20 && posX_arkm >= shelf_x && posX_arkm < (shelf_x + SHELF_LENGTH)) {
      // таймаут, чтобы исключить дрочку у полки
      if (popTimeout.isReady()) {

        // тут, короче, если длина полки больше двух, то её края "подкручивают"
        // шарик, т.е. при отскоке меняют скорость по оси Х
        if (SHELF_LENGTH > 2) {
          if (posX_arkm == shelf_x) {
            velX_ark -= 2;  // уменьшаем скорость по Х
            // расчёт скорости по У с учётом общего заданного вектора скорости
            velY_ark = (uint32_t)sqrt(sq(VELOCITY) - sq(velX_ark));
          } else if (posX_arkm == (shelf_x + SHELF_LENGTH - 1)) {
            velX_ark += 2;  // увеличиваем скорость по Х
            velY_ark = (uint32_t)sqrt(sq(VELOCITY) - sq(velX_ark));
          } else {
            velY_ark = -velY_ark;
          }
        } else {
          velY_ark = -velY_ark;
        }
      }
    }

    // пробитие верха
    if (posY_arkm > pHEIGHT - 1) {
      posY_ark = (pHEIGHT - 1) * 10;
      velY_ark = -velY_ark;
    }
    uint8_t ballX = floor(posX_ark / 10);
    uint8_t ballY = floor(posY_ark / 10);

    if (ballY > 2) {
      if (ballX < pWIDTH - 1 && velX_ark > 0 && getPixColorXY(ballX + 1, ballY) != 0) {
        redrawBlock(ballX + 1, ballY);
        velX_ark = -velX_ark;
      }
      if (ballX > 1 && velX_ark < 0 && getPixColorXY(ballX - 1, ballY) != 0) {
        redrawBlock(ballX - 1, ballY);
        velX_ark = -velX_ark;
      }
      if (ballY < pHEIGHT - 1 && velY_ark > 0 && getPixColorXY(ballX, ballY + 1) != 0) {
        redrawBlock(ballX, ballY + 1);
        velY_ark = -velY_ark;
      }
      if (velY_ark < 0 && getPixColorXY(ballX, ballY - 1) != 0) {
        redrawBlock(ballX, ballY - 1);
        velY_ark = -velY_ark;
      }
    }

    if (checkBlocks()) gameOverArkan();    

    drawPixelXY(ballX, ballY, GLOBAL_COLOR_1);
    for (uint8_t i = shelf_x; i < shelf_x + SHELF_LENGTH; i++) {
      drawPixelXY(i, 0, GLOBAL_COLOR_2);
    }
  }
}

bool checkBlocks() {   
  // возвр ДА если блоков нет
  for (uint8_t j = pHEIGHT - 1; j > pHEIGHT - 1 - BLOCKS_H; j--) {
    for (uint8_t i = 0; i < pWIDTH; i++) {
      if (getPixColorXY(i, j) != 0) {
        return false;
      }
    }
  }
  return true;
}

void redrawBlock(uint8_t blockX, uint8_t blockY) {
  arkScore++;  
  if (getPixColorXY(blockX, blockY) == BLOCK_COLOR_2) 
    drawPixelXY(blockX, blockY, BLOCK_COLOR_1);
  else if (getPixColorXY(blockX, blockY) == BLOCK_COLOR_3) 
    drawPixelXY(blockX, blockY, BLOCK_COLOR_2);
  else 
    drawPixelXY(blockX, blockY, 0);
}

void generateBlocks() {
  for (uint8_t j = pHEIGHT - 1; j > pHEIGHT - 1 - BLOCKS_H; j--) {
    for (uint8_t i = 0; i < pWIDTH; i++) {
      drawPixelXY(i, j, BLOCK_COLOR_1);
    }
  }
  for (uint8_t k = 0; k < ARK_LINE_NUM; k++) {
    uint8_t newPosX = random(0, pWIDTH - 1 - ARK_LINE_MAX);
    uint8_t newPosY = random(pHEIGHT - BLOCKS_H, pHEIGHT);
    uint8_t newColor = random(0, 3);
    for (uint8_t i = newPosX; i < newPosX + ARK_LINE_MAX; i++) {
      switch (newColor) {
        case 0: drawPixelXY(i, newPosY, 0);
          break;
        case 1: drawPixelXY(i, newPosY, BLOCK_COLOR_2);
          break;
        case 2: drawPixelXY(i, newPosY, BLOCK_COLOR_3);
          break;
      }
    }
  }
}

void gameOverArkan() {
  displayScore(arkScore);
  FastLEDshow();
  delay(1500);
  FastLED.clear();
  newGameArkan();
}

void shelfRight() {
  shelf_x++;            // прибавить координату полки
  if (shelf_x > shelfMAX) { // если полка пробила правый край
    shelf_x = shelfMAX;
  } else {
    drawPixelXY(shelf_x - 1, 0, CRGB::Black);   // стереть последнюю точку
    drawPixelXY(shelf_x + SHELF_LENGTH - 1, 0, GLOBAL_COLOR_2);  // нарисовать первую
  }
}

void shelfLeft() {
  shelf_x--;
  if (shelf_x < 0) { // если полка пробила левый край
    shelf_x = 0;
  } else {
    drawPixelXY(shelf_x, 0, GLOBAL_COLOR_2);   // стереть последнюю точку
    drawPixelXY(shelf_x + SHELF_LENGTH, 0, CRGB::Black);  // нарисовать первую
  }
}
