#if (USE_SD == 1)

File fxdata;
File folder;

int8_t  file_idx;    // Служебное - для определения какой следующий файл воспроизводить
String  fileName;    // Полное имя файла эффекта, включая имя папки

void InitializeSD() {  
  isSdCardReady = false;
  Serial.println(F("\nИнициализация SD-карты..."));  
  if (SD.begin(SD_CS_PIN)) {
    Serial.println(F("Загрузка списка файлов с эффектами..."));  
    loadDirectory();
    isSdCardReady = countFiles > 0;  
  } else {
    Serial.println(F("SD-карта недоступна"));
  }
}

void loadDirectory() {

  String directoryName = "/" + String(WIDTH) + "x" + String(HEIGHT);
  Serial.print(F("Папка с эффектами "));
  Serial.print(directoryName);
  if (SD.exists(directoryName)) {
    Serial.println(F(" обнаружена."));
  } else {
    Serial.println(F(" не обнаружена."));
    return;
  }

  String file_name;
  uint32_t file_size;
  float fsize;
  byte sz = 0;
  String fs_name;
  
  boolean first = true;
  folder = SD.open(directoryName);
    
  while (folder) {
    File entry =  folder.openNextFile();
    
    // Очередной файл найден? Нет - завершить
    if (!entry) break;
        
    if (!entry.isDirectory()) {
            
      file_name = entry.name();
      file_size = entry.size();
      
      if (!file_name.endsWith(".out") || file_size == 0) {
        entry.close();
        continue;    
      }

      if (countFiles >= MAX_FILES) {
        Serial.print(F("Максимальное количество эффектов: "));        
        Serial.println(MAX_FILES);
        entry.close();
        break;
      }
        
      if (first) {
        first = false;
        Serial.println(F("Найдены файлы эффектов:"));        
      }

      sz = 0;
      fsize = file_size;
      fs_name = F("байт");

      if (fsize > 1024) { fsize /= 1024.0; fs_name = "К"; sz++;}
      if (fsize > 1024) { fsize /= 1024.0; fs_name = "М"; sz++;}
      if (fsize > 1024) { fsize /= 1024.0; fs_name = "Г"; sz++;}
            
      Serial.print("  ");
      Serial.print(file_name);
      Serial.print("\t");
      if (sz == 0)
        Serial.print(file_size, DEC);
      else
        Serial.print(fsize, 2);      
      Serial.println(" " + fs_name);
      
      nameFiles[countFiles++] = file_name;
    }
    
    entry.close();
  }

  if (countFiles == 0) {
    Serial.println(F("Доступных файлов эффектов не найдено"));
  }  
}

void sdcardRoutine() {
  
 if (loadingFlag || play_file_finished) {
   //modeCode = MC_SDCARD;

   // Если карта не готова (нт файлов эффектов) - перейти к следующему режиму
   if (!isSdCardReady) {
     nextMode();
     return;
   }

   // Выбор другого файла - только если установлен loadingFlag
   // Если сюда попали по play_file_finished - просто вернуться к началу проигранного файла и воспроизвести его еще раз.
   // Это позволит длительное время "играть" эффект использую зацикленные короткие фрагменты 
   if (loadingFlag) {
     int8_t currentFile = (specialTextEffectParam >= 0) ? (specialTextEffectParam - 1) : (effectScaleParam2[MC_SDCARD] - 1);
        
     if (currentFile < 0 || currentFile >= countFiles) {
        if (countFiles == 1) {
          file_idx = 0;
        } else if (countFiles == 2) {
          file_idx = (file_idx != 1) ? 0 : 1;
        } else {
          file_idx = random16(0,countFiles);
        }
      } else {
        file_idx = currentFile;
      }
    }
    
    fileName = "/" + String(WIDTH) + "x" + String(HEIGHT) + "/" + nameFiles[file_idx];
    play_file_finished = false;
    Serial.print(F("Загрузка файла эффекта: '"));
    Serial.print(fileName);

    fxdata = SD.open(fileName);
    if (fxdata) {
      Serial.println(F("' -> ok"));
    } else {
      Serial.println(F("' -> ошибка"));
    }

    FastLED.clear();
    loadingFlag = false;
  }  

  // Карта присутствует и файл открылся правильно?
  // Что-то пошло не так - перейти к следующему эффекту
  if (!(isSdCardReady && fxdata)) {
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
    fxdata.close();
  }

  if (play_file_finished) {
    Serial.println("'" + fileName + String(F("' - завершено")));
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

#endif
