
// **************** НАСТРОЙКИ ТЕТРИС ****************
#define FAST_SPEED 20     // скорость падения при удержании "вниз" (меньше - быстрее)
#define STEER_SPEED 40    // скорость перемещения в бок при удержании кнопки (меньше - быстрее)
#define FULL_SCREEN 0     // 0 - ширина стакана равна его высоте; 1 - тетрис на всю ширину экрана

// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------
#define ADD_COLOR 0x010101

int8_t fig = 0, ang = 0, pos = pWIDTH / 2, height = pHEIGHT - 1;
int8_t prev_ang, prev_pos, prev_height;

uint32_t colors[6] {0x0000EE, 0xEE0000, 0x00EE00, 0x00EEEE, 0xEE00EE, 0xEEEE00};
uint32_t color = 0x000088;
byte color_index;
byte linesToClear;
boolean down_flag = true;
byte lineCleanCounter;
byte left_offset, right_offset;

// самая важная часть программы! Координаты пикселей фигур
//  0 - палка
//  1 - кубик
//  2 - Г
//  3 - Г обратная
//  4 - Z
//  5 - Z обратная
//  6 - Т

const int8_t figures[7][12][2] PROGMEM = {
  {
    { -1, 0}, {1, 0}, {2, 0},
    {0, 1}, {0, 2}, {0, 3},
    { -1, 0}, {1, 0}, {2, 0},
    {0, 1}, {0, 2}, {0, 3},
  },
  {
    {0, 1}, {1, 0}, {1, 1},
    {0, 1}, {1, 0}, {1, 1},
    {0, 1}, {1, 0}, {1, 1},
    {0, 1}, {1, 0}, {1, 1},
  },
  {
    { -1, 0}, { -1, 1}, {1, 0},
    {0, 1}, {0, 2}, {1, 2},
    { -2, 1}, { -1, 1}, {0, 1},
    { -1, 0}, {0, 1}, {0, 2},
  },
  {
    { -1, 0}, {1, 0}, {1, 1},
    {0, 1}, {0, 2}, {1, 0},
    {0, 1}, {1, 1}, {2, 1},
    {0, 1}, {0, 2}, { -1, 2},
  },
  {
    { -1, 0}, {0, 1}, {1, 1},
    {0, 1}, { -1, 1}, { -1, 2},
    { -1, 0}, {0, 1}, {1, 1},
    {0, 1}, { -1, 1}, { -1, 2},
  },
  {
    { -1, 1}, {0, 1}, {1, 0},
    {0, 1}, {1, 1}, {1, 2},
    { -1, 1}, {0, 1}, {1, 0},
    {0, 1}, {1, 1}, {1, 2},
  },
  {
    { -1, 0}, {0, 1}, {1, 0},
    {0, 1}, {0, 2}, {1, 1},
    { -1, 1}, {0, 1}, {1, 1},
    { -1, 1}, {0, 1}, {0, 2},
  }
};

void tetrisRoutine() {
  if (loadingFlag || gameOverFlag) {
    // modeCode = MC_TETRIS;
    FastLED.clear();
    loadingFlag = false;

    if (FULL_SCREEN == 1) {
      left_offset = 0;
      right_offset = pWIDTH - 1;
    } else {
      left_offset = pWIDTH > pHEIGHT ? ((pWIDTH - pHEIGHT) / 2) : 0;
      right_offset = pWIDTH > pHEIGHT ? pWIDTH - ((pWIDTH - pHEIGHT) / 2) - (pWIDTH % 2 == 0 ? 1 : 0) : pWIDTH - 1;
    }

    // -----------------------------------
    // Рисуем стакан
    byte center = pWIDTH / 2;
    int8_t xl = center;
    int8_t xr = center;
  
    CRGB color = 0x1E1E1E;
    while (xl >= left_offset || xr <= right_offset) {
      if (xl >= left_offset) drawPixelXY(xl, 0, color);
      if (xr <= right_offset) drawPixelXY(xr, 0, color);
      delay(25);
      FastLEDshow();
      xl--; xr++;
    }
    for (byte i = 0; i < pHEIGHT; i++) {
      drawPixelXY(left_offset, i, color);
      drawPixelXY(right_offset, i, color);
      delay(25);
      FastLEDshow();
    }
    // -----------------------------------

    newGameTetris();
  }

  bool timerReady = gameTimer.isReady();

  if (gameDemo && timerReady) checkTetrisButtons();

  if (buttons == 3) {        // кнопка нажата
    buttons = 4;
    stepLeft();
  }

  if (buttons == 1) {
    buttons = 4;
    stepRight();
  }

  if (buttons == 0) {
    buttons = 4;
    if (checkArea(3)) {       // проверка возможности поворота
      prev_ang = ang;         // запоминаем старый угол
      ang++;
      ang = ang % 4;          // изменяем ang от 0 до 3
      redrawFigure(prev_ang, pos, height);    // перерисовать фигуру
    }
  }

  if (buttons == 2) {         // кнопка вниз - "сбросить" фигуру
    buttons = 4;    
    gameTimer.setInterval(FAST_SPEED);  // увеличить скорость
  }

  if (!gamePaused && timerReady) {      // главный таймер игры
    prev_height = height;
  
    if (!checkArea(0)) {            // проверяем столкновение с другими фигурами
      if (height >= pHEIGHT - 2) {   // проиграл по высоте
        gameOverTetris();           // игра окончена, очистить всё
        gameOverFlag = true;
      } else {                      // если не достигли верха
        fixFigure();                // зафиксировать
        checkAndClear();            // проверить ряды и очистить если надо
        newGameTetris();            // Запустить новую фигуру
      }
    } else if (height == 0) {       // если достигли дна
      fixFigure();                  // зафиксировать
      checkAndClear();              // проверить ряды и очистить если надо
      newGameTetris();              // Запустить новую фигуру
    } else {                                // если путь свободен
      height--;                             // идём вниз
      redrawFigure(ang, pos, prev_height);  // перерисовка
    }
  }
}

// Заглушка чтения кнопок управления игрой
void checkTetrisButtons() {
  byte value = random(7);
  switch (value) {
    case 1: 
      buttons = 1; 
      break;
    case 5: 
      buttons = 3; 
      break;
    case 0: 
    case 7: 
      buttons = 0; 
      break;
  }
}

// поиск и очистка заполненных уровней
void checkAndClear() {
  linesToClear = 1;                 // счётчик заполненных строк по вертикали. Искусственно принимаем 1 для работы цикла
  boolean full_flag = true;         // флаг заполненности
  byte y_st = 1;
  byte x_st = left_offset + 1;
  byte x_en = right_offset - 1;
  
  while (linesToClear != 0) {       // чисти чисти пока не будет чисто!
    linesToClear = 0;
    byte lineNum = 255;             // высота, с которой начинаются заполненные строки (искусственно увеличена)
    for (byte Y = y_st; Y < pHEIGHT; Y++) {   // сканируем по высоте
      full_flag = true;                   // поднимаем флаг. Будет сброшен, если найдём чёрный пиксель
      for (byte X = x_st; X <= x_en; X++) {  // проходимся по строкам
        if ((long)getPixColorXY(X, Y) == (long)0x000000) {  // если хоть один пиксель чёрный
          full_flag = false;                                 // считаем строку неполной
        }
      }
      if (full_flag) {        // если нашлась заполненная строка
        linesToClear++;       // увеличиваем счётчик заполненных строк
        if (lineNum == 255)   // если это первая найденная строка
          lineNum = Y;        // запоминаем высоту. Значение 255 было просто "заглушкой"
      } else {                // если строка не полная
        if (lineNum != 255)   // если lineNum уже не 255 (значит строки были найдены!!)
          break;              // покинуть цикл
      }
    }
    if (linesToClear > 0) {             // если найденных полных строк больше 1
      lineCleanCounter += linesToClear;   // суммируем количество очищенных линий (игровой "счёт")

      // заполняем весь блок найденных строк белым цветом слева направо
      for (byte X = x_st; X <= x_en; X++) {
        for (byte i = 0; i < linesToClear; i++) {
          leds[getPixelNumber(X, lineNum + i)] = CHSV(0, 0, 255);         // закрашиваем его белым
        }
        FastLEDshow();
        delay(5);     // задержка между пикселями слева направо
      }
      delay(10);

      // теперь плавно уменьшаем яркость всего белого блока до нуля
      for (byte val = 0; val <= 30; val++) {
        for (byte X = x_st; X <= x_en; X++) {
          for (byte i = 0; i < linesToClear; i++) {
            leds[getPixelNumber(X, lineNum + i)] = CHSV(0, 0, 240 - 8 * val);  // гасим белый цвет
          }
        }
        FastLEDshow();
        delay(5);       // задержка между сменой цвета
      }
      delay(10);

      // и теперь смещаем вниз все пиксели выше уровня с первой найденной строкой
      for (byte i = 0; i < linesToClear; i++) {
        for (byte Y = lineNum; Y < pHEIGHT - 1; Y++) {
          for (byte X = x_st; X <= x_en; X++) {
            drawPixelXY(X, Y, getPixColorXY(X, Y + 1));      // сдвигаем вниз
          }
          //FastLEDshow();   // Почему-то этот show() рисует весь "стакан", прижатым к правому краю матрицы, хотя на широких матрицах всё действо происходит по центру
                             // и предыдущие show() в циклах выше рисуют все точки в правильном месте
        }
        // delay(200);       // задержка между "сдвигами" всех пикселей на один уровень - убрал, так как отрисовка выше убрана
      }
    }
  }
}


// функция фиксации фигуры
void fixFigure() {
  color += ADD_COLOR;                   // чутка перекрасить
  redrawFigure(ang, pos, prev_height);  // перерисовать
}

// проигрыш
void gameOverTetris() {
  FastLED.clear();

  // тут можно вывести счёт lineCleanCounter
  if (!gameDemo) {
    displayScore(lineCleanCounter);
    FastLEDshow();
  }
  delay(1500);

  lineCleanCounter = 0;   // сброс счёта
  FastLED.clear();

  gameOverFlag = true;
  gamePaused = !gameDemo;

  delay(20);
}

// новый раунд
void newGameTetris() {
  delay(10);
  
  gameOverFlag = false;

  setTimersForMode(MC_TETRIS);   // восстановить скорость

  buttons = 4;
  height = pHEIGHT;    // высота = высоте матрицы
  pos = pWIDTH / 2;    // фигура появляется в середине
  fig = random(7);     // выбираем случайно фигуру
  ang = random(4);     // и угол поворота
  //color = colors[random(6)];      // случайный цвет

  color_index++;
  color_index = color_index % 6;    // все цвета по очереди
  color = colors[color_index];

  // если включен демо-режим, позицию по горизонтали выбираем случайно
  if (gameDemo) {
    pos = random(left_offset+2, right_offset - 1);
  }
      
  down_flag = false;  // разрешаем ускорять кнопкой "вниз"
  delay(10);
}

// управление фигурами вправо и влево
void stepRight() {
  byte x_en = right_offset - 1;
  if (checkArea(1)) {
    prev_pos = pos;
    if (++pos >= x_en) pos = x_en;
    redrawFigure(ang, prev_pos, height);
  }
}

void stepLeft() {
  if (checkArea(2)) {
    prev_pos = pos;
    if (--pos < left_offset + 1) pos = left_offset + 1;
    redrawFigure(ang, prev_pos, height);
  }
}

// проверка на столкновения
boolean checkArea(int8_t check_type) {
  // check type:
  // 0 - проверка лежащих фигур и пола
  // 1 - проверка стенки справа и фигур
  // 2 - проверка стенки слева и фигур
  // 3 - проверка обеих стенок и пола

  boolean flag = true;
  int8_t X, Y;
  boolean offset = 1;
  int8_t this_ang = ang;

  // этот режим для проверки поворота. Поэтому "поворачиваем"
  // фигуру на следующий угол чтобы посмотреть, не столкнётся ли она с чем
  if (check_type == 3) {
    this_ang++;
    this_ang = this_ang % 4;
    offset = 0;   // разрешаем оказаться вплотную к стенке
  }

  byte x_st = left_offset + 1;
  byte x_en = right_offset - 1;

  for (byte i = 0; i < 4; i++) {
    // проверяем точки фигуры
    // pos, height - координаты главной точки фигуры в ГЛОБАЛЬНОЙ системе координат
    // X, Y - координаты остальных трёх точек в ГЛОБАЛЬНОЙ системе координат
    if (i == 0) {   // стартовая точка фигуры (начало отсчёта)
      X = pos;
      Y = height;
    } else {        // остальные три точки
      // получаем глобальные координаты точек, прибавив их положение в
      // системе координат главной точки к координатам самой главной
      // точки в глобальной системе координат. Если ты до сюда дочитал,
      // то стукни мне на почту alex@alexgyver.ru . Печенек не дам, но ты молодец!
      X = pos + (int8_t)pgm_read_byte(&figures[fig][this_ang * 3 + i - 1][0]);
      Y = height + (int8_t)pgm_read_byte(&figures[fig][this_ang * 3 + i - 1][1]);

      // дичь дикая! Но на самом деле просто восстанавливаем из прогмема данные
      // и берём нужное число в массиве. Откуда все эти * 3 -1 ... можно додуматься
    }

    // границы поля
    if (check_type == 1 || check_type == 3) {
      if (X + 1 > x_en) flag = false;    // смотрим следующий справа
      uint32_t getColor = 0;
      if (Y < pHEIGHT)
        getColor = getPixColorXY(X + offset, Y);
      if ((getColor != color) && (getColor != 0x000000)) {
        flag = false;         // если не СВОЙ цвет и не чёрный
      }
    }
    
    if (check_type == 2 || check_type == 3) {
      if (X - 1 < x_st) flag = false;    // смотрим следующий слева
      uint32_t getColor = 0;
      if (Y < pHEIGHT)
        getColor = getPixColorXY(X - offset, Y);
      if ((getColor != color) && (getColor != 0x000000)) {
        flag = false;         // если не СВОЙ цвет и не чёрный
      }
    }

    if (check_type == 0 || check_type == 3) {
      uint32_t getColor = 0;
      if (Y < pHEIGHT) {
        getColor = getPixColorXY(X, Y - 1);
        if ((getColor != color) && (getColor != 0x000000)) {
          flag = false;         // если не СВОЙ цвет и не чёрный
        }
      }
    }
  }
  return flag;    // возвращаем глобальный флаг, который говорит о том, столкнёмся мы с чем то или нет
}

// функция, которая удаляет текущую фигуру (красит её чёрным)
// а затем перерисовывает её в новом положении
void redrawFigure(int8_t clr_ang, int8_t clr_pos, int8_t clr_height) {
  drawFigure(fig, clr_ang, clr_pos, clr_height, 0x000000);            // стереть фигуру
  drawFigure(fig, ang, pos, height, color);                           // отрисовать
}

// функция, отрисовывающая фигуру заданным цветом и под нужным углом
void drawFigure(byte figure, byte angle, byte x, byte y, uint32_t color) {
  drawPixelXY(x, y, color);         // рисуем точку начала координат фигуры
  int8_t X, Y;                      // вспомогательные
  for (byte i = 0; i < 3; i++) {    // рисуем 4 точки фигуры
    // что происходит: рисуем фигуру относительно текущей координаты падающей точки
    // просто прибавляем "смещение" из массива координат фигур
    // для этого идём в прогмем (функция pgm_read_byte)
    // обращаемся к массиву по адресу &figures
    // преобразовываем число в int8_t (так как progmem работает только с "unsigned"
    // angle * 3 + i - обращаемся к координатам согласно текущему углу поворота фигуры

    X = x + (int8_t)pgm_read_byte(&figures[figure][angle * 3 + i][0]);
    Y = y + (int8_t)pgm_read_byte(&figures[figure][angle * 3 + i][1]);
    if (Y > pHEIGHT - 1) continue;   // если выходим за пределы поля, пропустить отрисовку
    drawPixelXY(X, Y, color);
  }
}
