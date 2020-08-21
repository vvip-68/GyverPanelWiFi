
// ****************** ПИНЫ ПОДКЛЮЧЕНИЯ *******************

// Внимание!!! При использовании платы микроконтроллера Wemos D1 (xxxx) и выбранной в менеджере плат платы "Wemos D1 (xxxx)"
// прошивка скорее всего нормально работать не будет. 
// Наблюдались следующие сбои у разных пользователей:
// - нестабильная работа WiFi (постоянные отваливания и пропадание сети) - попробуйте варианты с разным значением параметров компиляции IwIP в Arduino IDE
// - прекращение вывода контрольной информации в Serial.print() - в монитор порта не выводится
// - настройки в EEPROM не сохраняются
// Думаю все эти проблемы из за некорректной работы ядра ESP8266 для платы (варианта компиляции) Wemos D1 (xxxx) в версии ядра ESP8266 2.5.2
// Диод на ногу питания Wemos как нарисовано в схеме от Alex Gyver не ставить (!!!), конденсатор по питанию для NodeMCU - желателен, для Wemos - обязателен (!!!)
// Пины подписаны согласно pinout платы, а не надписям на пинах!

// ================== Тестовый стенд =====================

#if defined(ESP8266)
#define WIDTH 48              // ширина матрицы
#define HEIGHT 16             // высота матрицы
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)
#define DEVICE_TYPE 1         // Использование матрицы: 0 - свернута в трубу для лампы; 1 - плоская матрица в рамке   
#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 1    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 3     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
#define USE_MP3 0             // поставьте 0, если у вас нет звуковой карты MP3 плеера
#define USE_TM1637 0          // поставьте 1, если используется дополнительный индикатор TM1637, 0 если индикатора нет
#define USE_WEATHER 1                    // 1 - использовать получение информации о текущей погоде; 0 - не использовать 

#define LED_PIN 2             // D2 пин ленты
#define STX D3                // D3 of ESP8266, connect to RX of DFPlayer module
#define SRX D4                // D4 of ESP8266, connect to TX of DFPlayer module
#define PIN_BTN D1            // кнопка подключена сюда (D4 --- КНОПКА --- GND)
#define DIO D5                // D5 TM1637 display DIO pin - не используется
#define CLK D7                // D7 TM1637 display CLK pin - не используется
#endif

// ============== Гирлянда на балконе =====================

/*
#if defined(ESP8266)
#define WIDTH 50              // ширина матрицы
#define HEIGHT 20             // высота матрицы
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)
#define DEVICE_TYPE 1         // Использование матрицы: 0 - свернута в трубу для лампы; 1 - плоская матрица в рамке   
#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 0    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 0     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
#define USE_MP3 0             // поставьте 0, если у вас нет звуковой карты MP3 плеера
#define USE_TM1637 0          // поставьте 1, если используется дополнительный индикатор TM1637, 0 если индикатора нет
#define USE_WEATHER 1                    // 1 - использовать получение информации о текущей погоде; 0 - не использовать 

#define LED_PIN 2             // D2 пин ленты
#define STX D3                // D3 of ESP8266, connect to RX of DFPlayer module
#define SRX D5                // D4 of ESP8266, connect to TX of DFPlayer module
#define PIN_BTN D4            // кнопка подключена сюда (D4 --- КНОПКА --- GND)
#define DIO D5                // D5 TM1637 display DIO pin - не используется
#define CLK D7                // D7 TM1637 display CLK pin - не используется
#endif
*/

// =======================================================

/*
 * NodeMCU ESP32
 * Физическое подключение:
 * Матрица зигзаг, левый нижний угол, направление вапрво
 * Пин ленты    - 2
 * Пин кнопки   - 4
 * MP3Player    - 17 к RX, 16 к TX плеера 
 * TM1637       - 23 к DIO, 22 к CLK
 * В менеджере плат выбрано "ESP32 Dev Module"
 */ 
#if defined(ESP32)
#define WIDTH 16              // ширина матрицы
#define HEIGHT 16             // высота матрицы
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)
#define DEVICE_TYPE 1         // Использование матрицы: 0 - свернута в трубу для лампы; 1 - плоская матрица в рамке   
#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 3    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 1     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
#define USE_MP3 0             // поставьте 0, если у вас нет звуковой карты MP3 плеера
#define USE_TM1637 0          // поставьте 1, если используется дополнительный индикатор TM1637? 0 если индикатора нет
#define USE_WEATHER 1                    // 1 - использовать получение информации о текущей погоде; 0 - не использовать 

#define LED_PIN (2U)          // пин ленты, физически подключена к пину D2 на плате
#define SRX (16U)             // 16 is RX of ESP32, connect to TX of DFPlayer
#define STX (17U)             // 17 is TX of ESP32, connect to RX of DFPlayer module
#define PIN_BTN (4U)          // кнопка подключена сюда (PIN --- КНОПКА --- GND)
#define DIO (23U)             // TM1637 display DIO pin
#define CLK (22U)             // TM1637 display CLK pin
#endif

// =======================================================

// ************** ИСПОЛЬЗУЕМЫЕ БИБЛИОТЕКИ ****************

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
#endif

#if defined(ESP32)
  #include <ESPmDNS.h>
#endif

#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>         // Библиотека для разбора JSON-ответа от червия Yandex-Погода
#include <TimeLib.h>
#include <EEPROM.h>
#include <FastLED.h>             // Установите в менеджере библиотек стандартную библиотеку FastLED
#include "TM1637Display.h"       // Внимание!!! Библиотека в папке проекта libraries изменена - константы букв и цифр переименованы с вида _1, _A на _1_, _A_ из за ошибок компиляции для ESP32
#include "timerMinim.h"
#include "GyverButton.h"
#include "fonts.h"

#if (USE_MP3 == 1)
#include <SoftwareSerial.h>      // Установите в менеджере библиотек "EspSoftwareSerial" для ESP8266/ESP32 https://github.com/plerup/espsoftwareserial/
#include "DFRobotDFPlayerMini.h" // Установите в менеджере библиотек стандартную библиотеку DFRobotDFPlayerMini ("DFPlayer - A Mini MP3 Player For Arduino" )
#endif

// =======================================================

#if defined(ESP32)
  #include <WiFi.h>
#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
  #define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#endif

// Comment the next line to stop useing hardware randomizer for initial random seed. 
// So reading analog input 0 + microseconds will be used instead
#define TRUE_RANDOM

// =======================================================

#define COLOR_ORDER GRB         // порядок цветов на ленте. Если цвет отображается некорректно - меняйте. Начать можно с RGB
#define BRIGHTNESS 32           // стандартная маскимальная яркость (0-255)

uint16_t CURRENT_LIMIT = 15000; // лимит по току в миллиамперах, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит

// =======================================================
