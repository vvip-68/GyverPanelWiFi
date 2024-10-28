// Гайд по постройке матрицы: https://alexgyver.ru/matrix_guide/
// Страница проекта на GitHub: https://github.com/vvip-68/GyverPanelWiFi
// Автор идеи, начальный проект - GyverMatrixBT: AlexGyver Technologies, 2019 (https://alexgyver.ru/gyvermatrixbt/)
// Дальнейшее развитие: vvip-68, 2019-2024
// https://AlexGyver.ru/
//
// Дополнительные ссылки для Менеджера плат ESP8266 и ESP32 в Файл -> Настройки
// https://raw.githubusercontent.com/esp8266/esp8266.github.io/master/stable/package_esp8266com_index.json
// https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

#define FIRMWARE_VER F("WiFiPanel v.1.14.2024.2810")

// -------------------------------------------------------------------------------------------------------
//
// Внимание!!!
// Проект разрабатывался и тестировался для плат разработчика на базе 
//   ESP8266 - 'NodeMCU ESP-12 DevKit v1.0' с CH340, CH341, CP2102, 'Wemos d1 mini' различных вариаций - у них у всех одинаковое назначение пинов
//   ESP32   - 'ESP32-WROOM-32 DevKit' 30,32,38-pin - у них одинаковое назначение пинов, но возможно не все выведены на гребенку
//
// Проверена и рекомендлвано к использованию ESP32-плата типа этих:
// https://aliexpress.ru/item/1005004605399313.html  ( ESP32-WROOM-32 )
// https://aliexpress.ru/item/32836372640.html       ( ESP32 D1 Mini  )
//
// Для остальных плат потребуется перепроверка назначения пинов с выяснением какие комбинации работают, а какие нет.
// Внимание!!!
// Рабочие комбинации версий ядра и библиотек:
//
// Версия ядра ESP8266 - 3.1.2   
// Версия ядра ESP32   - 2.0.14
// Версия FastLED      - 3.7.0  // 3.6.0   - смотри комментарий ниже в секции *** FastLED
// 
// Для версии ядра 3.1.2
//   Плату выбираем реально которую используете - для NodeMCU - "NodeMCU 1.0 (ESP12E Module)", для Wemos d1 mini - "LOLIN(WEMOS) D1 mini (clone)"
//   Пин вывода на ленту в конфигурации по умолчанию LED_PIN обозначен как D2 (GPIO4) - вывод идет на D2
//
// Для ядра ESP32 v2.0.14
//   тип микроконтроллера в меню "Инструменты -> Плата" 
//     - для большинства контроллеров выбирать "ESP32 Dev Module" 
//     - другие типы  микроконтроллеров ESP32S2, ESP32S3, ESP32С3 и т.д. в настойщее время номинально не поддерживаются
//       Если хотите использовать их - на свой страх и риск. Придется искать какие пины работают, какие нет.
//       Не вся переферия TM1637, DFPlayer будет работать - нужны существенные изменения скетча для поддержки всех типов плат ESP32
//   более поздние версии ядра не всегда совместимы с FastLED - на некооторых типах плат скетч может вообще не собираеться - ошибки компиляции.
//
// ---
//
//   Для выделения места под файловую систему в меню "Инструменты" Arduino IDE в настройке распределения памяти устройства
//   для стандарного контроллера с 4МБ флэш-памяти памяти на борту устройства выбер//ите вариант: "Partition scheme: Default 4MB with spiff(1.2MB APP/1.5MB SPIFFS)";
//   Если включена поддержка всех возможностий и компилятор ругается на недостаток памяти - придется отказаться от
//   возможности обновления "по воздуху" (OTA, Over The Air) и выбрать вариант распределения памяти устройства "Partition scheme: No OTA (2MB APP/2MB SPIFFS)";
//
//   Важно!!! 
//   Прошивка достигла некоторого предела, при котором при всех включенных возможностях скетч на некоторых контроллерах ESP32 не умещается в памяти.
//   Если при компиляции выдвется ошибка - размер скетча превысил размер доступной памяти и составляет (104%) - вам придется
//     - либо выбирать плату с Большим размером доступной памяти - например 8MB вместо стандартных 4MB
//     - либо чем-то жертвовать, отключая поддержку того или иного оборудования и/или возможности - например, обновление по воздуху - ОТА 
//       При отключении возможности обновления по воздуху в меню "Инструменты", "Partition Scheme" - выберите "No OTA (2MB App / 2MB SPIFFS)"
//     - либо использовать хак от mikewap83 - увеличение раздела под ОТА и скетч
//       - Открыть файл C:\Users\[ИМЯ пользователя]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\boards.txt
//       - Добавить строки в разделе ## IDE 2.0 Seems to not update the value где идет описание PartitionScheme
//          esp32.menu.PartitionScheme.default_1_4MB=4MB with spiffs (1.408MB APP/1.152MB SPIFFS)
//          esp32.menu.PartitionScheme.default_1_4MB.build.partitions=default_1_4MB
//          esp32.menu.PartitionScheme.default_1_4MB.upload.maximum_size=1441792
//       - Скопировать файл default_1_4MB.csv в папку C:\Users\[ИМЯ пользователя]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\tools\partitions
//       - В Arduino IDE 2.x в меню "Инструменты" не появится новый вариант PartitionScheme, пока мы не удалим скешированную папку приложения, для этого 
//         необходимо удалить папку C:\Users\[ИМЯ пользователя]\AppData\Roaming\arduino-ide\
//       - Перезапустить Arduino и выбрать в настройка размер памяти 4M with spiffs (1.408MB APP/1.152MB SPIFFS)
//
//   Сам CSV-файл разметки находится в проекте, в папке Wiki/OTA/mikewap83 - default_1_4MB.zip
//
//   После этого получается 1.408MB на скетч, столько же для OTA обновлений и 1.152MB для хранения папки Data. 
//
// -------------------------------------------------------------------------------------------------------
//
// *** Arduino IDE
//
// Проект собирался и тестировался в двух версиях Arduino IDE - v1.8.19 и v2.3.3 
//
// -------------------------------------------------------------------------------------------------------
//
// *** FastLED
// 
// Последние эксперименты показали, что наиболее стабильная работа достигается при использовании ядер версии 
// ESP8266 - 3.1.2, ESP32 - 2.0.14, библиотеки FastLED версии 3.6.0 или 3.7.0 
// Версии 3.6.x (кроме 3.6.0) не проверялись. 
// На последней на текущий момент версии 3.7.4 эффекты выводятся с подергиванием, используйте 3.6.0 или 3.7.0
//
// -------------------------------------------------------------------------------------------------------
//
// *** ArduinoJSON
//
// Обмен сообщениями с MQTT сервером, получение сведений о погоде с серверов погоды используют JSON формат сообщения
// Скетч использует библиотеку ArduinoJSON последней из "шестых" версий - 6.21.5.
// По заявлению разработчика библиотеки ArduinoJSON версии 6.x оптимизированы для минимизации использования памяти, 
// в то время как более новые версии 7.х имеют расширенный функционал и ориентированы на удобство использования 
// в ущерб размеру используемой памяти. В связи с этим - из за дефицита оперативной память - рекомендуется оставаться
// на версии библиотеки ArduinoJSON 6.21.5, которая находится в папке libraries проекта
//
// -------------------------------------------------------------------------------------------------------
//
// *** MP3 DFPlayer
//
// Используйте версию библиотеки DFPlayer_Mini_Mp3_by_Makuna из папки libraries проекта - это версия 1.1.1
// Более новые версии этой библиотеки из предлагаемых в Library Manager of Arduino IDE (например, 1.2.0) - в отличии от 1.1.1 
// почему-то не находят файлов звуков на SD-карте - скетч при запуске скажет, что MP3 плеер недоступен.
//
// Будет ли работать MP3 DFPlayer - чистая лотерея, зависит от чипа и фаз луны
// В некоторых случаях может быть циклическая перезагрузка после подключения к сети,
// в некоторых случаях - перезагрузка микроконтроллера при попытке проиграть мелодию будильника из приложения
// Но, может и работать нормально.
//
// Вероятнее всего нестабильность работы плеера зависит от версии библиотеки SoftwareSerial, входящей в состав ядра.
// Используйте только рекомендованные выше проверенные версии ядра и библиотеки из папки libraries проекта.
//
// Другая вероятная причина возможных сбоев плеера - (не)стабильность или недостаточное (или завышенное) 
// напряжение питание платы плеера или недостаток тока, предоставляемого БП для питания плеера.
// По Datasheet напряжение питания DFPlayer - 4.2 вольта (с допустимым заявленным диапазоном 3.3В..5В), однако при использовании напряжения 
// 5 вольт или чуть выше - плеер работает нестабильно (зависит от чипа - как повезет).
//
// Общая рекомендация - питать всю систему от напряжения 4.9 вольта при необходимости используя подпирающий диод между GND блока питания (матрицы) 
// и пином GND микроконтроллера / DFPlayer`a
//
// -------------------------------------------------------------------------------------------------------
//
// *** Дополнительный индикатор TM1637
//
// Библиотеку TM1637 следует обязательно устанавливать из папки проекта. В ней сделаны исправления, позволяющие
// компилироваться проекту для микроконтроллера ESP32. Со стандартной версией библиотеки из менеджера библиотек
// проект не будет компилироваться.
//
// --------------------------------------------------------
//
// *** Синхронизация устройств по протоколу E1.31
//
// Библиотеку ESPAsyncE131 следует обязательно устанавливать из папки проекта. В ней исправлены ошибки стандартной
// библиотеки (добавлен деструктор и освобождение выделяемых ресурсов), а также добавлен ряд функций, позволяющих
// осуществлять передачу сформированных пакетов в сеть. Со стандартной версией библиотеки из менеджера библиотек
// проект не будет компилироваться. Вещание ведется в Multicast UDP в локальной сети. 
// В настройках роутера Multicast должен быть разрешен, роутер обладать достаточной пропускной способностью.
//
// При недостаточной пропускной способности роутера вывод трансляции может проходить неравномерно, с рывками.
// Некоторые роутеры (например TPLink Archer C80) имеют кривую реализацию протокола маршрутизации multicast
// трафика, отдают полученные пакеты в сеть неравномерно. При большом входящем трафике роутер может "зависать", тогда
// компьютеры в локальной сети будут писать "нет подключения к интернет". Так же на этом роутере наблюдалось полное зависание сети,
// от 1 до 10 минут или до перезагрузки роутера при внезапном отключении одного из приемников multicat трафика.
//
// --------------------------------------------------------
//
// *** SD-карта
//
// Некоторые SD-shield требуют напряжения питания 5 вольт, некоторые - 3.3 вольта
// Если на SD-shield подать напряжение, не соответствующее его характеристикам - файлы с SD карты также будут не видны.
// При использовании "матрешки" из Wemos d1 mini и соотвествующего ей Shield SD-card рекомендается распаивать ОБА пина питания - и +5В и +3.3В 
//
// Рекомендуемый к использованию шилд SD-карты: https://aliexpress.ru/item/32718621622.html
// Он удобно устанавливается "матрешкой" на платы микроконтроллеров
//   ESP8266 - Wemos d1 mini       - https://aliexpress.ru/item/32630518881.html
//   ESP32   - Wemos d1 mini esp32 - https://aliexpress.ru/item/32858054775.html
//
// --------------------------------------------------------
//
// *** OTA - обновление "по воздуху" - Over The Air - без подключения к USB
//
// Обычно для обновления по воздуху, требуется, чтобы в меню "Инструменты" - "Flash Size" - была выбрана разметка файловой системы 
// с выделением места под OTA:
//   Для микроконтроллеров ESP8266 с 4МБ флэш-памяти рекомендуется вариант "Flash Size: 4MB(FS:2MB OTA:~1019KB)"
//   Для микроконтроллеров ESP32   с 4МБ флэш-памяти рекомендуется вариант "Partition scheme: Default 4MB with spiff(1.2MB APP/1.5MB SPIFFS)"; 
//
// Однако размер прошивки достиг (пред)критического размера, когда при поддержке всех включенных возможностей раздел приложения занимает 99%
// flash-памяти (смотри сообщения компилятора при провверке скетча). В этом случае прошивка может загрузиться на плату, но при старте не запустится
// на выполнение, а уйдет в вечный бесконечный цикл перезагрузки.
//
// Если такое случилось, для запуски прошивки нужно пойти по одному из следующих путей:
// - Взять плату с бОльшим размером установленной флэш-памяти - 8MB или 16Ьии указать соответствующий размер в меню "Инструменты" - выбрать значения, соответствующие вашей плате
// - Отключить поддержку некоторых возможностей, установив в настройках скетча константы USE_XXXX в 0 - поддержка отключена (USE_TP1637, USE_MP3, USE_SD, USE_ANIMATION, INITIALIZE_TEXTS и др.)
// - Отказаться от обновления по воздуху, выбрав в меню "Инструменты" Arduino IDE в настройке распределения памяти устройства выберите вариант: 
//     Для микроконтроллеров ESP8266  - перейти на микроконтроллер ESP32
//     Для микроконтроллеров ESP32    - с 4МБ флэш-памяти рекомендуется вариант "Partition scheme: No OTA (2MB App/2MB SPIFFS)"; 
//   а также установив в настройках скетча константу USE_OTA в значение 0
//
// Инструкция от mikewap83 - увеличение раздела под ОТА и скетч:
// - Открыть файл C:\Users\[ИМЯ пользователя]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\boards.txt
// - Добавить строки в разделе ## IDE 2.0 Seems to not update the value где идет описание PartitionScheme
//     esp32.menu.PartitionScheme.default_1_4MB=4MB with spiffs (1.408MB APP/1.152MB SPIFFS)
//     esp32.menu.PartitionScheme.default_1_4MB.build.partitions=default_1_4MB
//     esp32.menu.PartitionScheme.default_1_4MB.upload.maximum_size=1441792
// - Скопировать файл default_1_4MB.csv в папку C:\Users\[ИМЯ пользователя]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\tools\partitions
// - В Arduino IDE 2.x в меню "Инструменты" не появится новый вариант PartitionScheme, пока мы не удалим скешированную папку приложения, для этого 
//   необходимо удалить папку C:\Users\[ИМЯ пользователя]\AppData\Roaming\arduino-ide\
// - Перезапустить Arduino и выбрать в настройка размер памяти 4M with spiffs (1.408MB APP/1.152MB SPIFFS)
// Сам CSV-файл разметки находится в проекте, в папке Wiki/OTA/mikewap83 - default_1_4MB.zip
//
// После этого получается 1.408MB на скетч, столько же для OTA обновлений и 1.152MB для хранения папки Data. 
// В файловой системе хранятся файлы веб-интерфейса, тексты бегущей строки, файлы сохраненных настроек - backup
// Необязательные файлы - файлы картинок пользователя для рисования и эффекта "Слайды", для матриц небольшого размера - могут краниться файлы эффектов SD-карты
// при эмуляции SD-карты, если она физически отсутствует.
// Если переразмеченного пространста не хватает для скетча и OTA - можно снова изменить разбивку partitions уменьшив раздел под файлы,
// увеличив раздел под скетч и OTA.
//
// -------------------------------------------------------------------------------------------------------
//
// *** Настройка параметров прошивки под вашу конфигурацию оборудования
//
// Для начала работы выполните настройки параметров прошивки - укажите нужные значения в файлах 
//  a_def_hard.h - параметры вашей матрицы, наличие дополнительных модулей, пины подключения, использование возможностей прошивки
//  a_def_soft.h - параметры подулючения к сети - имя сети, пароль, временная зона, коды для получения погоды и т.д.
// Большинство параметров в последствии могут быть изменены в Android-приложении при подключении к устройству или в приложении на смартфоне, 
// если таковое уже есть для текущей версии прошивки.
//
// -------------------------------------------------------------------------------------------------------
//
// ** Загрузка айлов из папки firmware/data в файловую систему микроконтроллера
//
// ** Arduino IDE 1.8.x
//
// В папке проекта tools найдите папку /LittleFS_Uploader/Arduino IDE 1.8.x
// - ESP8266LittleFS-2.6.0.zip - плагин для загрузки данных в файловую систему LittleFS для ESP8266                ( https://github.com/earlephilhower/arduino-esp8266littlefs-plugin )
// - ESP32LittleFS.zip - плагин для загрузки данных в файловую систему LittleFS для ESP32                          ( https://github.com/lorol/arduino-esp32littlefs-plugin            )
// - ESP32fs.zip - универсальный плагин для загрузки данных в файловую систему SPIFFS, LittleFS и FatFS для ESP32  ( https://github.com/lorol/arduino-esp32fs-plugin                  )
//
//   1. Перейдите в папку, где установлена Arduino IDE (обычно "C:\Program Files (x86)\Arduino\")
//   2. Если в папке нет подпапки tools - создайте ее, если есть - перейдите в эту папку
//   3. Распакуйте архив ESP8266LittleFS-2.6.0.zip в папку tools - в ней будет создана подпапка ESP8266LittleFS с файлами плагина
//   4. Перезагрузите Arduino IDE. 
//      Важно!!! Закройте окно монитора порта, если оно было открыто, в противном случае получите ошибку "Порт занят"/"Access denied"
//   5. В Arduino IDE в меню "Инструменты" выберите пункт "ESP8266 LittleFS Data Upload"
//   6. Наблюдайте в ArduinoIDE области сообщений ход загрузки файлов, дождитесь сообщения о завершении загрузке и
//      о перезагрузке микроконтроллеоа
//   7. Загрузка файлов завершена.
//
// ** Arduino IDE 2.3.x
//
// В папке проекта tools найдите папку /LittleFS_Uploader/Arduino IDE 2.3.x
// - arduino-littlefs-upload-1.2.1.vsix - плагин для загрузки данных в файловую систему LittleFS для ESP8266/ESP32  ( https://github.com/earlephilhower/arduino-littlefs-upload       )
//
//   1. Перейдите в папку C:\Users\<username>\.arduinoIDE\plugins\ и скопируйте сюда файл плагина. Если папки нет - создайте ее.
//   2. Запустите Arduino IDE 2.3.x
//      Важно!!! Закройте окно монитора порта, если оно было открыто, в противном случае получите ошибку "Порт занят"/"Access denied"
//   3. Нажмите комбинацию клавиш [Ctrl] + [Shift] + [P]
//   4. Выберите в появившемся меню "Upload LittleFS to Pico/ESP8266/Esp32" или
//      начните мечатать в строке поиска 'Upload' и фильтр поможет найти плагин
//
// *******************************************************************************************************
// *                                              WIFI ПАНЕЛЬ                                            *
// *******************************************************************************************************

#include "a_def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
#include "a_def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

#if (USE_MQTT == 1)

// ------------------ MQTT CALLBACK -------------------

void callback(char* topic, uint8_t* payload, uint32_t length) {
  if (stopMQTT) return;
  // проверяем из нужного ли нам топика пришли данные
  DEBUG("MQTT << topic='" + String(topic) + "'");
  if (strcmp(topic, mqtt_topic(TOPIC_CMD).c_str()) == 0) {
    memset(incomeMqttBuffer, 0, BUF_MQTT_SIZE);
    memcpy(incomeMqttBuffer, payload, length);
    
    DEBUG(F("; cmd='"));
    DEBUG(incomeMqttBuffer);
    DEBUG("'");
    
    // В одном сообщении может быть несколько команд. Каждая команда начинается с '$' и заканчивается ';'/ Пробелы между ';' и '$' НЕ допускаются.
    String command = String(incomeMqttBuffer);    
    command.replace("\n", "~");
    command.replace(";$", "\n");
    uint32_t count = CountTokens(command, '\n');
    
    for (uint8_t i=1; i<=count; i++) {
      String cmd = GetToken(command, i, '\n');
      cmd.replace('~', '\n');
      cmd.trim();
      // После разделения команд во 2 и далее строке '$' (начало команды) удален - восстановить
      if (!cmd.startsWith("$")) {
        cmd = "$" + cmd;
      }
      // После разделения команд во 2 и далее строке ';' (конец команды) удален - восстановить
      // Команда '$6 ' не может быть в пакете и признак ';' (конец команды) не используется - не восстанавливать
      if (!cmd.endsWith(";") && !cmd.startsWith(F("$6 "))) {
        cmd += ";";
      }        
      if (cmd.length() > 0 && queueLength < QSIZE_IN) {
        queueLength++;
        cmdQueue[queueWriteIdx++] = cmd;
        if (queueWriteIdx >= QSIZE_IN) queueWriteIdx = 0;
      }
    }    
  }
  DEBUGLN();
}

#endif

// ---------------------------------------------------------------

void setup() {
  #if defined(ESP8266)
    ESP.wdtEnable(WDTO_8S);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
  #endif

  // Инициализация EEPROM и загрузка начальных значений переменных и параметров
  #if (EEPROM_MAX <= EEPROM_TEXT)
    #pragma message "Не выделено памяти для хранения строк эффекта 'Бегущая строка'"
    EEPROM.begin(EEPROM_TEXT);
  #else  
    EEPROM.begin(EEPROM_MAX);
  #endif

  #if (DEBUG_SERIAL == 1)
    Serial.begin(115200);
    delay(300);
  #endif

  // пинаем генератор случайных чисел
  #if defined(ESP8266) && defined(TRUE_RANDOM)
  uint32_t seed = (int)RANDOM_REG32;
  #else
  uint32_t seed = (int)(analogRead(0) ^ micros());
  #endif
  randomSeed(seed);
  random16_set_seed(seed);

  #ifdef DEVICE_ID
    host_name = String(HOST_NAME) + "-" + String(DEVICE_ID);
  #else
    host_name = String(HOST_NAME);
  #endif

  uint8_t eeprom_id = EEPROMread(0);
  
  DEBUGLN();
  DEBUGLN(FIRMWARE_VER);
  DEBUG(F("Версия EEPROM: "));
  DEBUGLN("0x" + IntToHex(eeprom_id, 2));  
  if (eeprom_id != EEPROM_OK) {
    DEBUG(F("Обновлено до: "));
    DEBUGLN("0x" + IntToHex(EEPROM_OK, 2));  
  }
  DEBUGLN("Host: '" + host_name + "'");
  DEBUGLN();
  
  DEBUG(F("Контроллер: "));
  DEBUGLN(MCUTypeEx());

  {      
    #if defined(ESP32) && defined(ARDUINO_ESP32_RELEASE)
      String core_type = F("ESP32");
      String core_version(ARDUINO_ESP32_RELEASE);
    #elif defined(ESP8266) && defined(ARDUINO_ESP8266_RELEASE)
      String core_type = F("ESP8266");
      String core_version(ARDUINO_ESP8266_RELEASE);
    #endif
    if (core_version.length() > 0) {
      DEBUG(F("Версия ядра: "));
      DEBUG(core_type);
      core_version.replace("_", ".");
      DEBUG(F(" v"));
      DEBUGLN(core_version);
    }
  }
  
  {
    String fv(FASTLED_VERSION);
    String fv_maj(fv.substring(1,4).toInt());
    String fv_min(fv.substring(4).toInt());
    DEBUG(F("FastLED: ")); DEBUG(fv[0]); DEBUG('.');  DEBUG(fv_maj); DEBUG('.'); DEBUG(fv_min);    
    DEBUGLN();
  }

  DEBUGLN(F("Инициализация файловой системы... "));
  
  spiffs_ok = LittleFS.begin();
  if (!spiffs_ok) {
    DEBUGLN(F("Выполняется разметка файловой системы... "));
    LittleFS.format();
    spiffs_ok = LittleFS.begin();    
  }

  if (spiffs_ok) {
    #if defined(ESP32)
      spiffs_total_bytes = LittleFS.totalBytes();
      spiffs_used_bytes  = LittleFS.usedBytes();
      DEBUGLN(String(F("Использовано ")) + String(spiffs_used_bytes) + " из " + String(spiffs_total_bytes) + " байт");
      DEBUGLN();
    #else
      FSInfo fs_info;
      if (LittleFS.info(fs_info)) {
        spiffs_total_bytes = fs_info.totalBytes;
        spiffs_used_bytes  = fs_info.usedBytes;
        DEBUGLN(String(F("Использовано ")) + String(spiffs_used_bytes) + " из " + String(spiffs_total_bytes) + " байт");
        DEBUGLN();
      } else {
        DEBUGLN(F("Ошибка получения сведений о файловой системе."));
        DEBUGLN();
    }
    #endif
  } else {
    DEBUGLN(F("Файловая система недоступна."));
    DEBUGLN();
  }

  loadSettings();

  // -----------------------------------------  
  // Вывод основных возможностей: поддержка в прошивке - 
  // включена или выключена + некоторые параметры
  // -----------------------------------------  

  DEBUGLN();
  DEBUG(F("Матрица: "));
  if (DEVICE_TYPE == 0) DEBUG(F("труба "));
  if (DEVICE_TYPE == 1) DEBUG(F("плоская "));
  DEBUGLN(String(pWIDTH) + "x" + String(pHEIGHT));

  if (sMATRIX_TYPE == 2) {
    DEBUGLN(F("Aдресация: карта индексов"));
  } else {
    DEBUGLN(F("Адресация: по подключению"));
    DEBUG(F("  Угол: ")); 
    if (sCONNECTION_ANGLE == 0) DEBUGLN(F("левый нижний")); else
    if (sCONNECTION_ANGLE == 1) DEBUGLN(F("левый верхний")); else
    if (sCONNECTION_ANGLE == 2) DEBUGLN(F("правый верхний")); else
    if (sCONNECTION_ANGLE == 3) DEBUGLN(F("правый нижний"));
    DEBUG(F("  Направление: "));
    if (sSTRIP_DIRECTION == 0) DEBUGLN(F("вправо")); else
    if (sSTRIP_DIRECTION == 1) DEBUGLN(F("вверх")); else
    if (sSTRIP_DIRECTION == 2) DEBUGLN(F("влево")); else
    if (sSTRIP_DIRECTION == 3) DEBUGLN(F("вниз"));
    DEBUG(F("  Тип: "));
    if (sMATRIX_TYPE == 0) DEBUGLN(F("зигзаг")); else
    if (sMATRIX_TYPE == 1) DEBUGLN(F("параллельная"));
    
    if (mWIDTH > 1 || mHEIGHT > 1) {
      DEBUG(F("Cегменты: "));
      DEBUGLN(String(mWIDTH) + "x" + String(mHEIGHT));
      DEBUG(F("  Угол: ")); 
      if (mANGLE == 0) DEBUGLN(F("левый нижний")); else
      if (mANGLE == 1) DEBUGLN(F("левый верхний")); else
      if (mANGLE == 2) DEBUGLN(F("правый верхний")); else
      if (mANGLE == 3) DEBUGLN(F("правый нижний"));
      DEBUG(F("  Направление: "));
      if (mDIRECTION == 0) DEBUGLN(F("вправо")); else
      if (mDIRECTION == 1) DEBUGLN(F("вверх")); else
      if (mDIRECTION == 2) DEBUGLN(F("влево")); else
      if (mDIRECTION == 3) DEBUGLN(F("вниз"));
      DEBUG(F("  Тип: "));
      if (mTYPE == 0) DEBUGLN(F("зигзаг")); else
      if (mTYPE == 1) DEBUGLN(F("параллельная"));
    }
  }

  DEBUGLN();
  DEBUGLN(F("Доступные возможности:"));
  
  DEBUG(F("+ Бегущая строка: шрифт "));
  if (BIG_FONT == 0)
    DEBUGLN(F("5x8"));
  if (BIG_FONT == 2)
    DEBUGLN(F("8x13"));
  if (BIG_FONT == 1)
    DEBUGLN(F("10x16"));

  DEBUG(F("+ Кнопка управления: "));
  if (BUTTON_TYPE == 0)
    DEBUGLN(F("сенсорная"));
  if (BUTTON_TYPE == 1)
    DEBUGLN(F("тактовая"));

  DEBUGLN(F("+ Синхронизация времени с сервером NTP"));

  DEBUG((USE_POWER == 1 ? "+" : "-"));
  DEBUGLN(F(" Управление питанием"));

  DEBUG((USE_WEATHER == 1 ? "+" : "-"));
  DEBUGLN(F(" Получение информации о погоде"));

  DEBUG((USE_MQTT == 1 ? "+" : "-"));
  DEBUGLN(F(" Управление по каналу MQTT"));

  DEBUG((USE_E131 == 1 ? "+" : "-"));
  DEBUGLN(F(" Групповая синхронизация по протоколу E1.31"));

  DEBUG((USE_TM1637 == 1 ? "+" : "-"));
  DEBUGLN(F(" Дополнительный индикатор TM1637"));

  DEBUG((USE_SD == 1 ? "+" : "-"));
  DEBUG(F(" Эффекты Jinx! с SD-карты"));
  DEBUGLN(USE_SD == 1 && FS_AS_SD == 1 ? String(F(" (эмуляция в FS)")) : "");

  DEBUG((USE_MP3 == 1 ? "+" : "-"));
  DEBUGLN(F(" Поддержка MP3 Player"));

  DEBUGLN();

  if (sMATRIX_TYPE == 2) {
    pWIDTH = mapWIDTH;
    pHEIGHT = mapHEIGHT;
  } else {
    pWIDTH = sWIDTH * mWIDTH;
    pHEIGHT = sHEIGHT * mHEIGHT;
  }

  NUM_LEDS = pWIDTH * pHEIGHT;
  maxDim   = max(pWIDTH, pHEIGHT);
  minDim   = min(pWIDTH, pHEIGHT);

  // -----------------------------------------
  // В этом блоке можно принудительно устанавливать параметры, которые должны быть установлены при старте микроконтроллера
  // -----------------------------------------
  
  // -----------------------------------------  
    
  // Настройки ленты
  leds = new CRGB[NUM_LEDS];          
  overlayLEDs = new CRGB[OVERLAY_SIZE];

  // Создать массив для карты индексов адресации светодиодов в ленте
  bool ok = loadIndexMap();
  if (sMATRIX_TYPE == 2 && (!ok || mapListLen == 0)) {
    sMATRIX_TYPE = 0;
    putMatrixSegmentType(sMATRIX_TYPE);
  }

  FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );

  /*
  // Это пример, как выводить на две матрицы 16х16 (сборная матрица 32х16) через два пина D2 и D3
  // Чтобы вывод был именно на D2 и D3 - в меню Инструменты - плату выбирать "Wemos D1 mini pro" - при выбранной плате NodeMCU назначение пинов куда-то "съезжает" на другие - нужно искать куда. 
  // Убедитесь в правильном назначении адресации диодов матриц в сборной матрице используя индексные файлы или сьорную матрицу из матриц одного размера и подключения сегментов.
  FastLED.addLeds<WS2812, D2, COLOR_ORDER>(leds, 256).setCorrection( TypicalLEDStrip );
  FastLED.addLeds<WS2812, D3, COLOR_ORDER>(leds, 256, 256).setCorrection( TypicalLEDStrip );
  */
  
  FastLEDsetBrightness(globalBrightness);
  if (CURRENT_LIMIT > 0) {
    FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  }
  FastLED.clear();
  FastLED.show();

  // Инициализация SD-карты
  #if (USE_SD == 1)
    InitializeSD1();
  #endif

  // Проверить наличие резервной копии настроек EEPROM в файловой системе MK и/или на SD-карте
  eeprom_backup = checkEepromBackup();
  if ((eeprom_backup & 0x01) > 0) {
    DEBUGLN(F("Найдены сохраненные настройки: FS://eeprom.bin"));
  }
  if ((eeprom_backup & 0x02) > 0) {
    DEBUGLN(F("Найдены сохраненные настройки: SD://eeprom.bin"));
  }
    
  // Инициализация SD-карты
  #if (USE_SD == 1)
    InitializeSD2();
  #endif

  // Поиск доступных анимаций
  initAnimations();

  // Поиск картинок, пригодных для эффекта "Слайды"
  initialisePictures();
  
  #if (USE_POWER == 1)
    pinMode(POWER_PIN, OUTPUT);
  #endif
     
  // Первый этап инициализации плеера - подключение и основные настройки
  #if (USE_MP3 == 1)
    InitializeDfPlayer1();
  #endif

  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
  #endif

  // Настройка кнопки
  butt.setStepTimeout(100);
  butt.setClickTimeout(500);
  butt.setDebounce(50);
  butt.tick();
  butt.isHolded();

  // Второй этап инициализации плеера - проверка наличия файлов звуков на SD карте
  #if (USE_MP3 == 1)
    if (isDfPlayerOk) {
      InitializeDfPlayer2();
      if (!isDfPlayerOk) {
        DEBUGLN(F("MP3 плеер недоступен."));
      }
    } else
        DEBUGLN(F("MP3 плеер недоступен."));
  #endif

  // Подключение к сети
  connectToNetwork();

  #if (USE_E131 == 1)
    InitializeE131();
  #endif
  
  #if (USE_MQTT == 1)
  // Настройка соединения с MQTT сервером
  stopMQTT = !useMQTT;
  changed_keys = "";
  mqtt_client_name = host_name + "-" + String(random16(), HEX);
  last_mqtt_server = mqtt_server;
  last_mqtt_port = mqtt_port;
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);
  mqtt.setSocketTimeout(1);
  uint32_t t = millis();
  checkMqttConnection();
  if (millis() - t > MQTT_CONNECT_TIMEOUT) {
    nextMqttConnectTime = millis() + MQTT_RECONNECT_PERIOD;
  }
  String msg = F("START");
  SendMQTT(msg, TOPIC_STA);
  #endif

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
 
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(host_name.c_str());
 
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = F("скетча...");
    else // U_SPIFFS
      type = F("файловой системы SPIFFS...");
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    DEBUG(F("Начато обновление "));    
    DEBUGLN(type);    
  });

  ArduinoOTA.onEnd([]() {
    DEBUGLN(F("\nОбновление завершено"));
  });

  ArduinoOTA.onProgress([](uint32_t progress, uint32_t total) {
    #if (DEBUG_SERIAL == 1)
    Serial.printf("Progress: %u%%\r", (uint32_t)(progress / (total / 100)));
    #endif
  });

  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG(F("Ошибка: "));
    DEBUGLN(error);
    if      (error == OTA_AUTH_ERROR)    DEBUGLN(F("Неверное имя/пароль сети"));
    else if (error == OTA_BEGIN_ERROR)   DEBUGLN(F("Не удалось запустить обновление"));
    else if (error == OTA_CONNECT_ERROR) DEBUGLN(F("Не удалось установить соединение"));
    else if (error == OTA_RECEIVE_ERROR) DEBUGLN(F("Не удалось получить данные"));
    else if (error == OTA_END_ERROR)     DEBUGLN(F("Ошибка завершения сессии"));
  });

  ArduinoOTA.begin();

  // UDP-клиент на указанном порту
  udp.begin(localPort);

  // Настройка внешнего дисплея TM1637
  #if (USE_TM1637 == 1)
    currDisplayBrightness = 7;
    currDisplay[0] = _empty;
    currDisplay[1] = _empty;
    currDisplay[2] = _empty;
    currDisplay[3] = _empty;
  #endif

  // Таймер синхронизации часов
  ntpSyncTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);

  #if (USE_WEATHER == 1)     
  // Таймер получения погоды
  weatherTimer.setInterval(1000 * 60 * SYNC_WEATHER_PERIOD);
  #endif

  // Таймер рассвета
  dawnTimer.stopTimer();
  
  // Проверить соответствие позиции вывода часов размерам матрицы
  // При необходимости параметры отображения часов корректируются в соответствии с текущими аппаратными возможностями
  checkClockOrigin();
  
  // Если был задан спец.режим во время предыдущего сеанса работы матрицы - включить его
  // Номер спец-режима запоминается при его включении и сбрасывается при включении обычного режима или игры
  // Это позволяет в случае внезапной перезагрузки матрицы (например по wdt), когда был включен спец-режим (например ночные часы или выкл. лампы)
  // снова включить его, а не отображать случайный обычный после включения матрицы
  int8_t spc_mode = getCurrentSpecMode();

  if (spc_mode >= 0 && spc_mode < MAX_SPEC_EFFECT) {
    setSpecialMode(spc_mode);
    set_isTurnedOff(spc_mode == 0);
    set_isNightClock(spc_mode == 8);
  } else {
    set_thisMode(getCurrentManualMode());
    if (thisMode < 0 || thisMode == MC_TEXT || thisMode >= SPECIAL_EFFECTS_START) {
      setRandomMode();
    } else {
      setEffect(thisMode);        
    }
  }
  autoplayTimer = millis();

  #if (USE_MQTT == 1)
  if (!stopMQTT) mqttSendStartState();
  #endif

  setIdleTimer();
}

void loop() {
  if (wifi_connected) {
    ArduinoOTA.handle();
    #if (USE_MQTT == 1)
      if (!stopMQTT) {
        checkMqttConnection();
        mqtt.loop();
      }
    #endif
  }
  
  process();
}

// -----------------------------------------

void startWiFi(uint32_t waitTime) { 
  
  WiFi.disconnect(true);
  set_wifi_connected(false);
  
  delay(10);               // Иначе получаем Core 1 panic'ed (Cache disabled but cached memory region accessed)
  WiFi.mode(WIFI_STA);
 
  // Пытаемся соединиться с роутером в сети
  if (ssid.length() > 0) {
    DEBUG(F("\nПодключение к "));
    DEBUG(ssid);

    if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0) {
      WiFi.config(IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]),  // 192.168.0.106
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], GTW),        // 192.168.0.1
                  IPAddress(255, 255, 255, 0),                            // Mask
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], GTW),        // DNS1 192.168.0.1
                  IPAddress(8, 8, 8, 8));                                 // DNS2 8.8.8.8                  
      DEBUG(F(" -> "));
      DEBUG(IP_STA[0]);
      DEBUG(".");
      DEBUG(IP_STA[1]);
      DEBUG(".");
      DEBUG(IP_STA[2]);
      DEBUG(".");
      DEBUG(IP_STA[3]);                  
    }              

    WiFi.begin(ssid.c_str(), pass.c_str());
  
    // Проверка соединения (таймаут 180 секунд, прерывается при необходимости нажатием кнопки)
    // Такой таймаут нужен в случае, когда отключают электричество, при последующем включении устройство стартует быстрее
    // чем роутер успеет загрузиться и создать сеть. При коротком таймауте устройство не найдет сеть и создаст точку доступа,
    // не сможет получить время, погоду и т.д.
    bool     stop_waiting = false;
    uint32_t start_wifi_check = millis();
    uint32_t last_wifi_check = 0;
    int16_t  cnt = 0;
    while (!(stop_waiting || wifi_connected)) {
      delay(1);
      if (millis() - last_wifi_check > 250) {
        last_wifi_check = millis();
        set_wifi_connected(WiFi.status() == WL_CONNECTED); 
        if (wifi_connected) {
          // Подключение установлено
          DEBUGLN();
          DEBUG(F("WiFi подключен. IP адрес: "));
          DEBUGLN(WiFi.localIP());
          break;
        }
        if (cnt % 50 == 0) {
          DEBUGLN();
        }
        DEBUG(".");
        cnt++;
      }
      if (millis() - start_wifi_check > waitTime) {
        // Время ожидания подключения к сети вышло
        break;
      }
      delay(1);
      // Опрос состояния кнопки
      butt.tick();
      if (butt.hasClicks()) {
        butt.getClicks();
        DEBUGLN();
        DEBUGLN(F("Нажата кнопка.\nОжидание подключения к сети WiFi прервано."));  
        stop_waiting = true;
        break;
      }
      delay(1);
    }
    DEBUGLN();

    if (!wifi_connected && !stop_waiting)
      DEBUGLN(F("Не удалось подключиться к сети WiFi."));
  }  
}

void startSoftAP() {
  WiFi.softAPdisconnect(true);
  ap_connected = false;

  DEBUG(F("Создание точки доступа "));
  DEBUG(apName);
  
  // IP адрес который назначается точке доступа
  IPAddress ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(ip, gateway, subnet);
  ap_connected = WiFi.softAP(apName, apPass);

  for (uint8_t j = 0; j < 10; j++ ) {

    delay(0);
    if (ap_connected) {
      DEBUGLN();
      DEBUG(F("Точка доступа создана. Сеть: '"));
      DEBUG(apName);
      // Если пароль совпадает с паролем по умолчанию - печатать для информации,
      // если был изменен пользователем - не печатать
      if (strcmp(apPass, "12341234") == 0) {
        DEBUG(F("'. Пароль: '"));
        DEBUG(apPass);
      }
      DEBUGLN(F("'."));
      DEBUG(F("IP адрес: "));
      DEBUGLN(WiFi.softAPIP());
      break;
    }    
    
    WiFi.enableAP(false);
    WiFi.softAPdisconnect(true);
    delay(500);
    
    DEBUG(".");
    ap_connected = WiFi.softAP(apName, apPass);
    
  }  
  DEBUGLN();  

  if (!ap_connected) 
    DEBUGLN(F("Не удалось создать WiFi точку доступа."));
}

void connectToNetwork() {
  // Подключиться к WiFi сети, ожидать подключения 180 сек пока, например, после отключения электричества роутер загрузится и поднимет сеть
  startWiFi(180000);

  // Если режим точки доступа не используется и к WiFi сети подключиться не удалось - создать точку доступа
  if (!wifi_connected){
    WiFi.mode(WIFI_AP);
    startSoftAP();
  }

  if (useSoftAP && !ap_connected) startSoftAP();    

  // Сообщить UDP порт, на который ожидаются подключения
  if (wifi_connected || ap_connected) {
    DEBUG(F("UDP-сервер на порту "));
    DEBUGLN(localPort);
  }
}
