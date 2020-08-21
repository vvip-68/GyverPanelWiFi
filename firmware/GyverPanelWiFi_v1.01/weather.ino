
#if (USE_WEATHER == 1)     

bool getWeather() {
  
  if (!wifi_connected) return false;
  if (!client.connect("yandex.com",443)) return false;        // Устанавливаем соединение с указанным хостом (Порт 443 для https)

  Serial.println(F("Запрос текущей погоды"));
  
  // Отправляем запрос
  client.println(String(F("GET /time/sync.json?geo=")) + regionID + String(F(" HTTP/1.1\r\nHost: yandex.com\r\n\r\n"))); 
    
  // Проверяем статус запроса
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  // It should be "HTTP/1.0 200 OK" or "HTTP/1.1 200 OK"
  if (strcmp(status + 9, "200 OK") != 0) {
    Serial.print(F("Ошибка сервера погоды: "));
    Serial.println(status);
    return false;
  } 

    // Пропускаем заголовки                                                                
  char endOfHeaders[] = "\r\n\r\n";                                       // Системные заголовки ответа сервера отделяются от остального содержимого двойным переводом строки
  if (!client.find(endOfHeaders)) {                                       // Отбрасываем системные заголовки ответа сервера
    Serial.println(F("Нераспознанный ответ сервера погоды"));             // Если ответ сервера не содержит системных заголовков, значит что-то пошло не так
    return false;                                                         // и пора прекращать всё это дело
  }

  const size_t capacity = 750;                                            // Эта константа определяет размер буфера под содержимое JSON (https://arduinojson.org/v5/assistant/)
  DynamicJsonDocument doc(capacity);

  // {"time":1597989853200,"clocks":{"62":{"id":62,"name":"Krasnoyarsk","offset":25200000,"offsetString":"UTC+7:00","showSunriseSunset":true,"sunrise":"05:31","sunset":"20:10","isNight":false,"skyColor":"#57bbfe","weather":{"temp":25,"icon":"bkn-d","link":"https://yandex.ru/pogoda/krasnoyarsk"},"parents":[{"id":11309,"name":"Krasnoyarsk Krai"},{"id":225,"name":"Russia"}]}}}

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);

  if (error) {
    Serial.print(F("JSON не разобран: "));
    Serial.println(error.c_str());
    return false;
  }

  client.stop();

  sunriseTime  = doc["clocks"][regionID]["sunrise"].as<String>();          // Достаём время восхода - Третий уровень вложенности пары ключ/значение clocks -> значение RegionID -> sunrise 
  sunsetTime   = doc["clocks"][regionID]["sunset"].as<String>();           // Достаём время заката - Третий уровень вложенности пары ключ/значение clocks -> значение RegionID -> sunset
  temperature  = doc["clocks"][regionID]["weather"]["temp"].as<int8_t>();  // Достаём время заката - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> temp
  skyColor     = doc["clocks"][regionID]["skyColor"].as<String>();         // Рекомендованный цвет фона
  isNight      = doc["clocks"][regionID]["isNight"].as<boolean>();
  
  strcpy(icon, doc["clocks"][regionID]["weather"]["icon"].as<String>().c_str());  // Достаём иконку - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> icon
  decodeWeather();
  
  weather_time = millis();  // запомнить время получения погоды с сервера
  init_weather = true;      // Флаг - погода получена
  refresh_weather = false;
  weather_t = 0; 
  weather_cnt = 0;

  Serial.println(F("Погода получена:"));
  Serial.print(F("Сейчас: "));
  Serial.print(weather + ", "); 
  if (temperature > 0) Serial.print("+"); 
  if (temperature < 0) Serial.print("-"); 
  Serial.println(String(temperature) + "ºC"); // '˚' '◦' 'º'
  Serial.println(dayTime);
  
  return true;
}

void decodeWeather(){  
  char * out = strtok(icon,"-");        // Выделяем первую часть из строки до символа '-'
  String part = String(out);
  while (out != NULL) {                 // Выделяем последующие части строки в цикле, пока значение out не станет нулевым (пустым)
    if (part == "skc")                // Перебираем в условиях все возможные варианты, зашифрованные в названии иконки
      weather = F("Ясно");
    else if (part == "ovc")
      weather = F("Пасмурно");
    else if (part == "bkn")
      weather = F("Облачно");
    else if (part == "ra")
      weather = F("Дождь'");
    else if (part == "ts")
      weather = F("Гроза");
    else if (part == "sn")
      weather = F("Снег");
    else if (part == "bl")
      weather = F("Метель'");
    else if (part == "fg")
      weather = F("Туман");
    else if (part == "n")
      dayTime = F("Темное время суток");
    else if (String(out) == "d")
      dayTime = F("Светлое время суток");
    
    out = strtok(NULL,"-");              // Выделяем очередную часть
  }
}

#else

bool getWeather() {
  return false;
}

#endif
