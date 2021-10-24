
// **************** НАСТРОЙКИ ЗМЕЙКИ ****************
#define START_LENGTH 4    // начальная длина змейки
#define MAX_LENGTH  80    // максимальная длина змейки

// **************** ДЛЯ РАЗРАБОТЧИКОВ ****************
int8_t  vectorX, vectorY;
int8_t  headX, headY, buttX, buttY;
int8_t  appleX, appleY;
int8_t  buttVector[MAX_LENGTH];
uint8_t snakeLength;
bool    butt_flag, pizdetc, missDelete = false;
volatile bool apple_flag;

void snakeRoutine() {
  if (loadingFlag || gameOverFlag) {
    FastLED.clear();
    loadingFlag = false;
    newGameSnake();
    // modeCode = MC_SNAKE;
    putApple();
  }  
  
  buttonsTickSnake();

  if (!apple_flag) putApple();  

  // запоминаем, куда повернули голову
  // 0 - право, 1 - лево, 2 - вверх, 3 - вниз
  if (vectorX > 0) buttVector[snakeLength] = 0;
  else if (vectorX < 0) buttVector[snakeLength] = 1;
  if (vectorY > 0) buttVector[snakeLength] = 2;
  else if (vectorY < 0) buttVector[snakeLength] = 3;

  // смещение головы змеи
  headX += vectorX;
  headY += vectorY;

  if (headX < 0 || headX > pWIDTH - 1 || headY < 0 || headY > pHEIGHT - 1) { // если вышла за границы поля
    pizdetc = true;
  }

  if (!pizdetc) {
    // проверка на pizdetc
    if ((uint32_t)(getPixColorXY(headX, headY) != 0 && (uint32_t)getPixColorXY(headX, headY) != GLOBAL_COLOR_2)) {   // если змея врезалась во что то, но не в яблоко
      pizdetc = true;                           // флаг на отработку
    }

    // БЛОК ОТРАБОТКИ ПОЕДАНИЯ ЯБЛОКА
    if (!pizdetc && (uint32_t)getPixColorXY(headX, headY) == (uint32_t)GLOBAL_COLOR_2) { // если попали головой в яблоко
      apple_flag = false;                       // флаг что яблока больше нет
      snakeLength++;                            // увеличить длину змеи
      buttVector[snakeLength] = 4;              // запоминаем, что надо будет не стирать хвост
    }

    // вычисляем координату хвоста (чтобы стереть) по массиву вектора
    switch (buttVector[0]) {
      case 0: buttX += 1;
        break;
      case 1: buttX -= 1;
        break;
      case 2: buttY += 1;
        break;
      case 3: buttY -= 1;
        break;
      case 4: missDelete = true;  // 4 значит не стирать!
        break;
    }

    // смещаем весь массив векторов хвоста ВЛЕВО
    for (uint8_t i = 0; i < snakeLength; i++) {
      buttVector[i] = buttVector[i + 1];
    }

    // если змея не в процессе роста, закрасить бывший хвост чёрным
    if (!missDelete) {
      drawPixelXY(buttX, buttY, 0x000000);
    }
    else missDelete = false;

    // рисуем голову змеи в новом положении
    drawPixelXY(headX, headY, GLOBAL_COLOR_1);
  }
  
  if (gameDemo) snakeDemo();

  // если он настал
  if (pizdetc) {
    gameOverFlag = true;
    pizdetc = false;

    // ну в общем плавно моргнуть, типо змейке "больно"
    for (uint8_t bright = 0; bright < 15; bright++) {
      FastLED.setBrightness(bright);
      for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
      }
      FastLEDshow();
      delay(10);
    }

    delay(100);
    FastLED.clear();
    FastLED.setBrightness(globalBrightness);

    //if (!gameDemo) {
      displayScore(snakeLength - START_LENGTH);
      FastLEDshow();
    //}
    delay(1500);    
  }
}

void putApple() {
  // БЛОК ГЕНЕРАЦИИ ЯБЛОКА
  while (!apple_flag) {                                   // пока яблоко не создано
    appleX = random(0, pWIDTH-1);                          // взять случайные координаты
    appleY = random(0, pHEIGHT-1);

    // проверить, не совпадает ли координата с телом змеи
    if ((uint32_t)getPixColorXY(appleX, appleY) == 0) {
      apple_flag = true;                                  // если не совпадает, считаем что яблоко создано
      drawPixelXY(appleX, appleY, GLOBAL_COLOR_2);        // и рисуем
    }
  }  
}

void snakeDemo() {
  // смещение головы змеи
  int8_t nextX = headX + vectorX;
  int8_t nextY = headY + vectorY;

  // ищем яблоко
  if (headX == appleX) {                // яблоко на одной линии по вертикали
    if (headY < appleY) buttons = 0;
    if (headY > appleY) buttons = 2;
  }
  if (headY == appleY) {                // яблоко на одной линии по горизонтали
    if (headX < appleX) buttons = 1;
    if (headX > appleX) buttons = 3;
  }

  if (getPixColorXY(nextX, nextY) == GLOBAL_COLOR_1) {   // проверка на столкновение с собой
    // поворачиваем налево
    if (vectorX > 0) buttons = 0;
    if (vectorX < 0) buttons = 2;
    if (vectorY > 0) buttons = 3;
    if (vectorY < 0) buttons = 1;
    return;
  }

  if (nextX < 0 || nextX > pWIDTH - 1 || nextY < 0 || nextY > pHEIGHT - 1) {       // проверка на столкновение со стеной
      
    // поворачиваем направо в обычном случае или налево в углу
    if (vectorX > 0) buttons = 2;
    if (vectorX > 0 && headY == 0) buttons = 0;

    if (vectorX < 0 && headY == pHEIGHT - 1) buttons = 2;
    if (vectorX < 0) buttons = 0;

    if (vectorY > 0) buttons = 1;
    if (vectorY > 0 && headX == pWIDTH - 1) buttons = 3;

    if (vectorY < 0 && headX == 0) buttons = 1;
    if (vectorY < 0) buttons = 3;
  }
}

void buttonsTickSnake() {
  if (checkButtons()) {
    if (buttons == 3 && vectorX != 1) {   // кнопка нажата
      vectorX = -1;
      vectorY = 0;
    }
    if (buttons == 1 && vectorX != -1) {   // кнопка нажата
      vectorX = 1;
      vectorY = 0;
    }
    if (buttons == 0 && vectorY != -1) {   // кнопка нажата
      vectorY = 1;
      vectorX = 0;
    }
    if (buttons == 2 && vectorY != 1) {   // кнопка нажата
      vectorY = -1;
      vectorX = 0;      
    }
    buttons = 4;
  }
}

void newGameSnake() {
  FastLED.clear();
  // свежее зерно для генератора случайных чисел
  randomSeed(millis());

  gameOverFlag = false;
  
  // длина из настроек, начинаем в середине экрана, бла-бла-бла
  snakeLength = START_LENGTH;
  headX = pWIDTH / 2;
  headY = pHEIGHT / 2;
  buttY = headY;

  vectorX = 1;  // начальный вектор движения задаётся вот здесь
  vectorY = 0;
  buttons = 4;

  // первоначальная отрисовка змейки и забивка массива векторов для хвоста
  for (uint8_t i = 0; i < snakeLength; i++) {
    drawPixelXY(headX - i, headY, GLOBAL_COLOR_1);
    buttVector[i] = 0;
  }

  buttX = headX - snakeLength;   // координата хвоста как голова - длина
  missDelete = false;
  apple_flag = false;
}
