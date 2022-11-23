
// ***************** НАСТРОЙКИ ГЕНЕРАЦИИ ЛАБИРИНТА *****************
#define GAMEMODE 0        // режим игры: 0 - видим весь лабиринт, 1 - видим вокруг себя часть
#define FOV 3             // область видимости в режиме игры 1

#define SHIFT 0                   // (1 да / 0 нет) смещение лабиринта (чтобы не видеть нижнюю и левую стену)
#define MAX_STRAIGHT 7            // максимальная длина прямых отрезков пути по краям
#define HOLES 2                   // количество дырок в стенах
#define MIN_PATH 20               // минимальная длина пути к выходу
// не рекомендуется ставить больше, чем треть количества светодиодов на матрице!

// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------
uint8_t  MAZE_WIDTH, MAZE_HEIGHT;
int8_t   playerPos[2];
uint32_t labTimer;
bool     mazeMode = false;
bool     mazeStarted = false;
uint16_t maxSolves;
char    *maze;

void newGameMaze() {
  playerPos[0] = !SHIFT;
  playerPos[1] = !SHIFT;

  gameOverFlag = false;
  buttons = 4;
  FastLED.clear();

  GenerateMaze(maze, MAZE_WIDTH, MAZE_HEIGHT);    // генерировать лабиринт обычным способом
  SolveMaze(maze, MAZE_WIDTH, MAZE_HEIGHT);       // найти путь

  if (!(GAMEMODE || mazeMode)) {
    for (uint8_t y = 0; y < MAZE_HEIGHT; y++) {
      for (uint8_t x = 0; x < MAZE_WIDTH; x++) {
        switch (maze[(y + SHIFT) * MAZE_WIDTH + (x + SHIFT)]) {
          case 1:  drawPixelXY(x, y, GLOBAL_COLOR_1); break;
          case 2:
            // drawPixelXY(x, y, GLOBAL_COLOR_1);    // рисовать путь к выходу
            break;
          default: drawPixelXY(x, y, 0x000000); break;
        }
      }
      // Отображаем сгенерированный лабиринт строка за строкой
      FastLEDshow();
      delay(25);
    }
  } else {
    for (uint8_t y = 0; y < FOV; y++) {
      for (uint8_t x = 0; x < FOV; x++) {
        switch (maze[(y + SHIFT) * MAZE_WIDTH + (x + SHIFT)]) {
          case 1:  drawPixelXY(x, y, GLOBAL_COLOR_1);  break;
          case 2:
            // drawPixelXY(x, y, GLOBAL_COLOR_1);    // рисовать путь к выходу
            break;
          default: drawPixelXY(x, y, 0x000000);  break;
        }
      }
    }
  }

  // Отрисовка - с видимыми границами по периметру (настройки SHIFT выше)
  // Слева от начальной позиции делаем дыру - это вход
  if (playerPos[0]>0) {
    playerPos[0] = playerPos[0] - 1;
    drawPixelXY(playerPos[0], playerPos[1], 0x000000);
  }
  
  drawPixelXY(playerPos[0], playerPos[1], GLOBAL_COLOR_2);

  FastLEDshow();
  delay(25);

  mazeStarted = false;  
}

void mazeRoutine() {
  if (loadingFlag || gameOverFlag) {  
    FastLED.clear();
    loadingFlag = false;
    // размеры лабиринта ДОЛЖНЫ БЫТЬ НЕЧЁТНЫЕ независимо от размеров матрицы!
    // при SHIFT 1 размер лабиринта можно ставить на 1 длиннее матрицы (матрица 16х16 лабиринт 17х17)
    MAZE_WIDTH = pWIDTH;
    MAZE_HEIGHT = pHEIGHT;
    if (MAZE_WIDTH % 2 == 0) MAZE_WIDTH--;
    if (MAZE_HEIGHT % 2 == 0) MAZE_HEIGHT--;
    maxSolves = MAZE_WIDTH * MAZE_WIDTH * 5;
    if (!maze) maze = (char*)malloc(MAZE_WIDTH * MAZE_HEIGHT * sizeof(char));
    newGameMaze();
    // modeCode = MC_MAZE;
  }

  if (gameDemo && !gamePaused) demoMaze();
  buttonsTickMaze();
}

void mazeRoutineRelease() {
  if (maze) { free(maze); maze = NULL; }
}

void buttonsTickMaze() {
  if (checkButtons()) {
    bool btnPressed = false;
    if (buttons == 3) {   // кнопка нажата
      btnPressed = true;
      int8_t newPos = playerPos[0] - 1;
      if (newPos >= 0 && newPos <= MAZE_WIDTH - 1)
        if (getPixColorXY(newPos, playerPos[1]) == 0) {
          movePlayer(newPos, playerPos[1], playerPos[0], playerPos[1]);
          playerPos[0] = newPos;
        }
    }
    if (buttons == 1) {   // кнопка нажата
      btnPressed = true;
      int8_t newPos = playerPos[0] + 1;
      if (newPos >= 0 && newPos <= MAZE_WIDTH - 1)
        if (getPixColorXY(newPos, playerPos[1]) == 0) {
          movePlayer(newPos, playerPos[1], playerPos[0], playerPos[1]);
          playerPos[0] = newPos;
        }
    }
    if (buttons == 0) {   // кнопка нажата
      btnPressed = true;
      int8_t newPos = playerPos[1] + 1;
      if (newPos >= 0 && newPos <= MAZE_HEIGHT - 1)
        if (getPixColorXY(playerPos[0], newPos) == 0) {
          movePlayer(playerPos[0], newPos, playerPos[0], playerPos[1]);
          playerPos[1] = newPos;
        }
    }
    if (buttons == 2) {   // кнопка нажата
      btnPressed = true;
      int8_t newPos = playerPos[1] - 1;
      if (newPos >= 0 && newPos <= MAZE_HEIGHT - 1)
        if (getPixColorXY(playerPos[0], newPos) == 0) {
          movePlayer(playerPos[0], newPos, playerPos[0], playerPos[1]);
          playerPos[1] = newPos;
        }
    }
    if (btnPressed && !mazeStarted) {
      mazeStarted = true;
      labTimer = millis();
    }
    buttons = 4;
  }
}

void movePlayer(int8_t nowX, int8_t nowY, int8_t prevX, int8_t prevY) {
  drawPixelXY(prevX, prevY, 0x000000);
  drawPixelXY(nowX, nowY, GLOBAL_COLOR_2);

  if ((nowX == (MAZE_WIDTH - 2) - SHIFT) && (nowY == (MAZE_HEIGHT - 1) - SHIFT)) {
    gameOverFlag = true;
    FastLEDshow();
    delay(250);
    FastLED.clear();
    //if (!gameDemo) {
      displayScore((millis() - labTimer) / 1000);
      FastLEDshow();
    //}
    delay(1500);
    return;
  }

  if (GAMEMODE || mazeMode) {
    for (int8_t y = nowY - FOV; y < nowY + FOV; y++) {
      for (int8_t x = nowX - FOV; x < nowX + FOV; x++) {
        if (x < 0 || x > pWIDTH - 1 || y < 0 || y > pHEIGHT - 1) continue;
        if (maze[(y + SHIFT) * MAZE_WIDTH + (x + SHIFT)] == 1) {
          drawPixelXY(x, y, GLOBAL_COLOR_1);
        }
      }
      FastLEDshow();
    }
  }
}

void demoMaze() {
  if (checkPath(0, 1)) buttons = 0;
  if (checkPath(1, 0)) buttons = 1;
  if (checkPath(0, -1)) buttons = 2;
  if (checkPath(-1, 0)) buttons = 3;
}

bool checkPath(int8_t x, int8_t y) {
  // если проверяемая клетка является путём к выходу
  if ( (maze[(playerPos[1] + y + SHIFT) * MAZE_WIDTH + (playerPos[0] + x + SHIFT)]) == 2) {
    maze[(playerPos[1] + SHIFT) * MAZE_WIDTH + (playerPos[0] + SHIFT)] = 4;   // убираем текущую клетку из пути (2 - метка пути, ставим любое число, например 4)
    return true;
  }
  else return false;
}

// функция, перегенерирующая лабиринт до тех пор,
// пока он не будет соответствовать требованиям "интересности"
void smartMaze() {
  uint8_t sum = 0, line;
  uint8_t attempt = 0;
  while (sum < MIN_PATH) {                          // пока длина пути меньше заданной
    attempt++;
    if (attempt > 200) break;                       // если совершено более 200 попыток, завершить алгоритм (если зависнет)
    
    //randomSeed(millis());                         // зерно для генератора псевдослучайных чисел
    GenerateMaze(maze, MAZE_WIDTH, MAZE_HEIGHT);    // генерировать лабиринт
    SolveMaze(maze, MAZE_WIDTH, MAZE_HEIGHT);       // найти путь

    // подсчёт длины пути
    sum = 0;
    for (uint8_t y = 0; y < MAZE_HEIGHT; y++) {        // перебор всех пикселей (элементов массива maze)
      for (uint8_t x = 0; x < MAZE_WIDTH; x++) {
        if (maze[y * MAZE_WIDTH + x] == 2) sum++;   // в массиве maze пиксель "правильного пути" имеет значение 2
      }
    }
    if (MAX_STRAIGHT > 0) {
      // подсчёт длины путей у краёв
      // если сумма превышает настройку, обнулить sum, таким образом цикл начнётся по новой
      line = 0;
      for (uint8_t y = 0; y < MAZE_HEIGHT; y++)
        if (maze[y * MAZE_WIDTH + MAZE_WIDTH - 2] == 2) line++;
      if (line > MAX_STRAIGHT) sum = 0;

      line = 0;
      for (uint8_t x = 0; x < MAZE_WIDTH; x++)
        if (maze[MAZE_WIDTH + x] == 2) line++;
      if (line > MAX_STRAIGHT) sum = 0;
    }
    delay(0);
  }
}

// функция, делающая отверстия в стенках для "неидеальности" лабиринта
void makeHoles() {
  uint8_t holes = 0;
  uint8_t attempt = 0;
  while (holes < HOLES) {                           // пока текущее число дыр меньше заданного
    attempt++;                                      // прибавляем число попыток
    if (attempt > 200) break;                       // если совершено более 200 попыток, завершить алгоритм (если зависнет)

    // берём случайные координаты внутри лабиринта
    uint8_t x = random(1, MAZE_WIDTH - 1);
    uint8_t y = random(1, MAZE_HEIGHT - 1);

    // итак, элемент массива maze, имеющий значение 1, является "стенкой" лабиринта. Погнали:
    if (maze[y * MAZE_WIDTH + x] == 1                   // если текущий пиксель является стенкой
        && maze[y * MAZE_WIDTH + (x - 1)] == 1          // и если рядом с ним по оси Х есть сосед
        && maze[y * MAZE_WIDTH + (x + 1)] == 1          // и с другой стороны тоже есть
        && maze[(y - 1) * MAZE_WIDTH + x] != 1          // а по оси У нет ни одного соседа
        && maze[(y + 1) * MAZE_WIDTH + x] != 1) {       // (это чтобы не дырявить угловые стенки)
      maze[y * MAZE_WIDTH + x] = 0;                     // сделать стенку проходом (пробить дыру)
      holes++;                                          // увеличить счётчик число дыр в стенах
    } else if (maze[y * MAZE_WIDTH + x] == 1            // если нет, проверим ещё раз
               && maze[(y - 1) * MAZE_WIDTH + x] == 1   // и если рядом с ним по оси У есть сосед
               && maze[(y + 1) * MAZE_WIDTH + x] == 1   // и с другой стороны тоже есть
               && maze[y * MAZE_WIDTH + (x - 1)] != 1   // а по оси X нет ни одного соседа
               && maze[y * MAZE_WIDTH + (x + 1)] != 1) {
      maze[y * MAZE_WIDTH + x] = 0;                     // сделать стенку проходом (пробить дыру)
      holes++;                                          // увеличить счётчик число дыр в стенах
    }
    delay(0);
  }
}

// копаем лабиринт
void CarveMaze(char *maze, int8_t width, int8_t height, int8_t x, int8_t y) {
  int8_t x1, y1;
  int8_t x2, y2;
  int8_t dx, dy;
  int8_t dir, count;

  dir = random(10) % 4;
  count = 0;
  while (count < 4) {
    dx = 0; dy = 0;
    switch (dir) {
      case 0:  dx = 1;  break;
      case 1:  dy = 1;  break;
      case 2:  dx = -1; break;
      default: dy = -1; break;
    }
    x1 = x + dx;
    y1 = y + dy;
    x2 = x1 + dx;
    y2 = y1 + dy;
    if (   x2 > 0 && x2 < width && y2 > 0 && y2 < height
           && maze[y1 * width + x1] == 1 && maze[y2 * width + x2] == 1) {
      maze[y1 * width + x1] = 0;
      maze[y2 * width + x2] = 0;
      x = x2; y = y2;
      dir = random(10) % 4;
      count = 0;
    } else {
      dir = (dir + 1) % 4;
      count += 1;
    }
  }
}

// генератор лабиринта
void GenerateMaze(char *maze, int8_t width, int8_t height) {
  uint16_t x, y;
  for (x = 0; x < width * height; x++) {
    maze[x] = 1;
  }
  maze[1 * width + 1] = 0;
  for (y = 1; y < height; y += 2) {
    for (x = 1; x < width; x += 2) {
      CarveMaze(maze, width, height, x, y);
    }
  }
  // вход и выход
  maze[0 * width + 1] = 0;
  maze[(height - 1) * width + (width - 2)] = 0;
}

// решатель (ищет путь)
void SolveMaze(char *maze, int8_t width, int8_t height) {
  int8_t dir, count;
  int8_t x, y;
  int8_t dx, dy;
  int8_t forward;
  // Remove the entry and exit. 
  maze[0 * width + 1] = 1;
  maze[(height - 1) * width + (width - 2)] = 1;

  forward = 1;
  dir = 0;
  count = 0;
  x = 1;
  y = 1;
  uint32_t attempts = 0;
  while (x != width - 2 || y != height - 2) {
    if (attempts++ > maxSolves) {   // если решатель не может найти решение (maxSolves в 5 раз больше числа клеток лабиринта)
      gameOverFlag = true;          // перегенерировать лабиринт
      break;                        // прервать решение
    }
    dx = 0; dy = 0;
    switch (dir) {
      case 0:  dx = 1;  break;
      case 1:  dy = 1;  break;
      case 2:  dx = -1; break;
      default: dy = -1; break;
    }
    if (   (forward  && maze[(y + dy) * width + (x + dx)] == 0)
           || (!forward && maze[(y + dy) * width + (x + dx)] == 2)) {
      maze[y * width + x] = forward ? 2 : 3;
      x += dx;
      y += dy;
      forward = 1;
      count = 0;
      dir = 0;
    } else {
      dir = (dir + 1) % 4;
      count += 1;
      if (count > 3) {
        forward = 0;
        count = 0;
      }
    }
  }

  // Replace the entry and exit.
  maze[(height - 2) * width + (width - 2)] = 2;
  maze[(height - 1) * width + (width - 2)] = 2;
}
