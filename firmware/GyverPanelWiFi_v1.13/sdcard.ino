#if (USE_SD == 1)

File fxdata;
File folder;

int8_t  file_idx;    // Служебное - для определения какой следующий файл воспроизводить
String  fileName;    // Полное имя файла эффекта, включая имя папки
bool    sd_card_ok = false;

void InitializeSD1() {  
  set_isSdCardReady(false);
  DEBUGLN(F("\nИнициализация SD-карты..."));  
  sd_card_ok = SD.begin(SD_CS_PIN);
}

void InitializeSD2() {  
  if (sd_card_ok) {
    DEBUGLN(F("Загрузка списка файлов с эффектами..."));  
    loadDirectory();
    sd_card_ok = countFiles > 0;
    set_isSdCardReady(sd_card_ok);
  }
  if (!sd_card_ok) {
    DEBUGLN(F("Эффекты Jinx! на SD-карте не обнаружены"));
  }
}

void loadDirectory() {

  String directoryName = "/" + String(pWIDTH) + "x" + String(pHEIGHT);
  DEBUG(F("Папка с эффектами "));
  DEBUG(directoryName);
  if (SD.exists(directoryName)) {
    DEBUGLN(F(" обнаружена."));
  } else {
    DEBUGLN(F(" не обнаружена."));
    return;
  }

  String   file_name, fn;
  uint32_t file_size;
  float    fsize;
  uint8_t  sz = 0;
  String   fs_name;
  
  bool     first = true;
  folder = SD.open(directoryName);
    
  while (folder) {
    File entry =  folder.openNextFile();
    
    // Очередной файл найден? Нет - завершить
    if (!entry) break;
        
    if (!entry.isDirectory()) {
            
      file_name = entry.name();
      file_size = entry.size();

      fn = file_name;
      fn.toLowerCase();
      
      if (!fn.endsWith(".out") || file_size == 0) {
        entry.close();
        continue;    
      }

      if (countFiles >= MAX_FILES) {
        DEBUG(F("Максимальное количество эффектов: "));        
        DEBUGLN(MAX_FILES);
        entry.close();
        break;
      }
        
      if (first) {
        first = false;
        DEBUGLN(F("Найдены файлы эффектов:"));        
      }

      sz = 0;
      fsize = file_size;
      fs_name = F("байт");

      if (fsize > 1024) { fsize /= 1024.0; fs_name = "К"; sz++;}
      if (fsize > 1024) { fsize /= 1024.0; fs_name = "М"; sz++;}
      if (fsize > 1024) { fsize /= 1024.0; fs_name = "Г"; sz++;}

      // Если полученное имя файла содержит имя папки (на ESP32 это так, на ESP8266 - только имя файла) - оставить только имя файла
      int16_t p = file_name.lastIndexOf("/");
      if (p >= 0) file_name = file_name.substring(p + 1);
      p = file_name.lastIndexOf(".");
      if (p >= 0) file_name = file_name.substring(0, p);
            
      DEBUG("   ");
      DEBUG(file_name);
      DEBUG("\t\t");
      if (sz == 0)
        DEBUGR(file_size, DEC);
      else
        DEBUGR(fsize, 2);      
      DEBUGLN(" " + fs_name);
      
      nameFiles[countFiles++] = file_name;
    }
    
    entry.close();
  }

  if (countFiles == 0) {
    DEBUGLN(F("Доступных файлов эффектов не найдено"));
  }  
}

void sdcardRoutine() {
 
 if (loadingFlag || play_file_finished) {
   //modeCode = MC_SDCARD;

   // Если карта не готова (нт файлов эффектов) - перейти к следующему режиму
   if (!isSdCardReady) {
     if (fxdata) fxdata.close();
     nextMode();
     return;
   }

   // Выбор другого файла - только если установлен loadingFlag
   // Если сюда попали по play_file_finished - блок if (loading) не выполняется - file_idx остается прежним - просто вернуться к началу проигранного файла и воспроизвести его еще раз.
   // Это позволит длительное время "играть" эффект использую зацикленные короткие фрагменты 
   // effectScaleParam2[MC_SDCARD]: 0 - случайный файл; 1 - последовательный перебор; 2 и далее - привести к индексы в массиве nameFiles
   
   if (loadingFlag) {

     loadingFlag = false;
     int8_t currentFile = -1;
     // Указан специальный эффект для бегущей строки? - брать его 
     if (specialTextEffectParam >= 0)
       currentFile = specialTextEffectParam - 1;
     // Указано случайное воспроизведение файлов с карты?
     else if (getEffectScaleParamValue2(MC_SDCARD) == 0)
       currentFile = -2;                                       // Случайный порядок
     else if (getEffectScaleParamValue2(MC_SDCARD) == 1)
       currentFile = -1;                                       // Последовательное воспроизведение
     else
       currentFile = getEffectScaleParamValue2(MC_SDCARD) - 2; // Указанный выбранный файл эффектов

     if (currentFile < 0 || currentFile >= countFiles) {
        if (countFiles == 1) {
          file_idx = 0;
        } else if (countFiles == 2) {
          file_idx = (file_idx != 1) ? 0 : 1;
        } else if (currentFile == -1) {  
          // Последовательный перебор файлов с SD-карты
          if (sf_file_idx < 0) sf_file_idx = 0;
          if (sf_file_idx >= countFiles) sf_file_idx = countFiles - 1;
          file_idx = sf_file_idx;
        } else {
          // Случайный с SD-карты
          file_idx = random16(0,countFiles);
          if (file_idx >= countFiles) file_idx = countFiles - 1;
        }
      } else {
        file_idx = currentFile;
      }
    }

    // При загрузке имен файлов с SD-карты в nameFiles только имя файла внутри выбранной папки -- чтобы получить полное имя файла для загрузки  нужно к имени файла добавить имя папки
    fileName = "/" + String(pWIDTH) + "x" + String(pHEIGHT) + "/" + nameFiles[file_idx] + ".out";

    play_file_finished = false;
    DEBUG(F("Загрузка файла эффекта: '"));
    DEBUG(fileName);

    bool error = false;
    String out;

    if (fxdata) fxdata.close();
    
    fxdata = SD.open(fileName);
    if (fxdata) {
      DEBUGLN(F("' -> ok"));
    } else {
      DEBUGLN(F("' -> ошибка"));
      error = true;
    }

    #if (USE_MQTT == 1)
      DynamicJsonDocument doc(256);
      doc["act"] = F("SDCARD");
      if (error) {
        doc["result"] = F("ERROR");  
      } else {
        doc["result"] = F("OK");
      }
      doc["file"] = fileName;
      serializeJson(doc, out);    
      SendMQTT(out, TOPIC_SDC);
    #endif

    FastLED.clear();
  }  

  // Карта присутствует и файл открылся правильно?
  // Что-то пошло не так - перейти к следующему эффекту
  if (!(isSdCardReady && fxdata)) {
    if (fxdata) fxdata.close();
    nextMode();
    return;    
  }
      
  if (fxdata.available()) {
    char tmp;
    fxdata.readBytes(&tmp, 1); // ??? какой-то байт в начале последовательности - отметка начала кадра / номер канала кадрового потока, передаваемого на устройство ???
    char* ptr = reinterpret_cast<char*>(&leds[0]);
    int16_t cnt = fxdata.readBytes(ptr, NUM_LEDS * 3); // 3 байта на цвет RGB
    play_file_finished = (cnt != NUM_LEDS * 3);    
  } else {
    play_file_finished = true;
  }

  if (play_file_finished) {
    DEBUGLN("'" + fileName + String(F("' - завершено")));
    fxdata.close();
    /*
    if (currentFile >= 0) {
      currentFile++; 
      if (currentFile > countFiles - 1) {
        currentFile = 0;
      }
    }
    */
  }
}

void sdcardRoutineRelease() {
  if (fxdata) fxdata.close();
}
#endif
