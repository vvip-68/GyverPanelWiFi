// Задержка между отправкой последовательных команд в модуль DFPlayer
// На некоторых платах достаточно 10 мс задержки, однако встречаются экземпляры DFPlayer, 
// которые не выполняют команды, если между отправкой аоследовательных команд задержка менее 100 мс.
// Рекомендуется подбирать опытным путем. Слишком большая задержка может давать суммарно до 0.5 сек замирания
// эффектов при начале/окончании воспроизведения звука.
//
// Проявление: если прошивка распознаёт подключенный DFPlayer и в InitializeDfPlayer2() файлы звуков считываются, 
// однако отправка команды "играть" из приложения, страница настройки будильника, комбобокс выбора зввука  
// не начинает воспроизведение звука - увеличьте значение задержки.

#define GUARD_DELAY 75

void InitializeDfPlayer1() {
#if (USE_MP3 == 1)
  mp3Serial.begin(9600, SWSERIAL_8N1, SRX, STX);
  dfPlayer.begin(mp3Serial, false, true);
  dfPlayer.setTimeOut(1000);
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.volume(1);
#endif  
}

void InitializeDfPlayer2() {    
#if (USE_MP3 == 1)
  DEBUG(F("\nИнициализация MP3 плеера."));
  refreshDfPlayerFiles();    
  DEBUGLN(String(F("Звуков будильника найдено: ")) + String(alarmSoundsCount));
  DEBUGLN(String(F("Звуков рассвета найдено: ")) + String(dawnSoundsCount));
  DEBUGLN(String(F("Звуков сообщений найдено: ")) + String(noteSoundsCount));
  set_isDfPlayerOk(alarmSoundsCount + dawnSoundsCount + noteSoundsCount > 0);
#else  
  set_isDfPlayerOk(false);
#endif  
}

#if (USE_MP3 == 1)
void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      //DEBUGLN(F("Таймаут!"));
      break;
    case WrongStack:
      //DEBUGLN(F("Ошибка стека!"));
      break;
    case DFPlayerCardInserted:
      DEBUGLN(F("Карта вставлена."));
      break;
    case DFPlayerCardRemoved:
      DEBUGLN(F("Карта удалена."));
      break;
    case DFPlayerCardOnline:
      DEBUGLN(F("Карта готова."));
      break;
    case DFPlayerUSBInserted:
      DEBUGLN(F("Подключен USB."));
      break;
    case DFPlayerUSBRemoved:
      DEBUGLN(F("Отключен USB."));
      break;
    case DFPlayerPlayFinished:
      DEBUG(F("Номер: "));
      DEBUG(value);
      DEBUGLN(F(". Завершено."));
      if (!(isAlarming || isPlayAlarmSound) && soundFolder == 0 && soundFile == 0 && runTextSound <= 0) {
        dfPlayer.stop();
      }
      break;
    case DFPlayerError:
      DEBUG(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          DEBUGLN(F("Нет карты"));
          break;
        case Sleeping:
          DEBUGLN(F("Ожидание..."));
          break;
        case SerialWrongStack:
          DEBUGLN(F("Неверные данные"));
          break;
        case CheckSumNotMatch:
          DEBUGLN(F("Ошибка контрольной суммы"));
          break;
        case FileIndexOut:
          DEBUGLN(F("Неверный индекс файла"));
          break;
        case FileMismatch:
          DEBUGLN(F("Файл не найден"));
          break;
        case Advertise:
          DEBUGLN(F("Реклама"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void refreshDfPlayerFiles() {
  // Чтение почему-то не всегда работает, иногда возвращает 0 или число от какого-то предыдущего запроса
  // Для того, чтобы наверняка считать значение - первое прочитанное игнорируем, потом читаем несколько раз до повторения.

  // Папка с файлами для будильника
  int cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.readFileCountsInFolder(1);     delay(GUARD_DELAY);
    new_val = dfPlayer.readFileCountsInFolder(1); delay(GUARD_DELAY);    
    if (val == new_val && val != 0) break;
    cnt++;
    delay(100);
    DEBUG(F("."));
  } while ((val == 0 || new_val == 0 || val != new_val) && cnt < 5);
  alarmSoundsCount = val < 0 ? 0 : val;
  
  // Папка с файлами для рассвета
  cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.readFileCountsInFolder(2);     delay(GUARD_DELAY);
    new_val = dfPlayer.readFileCountsInFolder(2); delay(GUARD_DELAY);     
    if (val == new_val && val != 0) break;
    cnt++;
    delay(100);
    DEBUG(F("."));
  } while ((val == 0 || new_val == 0 || val != new_val) && cnt < 5);    
  dawnSoundsCount = val < 0 ? 0 : val;

  // Папка с файлами для звуков в бегущей строке
  cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.readFileCountsInFolder(3);     delay(GUARD_DELAY);
    new_val = dfPlayer.readFileCountsInFolder(3); delay(GUARD_DELAY);     
    if (val == new_val && val != 0) break;
    cnt++;
    delay(100);
    DEBUG(F("."));
  } while ((val == 0 || new_val == 0 || val != new_val) && cnt < 5);    
  noteSoundsCount = val < 0 ? 0 : val;

  DEBUGLN();  
}
#endif

void PlayAlarmSound() {
  
  if (!isDfPlayerOk) return;

  #if (USE_MP3 == 1)  
  int8_t sound = alarmSound;
  // Звук будильника - случайный?
  if (sound == 0) {
    sound = random8(1, alarmSoundsCount);     // -1 - нет звука; 0 - случайный; 1..alarmSoundsCount - звук
  }
  // Установлен корректный звук?
  if (sound > 0) {
    dfPlayer.stop();
    delay(GUARD_DELAY);                              // Без этих задержек между вызовами функция dfPlayer приложение крашится.
    dfPlayer.volume(constrain(maxAlarmVolume,1,30));
    delay(GUARD_DELAY);
    dfPlayer.playFolder(1, sound);
    delay(GUARD_DELAY);
    dfPlayer.enableLoop();
    delay(GUARD_DELAY);    
    alarmSoundTimer.setInterval(alarmDuration * 60L * 1000L);
    alarmSoundTimer.reset();
    set_isPlayAlarmSound(true);
  } else {
    // Звука будильника нет - плавно выключить звук рассвета
    StopSound(1000);
  }
  #endif
}

void PlayDawnSound() {
  if (!isDfPlayerOk) return;

  #if (USE_MP3 == 1)
  // Звук рассвета отключен?
  int8_t sound = dawnSound;
  // Звук рассвета - случайный?
  if (sound == 0) {
    sound = random8(1, dawnSoundsCount);     // -1 - нет звука; 0 - случайный; 1..alarmSoundsCount - звук
  }
  // Установлен корректный звук?
  if (sound > 0) {
    dfPlayer.stop();
    delay(GUARD_DELAY);                             // Без этих задержек между вызовами функция dfPlayer приложение крашится.
    dfPlayer.volume(1);
    delay(GUARD_DELAY);
    dfPlayer.playFolder(2, sound);
    delay(GUARD_DELAY);
    dfPlayer.enableLoop();
    delay(GUARD_DELAY);
    // Установить время приращения громкости звука - от 1 до maxAlarmVolume за время продолжительности рассвета realDawnDuration
    fadeSoundDirection = 1;   
    fadeSoundStepCounter = maxAlarmVolume;
    if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = 1;
    fadeSoundTimer.setInterval(realDawnDuration * 60L * 1000L / fadeSoundStepCounter);
    alarmSoundTimer.setInterval(4294967295);
  } else {
    StopSound(1000);
  }
  #endif
}

void StopSound(int duration) {

  if (!isDfPlayerOk) return;

  #if (USE_MP3 == 1)
  
  set_isPlayAlarmSound(false);

  if (duration <= 0) {
    dfPlayer.stop();
    delay(GUARD_DELAY);
    dfPlayer.volume(0);
    return;
  }
  
  // Чтение текущего уровня звука часто глючит и возвращает 0. Тогда использовать maxAlarmVolume
  fadeSoundStepCounter = dfPlayer.readVolume();
  if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = maxAlarmVolume;
  if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = 1;
    
  fadeSoundDirection = -1;     
  fadeSoundTimer.setInterval(duration / fadeSoundStepCounter);  
  
  #endif
}
