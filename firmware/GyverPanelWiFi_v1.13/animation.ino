/*
 Как добавить свою анимацию в проект

 1. Запустите приложение JinxFramer из папки Tools. 
    Установите в нем размеры рисунка в поле "Размеры матрицы"
    В разделе "Создать" выберите опцию "Ролик" и укажите количество кадров анимации
    Укажите имя файла в который будет сохранен ваш ролик
    Нажмите кнопку "Создать"
    Нажмите кнопку "Редактировать кадр". 
    Пользуясь инструментами рисования создайте первый кадр анимации.
    Нажмите кнопку "Сохранить".
    Используя трекбар под рисунком или кнопки навигации слева/справа от трекбара перейдите к следующему кадру
    Нарисуйте последовательно оставшиеся кадры анимации.
    В разделе "Сохранение" -> "Сохранить код", нажмите кнопку "Фрагмент".
    Укажите имя файла. Код будет сохранен файл с расширением ".h"
    Найдите в сохраненном файле структуру описания анимации (в самом низу файла) и
    настройте опции анимации - направление и скорость смещения, скорость анимации и другие опции,
    описывающие поведение анимации. Скопируйте файл в папку скетча.

 2. В этом файле (animation.ino) в строках начиная с 79 найдите блок описания файлов анимации и добавьте в #include ваш файл анимации
    В строках, начиная с 84 добавьте имя описателя вашей анимации, как это сделано для других анимаций, определенных в скетче    
    Имя описателя анимации смотрите в вашем подключенном файле анимации.
    В строке 90 добавьте имя своей анимации, как оно будет отображаться в приложении на телефоне
    #define IMAGE_LIST F("Сердце,Марио,Погода")   // Добавьте в список имя вашей анимации
*/

#define MAX_FRAMES_COUNT 20

// Структура описания анимации
typedef struct {
  const uint16_t* frames[MAX_FRAMES_COUNT];

  int8_t     start_x;                // Позиция отображения X (начальная)
  int8_t     start_y;                // Позиция отображения Y (начальная)
  uint16_t   options;                // Битовый флаг дополнительных параметров картинки
                                     //   1 - центрировать по горизонтали (позиция start_x игнорируется)
                                     //   2 - центрировать по вертикали (позиция start_y игнорируется)
                                     //   4 - есть прозрачные пиксели, цвет прозрачности в transparent_color
                                     //   8 - перед отрисовкой ПЕРВОГО кадра закрашивать всю матрицу цветом background_first_color
                                     //  16 - перед отрисовкой СЛЕДУЮЩИХ кадров закрашивать всю матрицу цветом background_color                                    
                                     //  64 - начальное отображение - зеркальное по оси х
                                     // 128 - начальное отображение - зеркальное по оси y
  uint8_t    frame_width;            // Ширина картинки (фрейма)
  uint8_t    frame_height;           // Высота картинки (фрейма)
  uint8_t    row_draw_direction;     // Направление построчного рисования (битовые флаги):
                                     // Биты 0 и 1 (маска 0x03)
                                     //   0 (0x00) - сверху вниз
                                     //   1 (0x01) - снизу вверх
                                     //   2 (0x10) - слева направо
                                     //   3 (0x11)- справа налево
  uint16_t   draw_frame_interval;    // Интервал отрисовки очередной порции картинки анимации (строка при построчной анимации или кадр при покадровой)
  uint8_t    draw_row_interval;      // Задержка мс между отрисовкой строк изображения (если 0 - рисуется кадр целиком, а не построчно)
  uint16_t   move_x_interval;        // Смещение по оси X каждые N мс
  uint16_t   move_y_interval;        // Смещение по оси Y каждые N мс
    
  uint16_t   move_type;              // Тип движения - битовый флаг (разрешено только при отрисовке кадра целиком, при построчном - не работает): 
                                     //   0 - нет; 
                                     //   1 - из начальной позиции вправо
                                     //   2 - из начальной позиции влево
                                     //   4 - из начальной позиции вверх
                                     //   8 - из начальной позиции вниз
                                     //  16 - отражение при достижении границы по горизонтали
                                     //  32 - отражение при достижении границы по вертикали
                                     //  64 - переворот при отражении по горизонтали
                                     // 128 - переворот при отражении по вертикали
                                     // 256 - выходить за боковые рамки (при установленном 16 - отражение по горизонтали); 1 - выходить; 0 - не выходить 
                                     // 512 - выходить за верх/низ (при установленном 32 - отражение по вертикали); 1 - выходить; 0 - не выходить
  uint32_t   transparent_color;      // Этот цвет - прозрачный, пиксели этого цвета не рисуются
  uint32_t   background_first_color; // Цвет заливки ВСЕЙ матрицы перед тем, как рисовать самый первый фрейм при активации эффекта анимации
  uint32_t   background_color;       // Цвет заливки ВСЕЙ матрицы перед тем, как рисовать очередной фрейм  
} animation_t;
 
uint8_t  max_image_num;              // Количество используемых анимаций
uint8_t* use_animations;             // Массив с индексами доступных к использованию анимаций из массива animations
String   animations_list;            // Список имен анимаций, который будет передан в приложение на смартфоне

// -----------------------------------------------------------------------------------------------------------

#include "heart.h"                   // Сердце
#include "mario.h"                   // Марио
#include "weather.h"                 // Картинки погоды

const animation_t animations[] = {
  animation_heart,                   // Сердце -- не удалять!!! Эта анимация имеет размер 8x8 и отображается для ВСЕХ размеров матрицж Должна быть всегда самой первой в списке !!!
  animation_mario,                   // Марио
  animation_weather                  // Картинки погоды 
};

// Список картинок, доступных для отрисовки в эффекте "Анимация"
#define IMAGE_LIST F("Сердце,Марио,Погода")   

#define MAX_IMAGE_WIDTH   16         // Здесь указаны максимальные размеры картинки, используемые в прошивке для которого нужен оверлей
#define MAX_IMAGE_HEIGHT  16         // Если картинка не использует эффекты в качестве бакграунда - оверлей не нужен
                                     // Не указывайте оверлей больше чем нужно - это расходует RAM
// -----------------------------------------------------------------------------------------------------------

void initAnimations() {  
  
  // Полный список анимаций, которые определены в приложении
  String  anim_name;
  String  name_list = String(IMAGE_LIST);
  uint8_t image_num = CountTokens(name_list, ',');
  uint8_t use_num = 0;  
  
  use_animations = new uint8_t[image_num];

  // Анимация "Сердце" имеет размеры 8x8 и предназначена для отображения на любых размерах матриц
  use_animations[0] = use_num++;
  anim_name = GetToken(name_list, 1, ',');
  animations_list += anim_name + ",";
  
  DEBUGLN(F("\nНайденные анимации:"));
  DEBUGLN(String(F("   ")) + anim_name + "\t" + String(animations[0].frame_width) + "x" + String(animations[0].frame_height));

  // Другие анимации добавляются в список используемых, только если размер матрицы позволяет отображать
  // анимацию с заданными в ней размерами
  FOR_i(1, image_num) {
    int8_t anim_width  = animations[i].frame_width;
    int8_t anim_height = animations[i].frame_height;
    if (anim_width <= pWIDTH && anim_height <= pHEIGHT) {
      anim_name = GetToken(name_list, i + 1, ',');
      animations_list += anim_name + ",";
      use_animations[use_num++] = i;
      DEBUGLN(String(F("   ")) + anim_name + "\t" + String(anim_width) + "x" + String(anim_height));
    }
  }
  DEBUGLN();
  
  // Удалить последнюю запятую из списка имен анимаций
  uint8_t len = animations_list.length();
  animations_list = animations_list.substring(0, len - 1);  
  max_image_num = use_num;
}

// ------------------- Загрузка картинок и фреймов анимации -------------------

int8_t currentImageIdx = 1;               // Текущая отрисовываемая анимация

animation_t image_desc;                   // Структура с параметрами отрисовки анимации

int8_t   pos_x = 0, pos_y = 0;            // Текущая позиция вывода изображения
int8_t   edge_left = 0, edge_right = 0;   // Граница где происходит разворот движения. Зависит от того выходит картинка ЗА размеры матрицы или разворачивается обратно, когда еще видима
int8_t   edge_bottom = 0, edge_top = 0;   // Граница где происходит разворот движения. Зависит от того выходит картинка ЗА размеры матрицы или разворачивается обратно, когда еще видима
int8_t   rcNum = 0;                       // Номер строки/колонки в кадре, если идет отрисовка по строкам/колонкам
uint8_t  frameNum = 0;                    // Номер кадра в анимации
uint8_t  frames_in_image = 0;             // Количество фреймов в картинке
bool     first_draw = false;              // Отрисовка самого первого кадра после включения эффекта   
bool     frame_completed = false;         // Отрисовка кадра завершена (при каждом кадре если рисуется покадрово или после отрисовки всех строк кадра, если рисуется построчно)
bool     image_completed = false;         // Отрисовка всех кадров изображения завершена
bool     draw_by_row = false;             // Эта картинка рисуется построчно, а не покадров
bool     flip_x = false;                  // Картинка зеркально отражена по оси X
bool     flip_y = false;                  // Картинка зеркально отражена по оси Y
bool     inverse_dir_x = false;           // Произошла смена направления движения при движении по горизонтали
bool     inverse_dir_y = false;           // Произошла смена направления движения при движении по горизонтали
uint32_t last_draw_row = 0;               // Время последнего обращения к процедуре отрисовки строки изображения, если рисуем картинку построчно
uint32_t last_draw_frame = 0;             // Время последней отрисовки полного кадра изображения
uint32_t last_move_x = 0;                 // Время последнего смещения картинки по оси X
uint32_t last_move_y = 0;                 // Время последнего смещения картинки по оси Y

// Загрузка в RAM описателя картинки из PROGMEM
void loadDescriptor(const animation_t (*src_desc)) {
  //  Читаем всю структуру desc из PROGMEM в image_desc
  memcpy_P ((void *)&image_desc, src_desc, sizeof( animation_t )); 
}

// Отрисовка строки изображения
void drawImageRow(uint8_t row, const uint16_t (*frame)) {  
  
  if (!frame) return;  

  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  int8_t y = flip_y ? pos_y + image_desc.frame_height - row - 1 : pos_y + row;
  if (y < 0  || y > pHEIGHT - 1) return;
  
  for (uint8_t i = 0; i < image_desc.frame_width; i++) {
    
    int8_t x = flip_x ? pos_x + image_desc.frame_width - i - 1 : pos_x + i;
    if (x < 0 || x > pWIDTH - 1) continue;
    
    uint16_t clr = (pgm_read_word(&(frame[(image_desc.frame_height - row - 1) * image_desc.frame_width + i])));

    // Это "прозрачный" пиксель?
    if ((image_desc.options & 4) > 0 && clr == image_desc.transparent_color) continue;

    // Расширяем цвет от 16-битного к 24-битному
    CRGB color = gammaCorrection(expandColor(clr));
    color.nscale8_video(effectBrightness);

    // Рисуем пиксель    
    drawPixelXY(x, y, color);        
  }
}

// Отрисовка колонки изображения
void drawImageCol(uint8_t col, const uint16_t (*frame)) {  

  if (!frame) return;
  
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));

  int8_t x = flip_x ? pos_x + image_desc.frame_width - col - 1 : pos_x + col;
  if (x < 0  || x > pWIDTH - 1) return;
  
  for (uint8_t i = 0; i < image_desc.frame_width; i++) {
    
    int8_t y = flip_y ? pos_y + image_desc.frame_height - i - 1 : pos_y + i;
    if (y < 0 || y > pHEIGHT - 1) continue;
    
    uint16_t clr = (pgm_read_word(&(frame[(image_desc.frame_height - i - 1) * image_desc.frame_width + col])));

    // Это "прозрачный" пиксель?
    if ((image_desc.options & 4) > 0 && clr == image_desc.transparent_color) continue;

    // Расширяем цвет от 16-битного к 24-битному
    CRGB color = gammaCorrection(expandColor(clr));
    color.nscale8_video(effectBrightness);

    // Рисуем пиксель
    drawPixelXY(x, y, color);    
  }
}

// Отрисовка кадра изображения целиком
void loadImageFrame(const uint16_t (*frame)) { 
  rcNum = 0;
  for (uint8_t j = 0; j < image_desc.frame_height; j++) {
    drawImageRow(j, frame);
  }      
}

void animationRoutine() {
  
  const uint16_t *ppFrame;
  
  uint8_t effectBrightness = getBrightnessCalculated(globalBrightness, getEffectContrastValue(thisMode));  

  // ------------- ИНИЦИАЛИЗАЦИЯ ПАРАМЕТРОВ --------------

  if (loadingFlag) {
    FastLED.clear();

    currentImageIdx = (specialTextEffectParam >= 0) ? specialTextEffectParam : getEffectScaleParamValue2(thisMode);

    // Индексы доступных картинок - от  1 до max_image_num;
    // Eсли currentImageIdx == 0 - брать случайную картинку
    if (currentImageIdx == 0 || currentImageIdx > max_image_num) {
      // При малом диапазоне генерации чисел (например random8(1,2)) почему-то всегда возвращается 1 и никогда 2.
      // То есть всё время будет отображаться одна и та же картинка с индексом 1
      // Чтобы они отображались хотя бы попеременно - проверяем - если очередная генерация выдала тот же номер, что отображался последним - брать другой.
      // Делаем 6 попыток.
      uint8_t att = 0;
      uint8_t idx = random8(1,max_image_num);
      while (max_image_num > 1 && idx == currentImageIdx && att < 6) {
        att++; idx++;
        if (idx > max_image_num) idx = 1;
      }        
      currentImageIdx = idx;
    }
    
    animation_t anim = animations[use_animations[currentImageIdx - 1]];
    frames_in_image = 0;
    FOR_i(0, MAX_FRAMES_COUNT) {
      if (anim.frames[i] == NULL) break;
      frames_in_image++;
    }
    loadDescriptor(&anim);
    
    frameNum = 0;
    last_draw_row = 0; 
    last_draw_frame = 0;
    
    pos_x = image_desc.start_x; 
    pos_y = image_desc.start_y;

    // Если в опциях задано центрирование рисунка - применить его
    if ((image_desc.options & 1) > 0) {
      pos_x = (pWIDTH - image_desc.frame_width) / 2;
    }
    if ((image_desc.options & 2) > 0) {
      pos_y = (pHEIGHT - image_desc.frame_height) / 2;
    }

    // Начальная отрисовка - зеркальная по оси X, Y
    flip_x = (image_desc.options &  64) > 0;
    flip_y = (image_desc.options & 128) > 0;
    
    inverse_dir_x = false;
    inverse_dir_y = false;
    
    if (image_desc.draw_frame_interval < 5) image_desc.draw_frame_interval = 5;
     
    // Если задан интервал между отрисовкой строк - рисуем построчно
    // Если межстрочный интервал зада нулевым - рисуем покадрово
    draw_by_row = image_desc.draw_row_interval > 0;
    switch (image_desc.row_draw_direction) {
      case 0:  rcNum = image_desc.frame_height - 1; break;
      case 3:  rcNum = image_desc.frame_width - 1; break;
      default: rcNum = 0; break;
    }

    // Нужна заливка всей матрицы перед отрисовкой самого первого кадра?
    if ((image_desc.options & 8) > 0) {
      // В опциях есть заливка бакграунда перед отрисовкой самого первого кадра
      CRGB color = image_desc.background_first_color;
      color.nscale8_video(effectBrightness);
      fillAll(color);
    } else if ((image_desc.options & 16) > 0) {
      // В опциях есть заливка бакграунда перед отрисовкой каждого кадра
      CRGB color = image_desc.background_color;
      color.nscale8_video(effectBrightness);
      fillAll(image_desc.background_color);
    }

    loadingFlag = false;    

    first_draw = true;
    image_completed = false;
    frame_completed = false;    
    
    last_move_x = millis();
    last_move_y = millis();
  }

  // ---------- КОНЕЦ ИНИЦИАЛИЗАЦИИ ПАРАМЕТРОВ -----------

  // -----------------------------------------------------
  // -----------------------------------------------------
  // -----------------------------------------------------
  
  // ------- ТАЙМЕРЫ ДВИЖЕНИЯ И ОТРИСОВКИ КАДРОВ ---------

  // -----------------------------------------------------
  // Для картинки задано движение (горизонталь / вертикаль)? 
  // -----------------------------------------------------
  if (!draw_by_row && image_desc.move_type != 0) {

    // -------------------------------
    // Пришло время смещения по оси X?
    // -------------------------------
    if ((image_desc.move_x_interval > 0) && (millis() - last_move_x > image_desc.move_x_interval)) {
      last_move_x = millis();
      edge_left  = ((image_desc.move_type & 256) > 0) ? (0 - image_desc.frame_width) : 0;
      edge_right = ((image_desc.move_type & 256) > 0) ? pWIDTH : pWIDTH - image_desc.frame_width;
      
      // Направление движение - вправо, не инвертировано (движение до отскока);
      // Направление движение - влево, инвертировано (движение после отскока);
      if ( ((image_desc.move_type & 0x01) > 0 && !inverse_dir_x) ||
           ((image_desc.move_type & 0x02) > 0 && inverse_dir_x)) {
        pos_x++;
        if (pos_x > edge_right) {
          if ((image_desc.move_type & 16) > 0 && (image_desc.frame_width < pWIDTH || (image_desc.move_type & 256) > 0 )) {
            inverse_dir_x = !inverse_dir_x;
          }
          if ((image_desc.move_type & 64) > 0 && (image_desc.frame_width < pWIDTH || (image_desc.move_type & 256) > 0 )) {
            flip_x = !flip_x;
          }
          if ((image_desc.move_type & 0x01))
            pos_x = inverse_dir_x ? edge_right : edge_left;
          else if ((image_desc.move_type & 0x02) > 0) 
            pos_x = inverse_dir_x ? edge_left : edge_right;
        }
      } else
      
      // Направление движение - влево, не инвертировано (движение до отскока);
      // Направление движение - вправо, инвертировано (движение после отскока);
      if ( ((image_desc.move_type & 0x02) > 0 && !inverse_dir_x) ||
           ((image_desc.move_type & 0x01) > 0 && inverse_dir_x)) {
        pos_x--;
        if (pos_x < edge_left) {
          if ((image_desc.move_type & 16) > 0 && (image_desc.frame_width < pWIDTH || (image_desc.move_type & 256) > 0 )) {
            inverse_dir_x = !inverse_dir_x;
          }
          if ((image_desc.move_type & 64) > 0 && (image_desc.frame_width < pWIDTH || (image_desc.move_type & 256) > 0 )) {
            flip_x = !flip_x;
          }
          if ((image_desc.move_type & 0x01))
            pos_x = inverse_dir_x ? edge_right : edge_left;
          else if ((image_desc.move_type & 0x02) > 0) 
            pos_x = inverse_dir_x ? edge_left : edge_right;
        }
      }            
    }

    // -------------------------------
    // Пришло время смещения по оси Y?
    // -------------------------------
    if ((image_desc.move_y_interval > 0) && (millis() - last_move_y > image_desc.move_y_interval)) {
      last_move_y = millis();
      edge_bottom  = ((image_desc.move_type & 512) > 0) ? (0 - image_desc.frame_height) : 0;
      edge_top = ((image_desc.move_type & 512) > 0) ? pHEIGHT : pHEIGHT - image_desc.frame_height;
      
      // Направление движение - вправо, не инвертировано (движение до отскока);
      // Направление движение - влево, инвертировано (движение после отскока);
      if ( ((image_desc.move_type & 0x04) > 0 && !inverse_dir_y) ||
           ((image_desc.move_type & 0x08) > 0 && inverse_dir_y)) {
        pos_y++;
        if (pos_y > edge_top) {
          if ((image_desc.move_type & 32) > 0 && (image_desc.frame_height < pHEIGHT || (image_desc.move_type & 512) > 0 )) {
            inverse_dir_y = !inverse_dir_y;
          }
          if ((image_desc.move_type & 128) > 0 && (image_desc.frame_height < pHEIGHT || (image_desc.move_type & 512) > 0 )) {
            flip_y = !flip_y;
          }
          if ((image_desc.move_type & 0x04))
            pos_y = inverse_dir_y ? edge_top : edge_bottom;
          else if ((image_desc.move_type & 0x08))
            pos_y = inverse_dir_y ? edge_bottom : edge_top;
        }
      } else
      
      // Направление движение - влево, не инвертировано (движение до отскока);
      // Направление движение - вправо, инвертировано (движение после отскока);
      if ( ((image_desc.move_type & 0x08) > 0 && !inverse_dir_y) ||
           ((image_desc.move_type & 0x04) > 0 && inverse_dir_y)) {
        pos_y--;
        if (pos_y < edge_bottom) {
          if ((image_desc.move_type & 32) > 0 && (image_desc.frame_height < pHEIGHT || (image_desc.move_type & 512) > 0 )) {
            inverse_dir_y = !inverse_dir_y;
          }
          if ((image_desc.move_type & 128) > 0 && (image_desc.frame_height < pHEIGHT || (image_desc.move_type & 512) > 0 )) {
            flip_y = !flip_y;
          }
          if ((image_desc.move_type & 0x04))
            pos_y = inverse_dir_y ? edge_top : edge_bottom;
          else if ((image_desc.move_type & 0x08))
            pos_y = inverse_dir_y ? edge_bottom : edge_top;
        }
      }            
    }    
  }
  
  // -----------------------------------------------------
  // Пришло время отрисовки следующей строки (если рисуется построчно)? 
  // -----------------------------------------------------
  if (draw_by_row && !frame_completed) {
    if (millis() - last_draw_row < image_desc.draw_row_interval) return;
  }
  
  // -----------------------------------------------------
  // Пришло время отрисовки следующего кадра?
  // Если в качестве бакграунда используется какой-либо эффект - отрисовка происходит непрерывно, 
  // а вот переход к следующему кадру - с указанным интервалом
  // -----------------------------------------------------

  bool need_change_frame = millis() - last_draw_frame >= image_desc.draw_frame_interval;

  if (draw_by_row && !need_change_frame) return; 

  // Нужна заливка всей матрицы перед отрисовкой очередного кадра?  
  if (!first_draw && frame_completed && ((image_desc.options & 16) > 0)) {
    CRGB color = image_desc.background_color;
    color.nscale8_video(effectBrightness);
    fillAll(image_desc.background_color);
  }

  first_draw = false;
  frame_completed = false;
  image_completed = false;
    
  // -----------------------------------------------

  animation_t anim = animations[use_animations[currentImageIdx - 1]];
  ppFrame = anim.frames[frameNum];

  // -----------------------------------------------
  // Рисуем построчно - слева направо, справа налево, снизу вверх, сверху вниз
  // -----------------------------------------------
  if (draw_by_row) {
    if (image_desc.row_draw_direction < 2) {
      // Сверху вниз или /Снизу вверх
      drawImageRow(rcNum, ppFrame);
    } else {
      // Слева направо или Справа налево
      drawImageCol(rcNum, ppFrame);
    }          
  } else 
  
  // -----------------------------------------------
  // или - рисуем кадр целиком за раз
  // -----------------------------------------------
  {
    loadImageFrame(ppFrame);
  }

  // -----------------------------------------------
  // Следующая строка / колонка, кадр отрисован полностью?
  // -----------------------------------------------
  if (draw_by_row) {
    if (image_desc.row_draw_direction == 0) {
      // Сверху вниз
      if (--rcNum < 0) {
        frame_completed = true;
        rcNum = image_desc.frame_height - 1;
      }          
    } else if (image_desc.row_draw_direction == 1) {
      // Снизу вверх
      if (++rcNum >= image_desc.frame_height) {
        frame_completed = true;
        rcNum = 0;
      }
    } else if (image_desc.row_draw_direction == 2) {
      // Слева направо
      if (++rcNum >= image_desc.frame_width) {
        frame_completed = true;
        rcNum = 0;
      }
    } else if (image_desc.row_draw_direction == 3) {
      // Справа налево
      if (--rcNum < 0) {
        frame_completed = true;
        rcNum = image_desc.frame_width - 1;
      }          
    }
    last_draw_row = millis();
  } else 
  
  // -----------------------------------------------
  // Кадр отрисован полностью весь за один проход
  // -----------------------------------------------
  {
    frame_completed = true;
  }

  // -----------------------------------------------
  // Если кадр отрисован полностью  - брать следующий по циклу
  // -----------------------------------------------
  if (frame_completed && need_change_frame) {    
    last_draw_frame = millis();
    if (++frameNum >= frames_in_image) {
      image_completed = true;
      frameNum = 0;
    }
  }
}

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------


// Gamma коррекция (Default Gamma = 2.8)
const uint8_t PROGMEM gammaR[] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
   2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,
   5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,
   9,  9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14,
  15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
  23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33,
  33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 46,
  46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
  62, 63, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 80,
  81, 83, 84, 85, 87, 88, 89, 91, 92, 94, 95, 97, 98, 99, 101, 102,
  104, 105, 107, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123, 125, 127, 128,
  130, 132, 134, 135, 137, 139, 141, 143, 145, 146, 148, 150, 152, 154, 156, 158,
  160, 162, 164, 166, 168, 170, 172, 174, 177, 179, 181, 183, 185, 187, 190, 192,
  194, 196, 199, 201, 203, 206, 208, 210, 213, 215, 218, 220, 223, 225, 227, 230
};

const uint8_t PROGMEM gammaG[] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
   2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
   5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

const uint8_t PROGMEM gammaB[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,
  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,  7,  8,
  8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 11, 11, 12, 12, 12, 13,
  13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 19,
  20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 27, 27, 28, 28,
  29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
  40, 41, 42, 43, 44, 44, 45, 46, 47, 48, 49, 50, 51, 51, 52, 53,
  54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 69, 70,
  71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
  90, 92, 93, 94, 96, 97, 98, 100, 101, 103, 104, 106, 107, 109, 110, 112,
  113, 115, 116, 118, 119, 121, 122, 124, 126, 127, 129, 131, 132, 134, 136, 137,
  139, 141, 143, 144, 146, 148, 150, 152, 153, 155, 157, 159, 161, 163, 165, 167,
  169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 196, 198, 200
};

// гамма-коррекция цвет плашки в цвет светодиода (более натуральные цвета)
uint32_t gammaCorrection(uint32_t color) {
  uint8_t r = (color >> 16) & 0xFF;  // Extract the RR byte
  uint8_t g = (color >> 8) & 0xFF;   // Extract the GG byte
  uint8_t b = color & 0xFF;          // Extract the BB byte

  r = pgm_read_byte(&gammaR[r]);
  g = pgm_read_byte(&gammaG[g]);
  b = pgm_read_byte(&gammaB[b]);

  uint32_t newColor = r << 16 | g << 8 | b;
  return newColor;
}

// обратная гамма-коррекция - из цвета светодиода в цвет плашки в программе
uint32_t gammaCorrectionBack(uint32_t color) {
  uint8_t r = (color >> 16) & 0xFF;  // Extract the RR byte
  uint8_t g = (color >> 8) & 0xFF;   // Extract the GG byte
  uint8_t b = color & 0xFF;          // Extract the BB byte
  uint8_t idx, tmp;

  idx = r;
  tmp = pgm_read_byte(&gammaR[idx]);
  while (idx < 255 && r > tmp) {
    tmp = pgm_read_byte(&gammaR[++idx]);
  }
  r = idx;
  
  idx = g;
  tmp = pgm_read_byte(&gammaG[idx]);
  while (idx < 255 && g > tmp) {
    tmp = pgm_read_byte(&gammaG[++idx]);
  }
  g = idx;

  idx = b;
  tmp = pgm_read_byte(&gammaB[idx]);
  while (idx < 255 && b > tmp) {
    tmp = pgm_read_byte(&gammaB[++idx]);
  }
  b = idx;

  uint32_t newColor = r << 16 | g << 8 | b;
  return newColor;
}

// gamma correction для expandColor
static const uint8_t PROGMEM
gamma5[] = {
  0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0b,
  0x0e, 0x11, 0x14, 0x18, 0x1d, 0x22, 0x28, 0x2e,
  0x36, 0x3d, 0x46, 0x4f, 0x59, 0x64, 0x6f, 0x7c,
  0x89, 0x97, 0xa6, 0xb6, 0xc7, 0xd9, 0xeb, 0xff
},
gamma6[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08,
  0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x10, 0x12, 0x13,
  0x15, 0x17, 0x19, 0x1b, 0x1d, 0x20, 0x22, 0x25,
  0x27, 0x2a, 0x2d, 0x30, 0x33, 0x37, 0x3a, 0x3e,
  0x41, 0x45, 0x49, 0x4d, 0x52, 0x56, 0x5b, 0x5f,
  0x64, 0x69, 0x6e, 0x74, 0x79, 0x7f, 0x85, 0x8b,
  0x91, 0x97, 0x9d, 0xa4, 0xab, 0xb2, 0xb9, 0xc0,
  0xc7, 0xcf, 0xd6, 0xde, 0xe6, 0xee, 0xf7, 0xff
};

// преобразовать цвет из 16 битного в 24 битный
static uint32_t expandColor(uint16_t color) {
  return ((uint32_t)pgm_read_byte(&gamma5[ color >> 11       ]) << 16) |
         ((uint32_t)pgm_read_byte(&gamma6[(color >> 5) & 0x3F]) <<  8) |
         pgm_read_byte(&gamma5[ color       & 0x1F]);
}

// ---------------------------------------------------
// Сохранение/Загрузка/Удаление/Получение списка
// изображений, которые пользователь нарисовал на матрице
// в программе WiFiPlayer
// ---------------------------------------------------

String openImage(String storage, String fName, void* lds, bool exactName) {

  File file;
  bool ok = true;
  String message = "";
  String directoryName = "/" + String(pWIDTH) + "p" + String(pHEIGHT);
  String fileName = exactName ? fName : (directoryName + "/" + fName + ".p");

  // Если нет поддержки SD=карты - работать с внутренней файловой системой МК
  if (USE_SD == 0 || USE_SD == 1 && FS_AS_SD == 1) storage = "FS";

  CRGB* alds = (CRGB*)lds;
  
  #if (USE_SD == 1 && FS_AS_SD == 0)
  if (storage == "SD") {    
    if (!SD.exists(directoryName)) {
      ok = SD.mkdir(directoryName);
      if (!ok) {
        message = String(F("Папка для хранения изображений '")) + storage + ":/" + directoryName + String(F("' не найдена."));
        DEBUGLN(message);
        return message;
      }
    }  
    file = SD.open(fileName, FILE_READ);
  }
  #endif

  if (storage == "FS") {    
    if (!LittleFS.exists(directoryName)) {
      ok = LittleFS.mkdir(directoryName);
      if (!ok) {
        message = String(F("Папка для хранения изображений '")) + storage + ":/" + directoryName + String(F("' не найдена."));
        DEBUGLN(message);
        return message;
      }
    }
    file = LittleFS.open(fileName, "r");
  }

  if (!file) {
    message = String(F("Файл '")) + storage + ":/" + fileName + String(F("' не найден."));
    DEBUGLN(message);
    return message;
  }

  size_t len = 0;
  uint8_t buf[3];
  len = file.read(buf, 3);
  ok = len == 3;
  if (!ok) {
    message = String(F("Ошибка чтения файла '")) + storage + ":/" + fileName + "'";
    DEBUGLN(message);
    file.close();
    return message;
  }

  uint8_t w = buf[1];
  uint8_t h = buf[2];

  // При загрузке новой картинки прямо на матрицу - предварительно очистить матрицу
  if (alds == nullptr) FastLED.clear();

  FOR_x(0, w) {
    FOR_y (0, h) {
      len = file.read(buf, 3);
      ok = len == 3;
      if (!ok) {
        message = String(F("Ошибка чтения файла '")) + storage + ":/" + fileName + "'";
        DEBUGLN(message);
        file.close();
        return message;
      }
      
      uint8_t r = buf[0];
      uint8_t g = buf[1];
      uint8_t b = buf[2];
      
      if (alds != nullptr) {
        // Если ссылка на массив НЕ передана - помещаем точку прямо на матрицу
        alds[x + w * y] = CRGB(r << 16 | g << 8 | b);
      } else {
        int8_t offset_x = (pWIDTH - w) / 2;
        int8_t offset_y = (pHEIGHT - h) / 2;
        // Если ссылка на массив передана - помещаем точку в этот массив
        int8_t cx = x + offset_x;
        int8_t cy = y + offset_y;
        if (cx >= 0 && cy >= 0 && cx < pWIDTH && cy < pHEIGHT) {
          int16_t idx = getPixelNumber(cx, cy);
          if (idx >= 0) {
            leds[idx] = CRGB(r << 16 | g << 8 | b);
          }
        }
      }      
    }
  }

  file.close();
  DEBUGLN(F("Файл загружен."));

  return message;
}

String saveImage(String storage, String fName) {
  
  File file;
  bool ok = true;
  String message = "";
  String directoryName = "/" + String(pWIDTH) + "p" + String(pHEIGHT);
  String fileName = directoryName + "/" + fName + ".p";

  // Если нет поддержки SD=карты - работать с внутренней файловой системой МК
  if (USE_SD == 0 || USE_SD == 1 && FS_AS_SD == 1) storage = "FS";

  DEBUG(F("Сохранение файла: "));
  DEBUGLN(storage + String(F(":/")) + fileName);

  #if (USE_SD == 1 && FS_AS_SD == 0)
  if (storage == "SD") {    
    if (!SD.exists(directoryName)) {
      ok = SD.mkdir(directoryName);
      if (!ok) {
        message = String(F("Ошибка создания папки для хранения изображений '")) + directoryName + "'";
        DEBUGLN(message);
        return message;
      }
    }

    // Если файл с таким именем уже есть - удалить (перезапись файла новым)
    if (SD.exists(fileName)) {
      ok = SD.remove(fileName);
      if (!ok) {
        message = String(F("Ошибка создания файла '")) + fileName + "'";
        DEBUGLN(message);
        return message;
      }
    }
  
    file = SD.open(fileName, FILE_WRITE);
  }
  #endif

  if (storage == "FS" || storage == "SD" && FS_AS_SD == 1) {    
    if (!LittleFS.exists(directoryName)) {
      ok = LittleFS.mkdir(directoryName);
      if (!ok) {
        message = String(F("Ошибка создания папки для хранения изображений '")) + directoryName + "'";
        DEBUGLN(message);
        return message;
      }
    }

    // Если файл с таким именем уже есть - удалить (перезапись файла новым)
    if (LittleFS.exists(fileName)) {
      ok = LittleFS.remove(fileName);
      if (!ok) {
        message = String(F("Ошибка создания файла '")) + fileName + "'";
        DEBUGLN(message);
        return message;
      }
    }
  
    file = LittleFS.open(fileName, "w");
  }

  if (!file) {
    message = String(F("Ошибка создания файла '")) + fileName + "'";
    DEBUGLN(message);
    return message;
  }

  size_t len = 0;
  uint8_t buf[] = {0x33, pWIDTH, pHEIGHT};
  len = file.write(buf, 3);
  ok = len == 3;
  if (!ok) {
    message = String(F("Ошибка записи в файл '")) + fileName + "'";
    DEBUGLN(message);
    file.close();
    return message;
  }
  for (uint8_t x = 0; x < pWIDTH; x++) {
    for (uint8_t y = 0; y < pHEIGHT; y++) {
      int16_t idx = getPixelNumber(x,y);
      buf[0] = idx >= 0 ? leds[idx].r : 0;
      buf[1] = idx >= 0 ? leds[idx].g : 0;
      buf[2] = idx >= 0 ? leds[idx].b : 0;
      len = file.write(buf, 3);
      ok = len == 3;
      if (!ok) {
        message = String(F("Ошибка записи в файл '")) + fileName + "'";
        DEBUGLN(message);
        file.close();
        return message;
      }
    }
  }
  file.close();
  DEBUGLN(F("Файл сохранен."));

  return message;
}

String deleteImage(String storage, String fName) {
  bool ok = false;
  String message = "";
  String fileName = "/" + String(pWIDTH) + "p" + String(pHEIGHT) + "/" + fName + ".p";

  DEBUG(F("Удаление файла: "));
  DEBUGLN(storage + String(F(":/")) + fileName);
  
  #if (USE_SD == 1 && FS_AS_SD == 0)
  if (storage == "SD") {
    ok = SD.remove(fileName);
  }
  #endif

  if (storage == "FS" || storage == "SD" && FS_AS_SD == 1) {
    ok = LittleFS.remove(fileName);
  }

  if (!ok) {
    message = String(F("Ошибка удаления файла '")) + fileName + "'";
    DEBUGLN(message);
    return message;
  }
  DEBUGLN(F("Файл удален."));
  
  return message;
}

String getStoredImages(String storage) {

  File entry;
  String list = "";
  String directoryName = "/" + String(pWIDTH) + "p" + String(pHEIGHT);
  
  #if (USE_SD == 1 && FS_AS_SD == 0)
  if (storage == "SD") {
    if (SD.exists(directoryName)) {
      
      String file_name, fn;
      uint32_t file_size;      
      File folder = SD.open(directoryName);
        
      while (folder) {
        entry =  folder.openNextFile();
        
        // Очередной файл найден? Нет - завершить
        if (!entry) break;
            
        if (!entry.isDirectory()) {
                
          file_name = entry.name();
          file_size = entry.size();
    
          fn = file_name;
          fn.toLowerCase();
          
          if (!fn.endsWith(".p") || file_size == 0) {
            entry.close();
            continue;    
          }
    
          // Если полученное имя файла содержит имя папки (на ESP32 это так, на ESP8266 - только имя файла) - оставить только имя файла
          int16_t p = file_name.lastIndexOf("/");
          if (p>=0) file_name = file_name.substring(p + 1);
          file_name = file_name.substring(0, file_name.length() - 2);

          list += "," + file_name;
          entry.close();
        }
      }
    }
  }
  #endif

  if (storage == "FS" || storage == "SD" && FS_AS_SD == 1) {
    if (LittleFS.exists(directoryName)) {
      
      String file_name, fn;
      uint32_t file_size;

      #if defined(ESP32)
        File folder = LittleFS.open(directoryName);
      #else
        Dir  folder = LittleFS.openDir(directoryName);
      #endif
        
      while (true) {

        #if defined(ESP32)
          entry = folder.openNextFile();
          if (!entry) break;
        #else        
          if (!folder.next()) break;
          entry = folder.openFile("r");
        #endif
                       
        if (!entry.isDirectory()) {
                
          file_name = entry.name();
          file_size = entry.size();
    
          fn = file_name;
          fn.toLowerCase();
          
          if (!fn.endsWith(".p") || file_size == 0) {
            entry.close();
            continue;    
          }
    
          // Если полученное имя файла содержит имя папки (на ESP32 это так, на ESP8266 - только имя файла) - оставить только имя файла
          int16_t p = file_name.lastIndexOf("/");
          if (p>=0) file_name = file_name.substring(p + 1);
          file_name = file_name.substring(0, file_name.length() - 2);

          list += "," + file_name;
          entry.close();
        }
      }
    }
  }
  
  if (list.length() > 0) list = list.substring(1);
    
  return list;
}

String GetAnimationsList() {
  return animations_list;
}
