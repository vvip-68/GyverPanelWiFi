#define  GEN_COLOR  CRGB::Green              // Цвет счетчика поколений

uint8_t  **world;                            // Битовый массив клеток поля текущего поколения. бит=1 - клетка "жива", bit=0 - клетка мертва
uint8_t  **gener;                            // Битовый массив клеток поля текущего поколения. бит=1 - клетка "жива", bit=0 - клетка мертва
uint16_t *arr_crc;                           // Служебный массив для расчета контрольных сумм

uint32_t generation_cnt = 0;                 // Счетчик поколений
uint16_t gen_crc[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // контрольная сумма для выявления зацикливаний

uint8_t  BYTE_WIDTH;                            // Массив должен быть кратен 8 битам (сколько байт по ширине)
uint8_t  WORLD_WIDTH;                           // Ширина игрового поля в точках с учетом кратности байту

bool     start_pause;
uint8_t  offset_x;                           // Ширина игрового поля должна быть кратна 8 бит и меньше ширины матрицы
uint32_t lastLifeDraw, lastScore;

// -------------------------------------------------------------------------

/*
   Получить значение жив/мертв для клетки с позицией x,у
*/
bool getXY (uint8_t **world, uint8_t x, uint8_t y) {
  uint8_t value = world[y][x / 8];
  uint8_t mask = 0x80 >> (x % 8);
  return (value & mask) > 0;
}

/*
   Установить значение жив/мертв для клетки с позицией x,у
*/
void setXY (uint8_t **world, uint8_t x, uint8_t y, bool val) {
  uint8_t idx = x / 8;
  uint8_t value = world[y][idx];
  uint8_t mask = 0x80 >> (x % 8);
  if (val) {
    world[y][idx] = value | mask;
  } else
    world[y][idx] = value & ~mask;
}

/*
   Инициализация первого поколения игры псевдослучайными значениями
*/
void init_world(uint8_t **world) {

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));
  CRGB color = CRGB(effectBrightness, effectBrightness, effectBrightness);

  // Отрисовываем имеющимися средствами на игровом боле произвольное двухзначное число
  for (uint8_t i = 0; i < seg_num; i++) {
    uint8_t px = ((seg_size - 7) / 2) + (dir_mx == 0 ? i * seg_size : 0);
    uint8_t py = ((seg_size - 5) / 2) + (dir_mx == 0 ? 0 : i * seg_size);
    
    uint8_t val = random8(10, 99);

    drawDigit3x5(val / 10, px,     py, color);
    drawDigit3x5(val % 10, px + 4, py, color);
  }
  
  FastLEDshow();

  // Считываем с поля клетки в основной массив
  for (uint8_t y = 0; y < pHEIGHT; y++) {
    for (uint8_t x = 0; x < WORLD_WIDTH; x++) {
      bool live = ((uint32_t)getPixColorXY(x + offset_x, y)) != 0;
      setXY(world, x, y, live);
    }
  }

  // Сохранить контрольную сумму нулевого поколения
  for (uint8_t i = 0; i < pHEIGHT; i++) {
    arr_crc[i] = getCrc16((uint8_t*)world[i], BYTE_WIDTH);
  }
  gen_crc[0] = getCrc16((uint8_t*)arr_crc, pHEIGHT * 2);
}

/*
   Вывести на экран игровое поле
*/
void print_world(uint8_t **world) {
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  for (uint8_t x = 0; x < pWIDTH; x++) {
    for (uint8_t y = 0; y < pHEIGHT; y++) {
      if (x >= offset_x && x < pWIDTH - offset_x) {
        CRGB color = getXY(world, x - offset_x, y) ? CRGB(effectBrightness, effectBrightness, effectBrightness) : CRGB::Black;
        drawPixelXY(x, y, color);
      } else {
        drawPixelXY(x, y, CRGB::Black);
      }
    }
  }
}

/*
   Количество живых клеток на игровом поле
*/
uint16_t get_live_count(uint8_t **world) {
  uint16_t count = 0;
  for (uint8_t x = 0; x < WORLD_WIDTH; x++) {
    for (uint8_t y = 0; y < pHEIGHT; y++) {
      if (getXY(world, x, y)) count++;
    }
  }
  return count;
}

/*
   Количество живых соседей у клетки с координатами x, y
*/
uint8_t count_live_neighbors(uint8_t **world, uint8_t x, uint8_t y) {

  // Подготовить массив nb[8][2] координат окружения клетки в позиции x,y
  uint8_t count = 0;
  uint8_t k = 0;
  int8_t  nb[8][2];

  // Получение координат соседей точки (окрестность мура 1 порядка)
  for (int8_t i = x - 1; i <= x + 1; i++) {
    for (int8_t j = y - 1; j <= y + 1; j++) {
      if (i == x && j == y) continue;
      nb[k][0] = i;
      nb[k][1] = j;
      k++;
    }
  }

  for (uint8_t i = 0; i < 8; i++) {
    int8_t ix = nb[i][0];
    int8_t iy = nb[i][1];
    if (ix < 0 || iy < 0 || ix >= WORLD_WIDTH || iy >= pHEIGHT) continue;
    if (getXY(world, ix, iy)) count++;
  }

  return count;
}

/*
   Сгенерировать следующее поколение игрового мира
*/
void next_generation(uint8_t **world, uint8_t **gener) {

  uint8_t live_nb_cnt;
  bool live;

  for (uint8_t i = 0; i < WORLD_WIDTH; i++) {
    for (uint8_t j = 0; j < pHEIGHT; j++) {
      live = getXY(gener, i, j);                            // состояние клетки жива / мертва
      live_nb_cnt = count_live_neighbors(gener, i, j);      // количество живых соседей

      if (!live) {
        if (live_nb_cnt == 3)
          setXY(world, i, j, true);                         // клетка мертва, но есть три живых соседа - зародить жизнь
      } else {
        if (live_nb_cnt < 2 || live_nb_cnt > 3)
          setXY(world, i, j, false);                        // клетка жива, но количество живых соседей не равно двум или трем - убить
      }
    }
  }
}

/*
   вывод счетчика поколений
*/
void print_generation_num() {
  String str = String(generation_cnt);
  int8_t px = pWIDTH - (str.length() * 4) + 1;
  while (px < 0) {
    str = str.substring(1);
    px = pWIDTH - (str.length() * 4);
  }
  for (uint8_t i = 0; i < str.length(); i++) {
    uint8_t val = (uint8_t)(str[i] - '0');
    drawDigit3x5_s(val, px, 0, GEN_COLOR);
    px += 4;
  }
}

// ---------------------------------------------------------------------------------

void lifeRoutine() {

  // Если игра завершена - в течение 5 секунд отображать последнее состояние и мерцающий номер последнего поколения
  if (gameOverFlag) {
    if ((millis() - lastLifeDraw < 5000)) {
      print_world(world);                                    // рисуем текущее поколение
      if (millis() - lastScore > 500) {
        start_pause = !start_pause;
        lastScore = millis();
      }
      if (start_pause) {
        print_generation_num();                              // Напечатать номер поколения
      }
      return;
    } else {
      loadingFlag = true;
      gameOverFlag = false;
    }
  }

  if (loadingFlag) {
    FastLED.clear();
    // modeCode = MC_LIFE;
    loadingFlag = false;

    generation_cnt = 0;
    start_pause = true;

    lastLifeDraw = millis();
    lastScore = lastLifeDraw;

    dir_mx = pWIDTH > pHEIGHT ? 0 : 1;                                   // 0 - квадратные сегменты расположены горизонтально, 1 - вертикально
    seg_num = dir_mx == 0 ? (pWIDTH / pHEIGHT) : (pHEIGHT / pWIDTH);     // вычисляем количество сегментов, умещающихся на матрице
    seg_size = dir_mx == 0 ? pHEIGHT : pWIDTH;                           // Размер квадратного сегмента (высота и ширина равны)
    seg_offset = ((dir_mx == 0 ? pWIDTH : pHEIGHT) - seg_size * seg_num) / (seg_num + 1); // смещение от края матрицы и между сегментами

    BYTE_WIDTH = (pWIDTH / 8);
    WORLD_WIDTH = (BYTE_WIDTH * 8);

    // Определяем отступ по ширине поля
    offset_x = (pWIDTH - WORLD_WIDTH) / 2;

    if (world == NULL)   { world   = new uint8_t*[pHEIGHT]; for (uint8_t i = 0; i < pHEIGHT; i++) { world[i] = new uint8_t [BYTE_WIDTH]; }}
    if (gener == NULL)   { gener   = new uint8_t*[pHEIGHT]; for (uint8_t i = 0; i < pHEIGHT; i++) { gener[i] = new uint8_t [BYTE_WIDTH]; }}
    if (arr_crc == NULL) { arr_crc = new uint16_t[pHEIGHT]; }

    init_world(world);                                   // генерируем первое поколение
  }

  print_world(world);                                    // рисуем текущее поколение
  print_generation_num();                                // Напечатать номер поколения

  // После генерации поля и отображении его на матрице выдерживаем изображение 5 секунд до начала генерации
  if (start_pause && (millis() - lastLifeDraw < 2500)) return;
  start_pause = false;

  uint8_t spd = map8(getEffectSpeedValue(MC_LIFE), 15, 50);

  if (millis() - lastLifeDraw < spd * 10) return;

  lastLifeDraw = millis();

  for (uint8_t i = 0; i < pHEIGHT; i++) {
    memcpy(gener[i], world[i], BYTE_WIDTH);              // сохраняем поколение как "предыдущее" для следующих расчетов
  }
  next_generation(world, gener);                         // генерируем следующее поколение

  for (uint8_t i = 0; i < pHEIGHT; i++) {
    arr_crc[i] = getCrc16((uint8_t*)world[i], BYTE_WIDTH);
  }
  crc = getCrc16((uint8_t*)arr_crc, pHEIGHT * 2);

  bool is_cycle = false;
  for (uint8_t i = 0; i < 8; i++) {
    if (crc == gen_crc[i]) {
      is_cycle = true;
      break;
    }
  }

  bool is_optimal = true;
  for (uint8_t i = 0; i < pHEIGHT; i++) {
    is_optimal = memcmp(gener[i], world[i], BYTE_WIDTH) == 0; // Что-то изменилось от предыдущего поколения к новому?
    if (!is_optimal) break;
  }
  uint16_t live_points = get_live_count(world);               // есть живые клетки?

  gen_crc[generation_cnt % 8] = crc;

  gameOverFlag = (live_points == 0 || is_optimal || is_cycle);

  if (!gameOverFlag) generation_cnt++;
}

void lifeRoutineRelease() {
  if (arr_crc != NULL) {
    delete[] arr_crc;
    arr_crc = NULL;
  }
  if (gener != NULL)   {
    for (uint8_t i = 0; i < pHEIGHT; i++) delete[] gener[i];
    delete[] gener;
    gener = NULL;
  }
  if (world != NULL)   {
    for (uint8_t i = 0; i < pHEIGHT; i++) delete[] world[i];
    delete[] world;
    world = NULL;
  }
}
