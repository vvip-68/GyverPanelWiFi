
#if (USE_WEATHER == 1)     

bool getWeather() {
  
  if (!wifi_connected) return false;  
  if (!client.connect("yandex.com",443)) return false;                    // Устанавливаем соединение с указанным хостом (Порт 443 для https)

  Serial.println();
  Serial.println(F("Запрос текущей погоды"));
  
  // Отправляем запрос
  client.println(String(F("GET /time/sync.json?geo=")) + String(regionID) + String(F(" HTTP/1.1\r\nHost: yandex.com\r\n\r\n"))); 

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

  String regId = String(regionID);
  sunriseTime  = doc["clocks"][regId]["sunrise"].as<String>();          // Достаём время восхода - Третий уровень вложенности пары ключ/значение clocks -> значение RegionID -> sunrise 
  sunsetTime   = doc["clocks"][regId]["sunset"].as<String>();           // Достаём время заката - Третий уровень вложенности пары ключ/значение clocks -> значение RegionID -> sunset
  temperature  = doc["clocks"][regId]["weather"]["temp"].as<int8_t>();  // Достаём время заката - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> temp
  skyColor     = doc["clocks"][regId]["skyColor"].as<String>();         // Рекомендованный цвет фона
  isNight      = doc["clocks"][regId]["isNight"].as<boolean>();
  icon         = doc["clocks"][regId]["weather"]["icon"].as<String>();  // Достаём иконку - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> icon
  
  // #57bbfe
  if (skyColor.length() != 7) {
    Serial.print(F("JSON не содержит данных о погоде"));
    return false;
  }

  String icon_orig = icon;

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
  Serial.println(String(F("Код иконки: '")) + icon_orig + "'");
  Serial.println(dayTime);
  Serial.println(String(F("Цвет неба: '")) + skyColor + "'");
  
  return true;
}

/*
Код расшифровки иконки. Возможные значения:
  bkn-minus-ra-d — облачно с прояснениями, небольшой дождь (день)
  bkn-minus-sn-d — облачно с прояснениями, небольшой снег (день)
  bkn-minus-sn-n — облачно с прояснениями, небольшой снег (ночь)
  bkn-d — переменная облачность (день)
  bkn-n — переменная облачность (ночь)
  bkn-ra-d — переменная облачность, дождь (день)
  bkn-ra-n — переменная облачность, дождь (ночь)
  bkn-sn-d — переменная облачность, снег (день)
  bkn-sn-n — переменная облачность, снег (ночь)
  bl — метель
  fg-d — туман
  ovc — облачно
  ovc-minus-ra — облачно, временами дождь
  ovc-minus-sn — облачно, временами снег
  ovc-ra — облачно, дождь
  ovc-sn — облачно, снег
  ovc-ts-ra — облачно, дождь, гроза
  skc-d — ясно (день)
  skc-n — ясно (ночь)
*/

void decodeWeather(){  
  bool hasDay   = icon.endsWith("-d");
  bool hasNight = icon.endsWith("-n");
  
  if (hasDay)
    dayTime = F("Светлое время суток");  // Сейчас день
  else if (hasNight)           
    dayTime = F("Темное время суток");   // Сейчас ночь

  if (hasDay || hasNight) {
    icon = icon.substring(0, icon.length() - 2);
  }

  if      (icon == F("bkn-minus-ra"))  weather = F("облачно с прояснениями, небольшой дождь");
  else if (icon == F("bkn-minus-sn"))  weather = F("облачно с прояснениями, небольшой снег");
  else if (icon == F("bkn"))           weather = F("переменная облачность");
  else if (icon == F("bkn-ra"))        weather = F("переменная облачность, дождь");
  else if (icon == F("bkn-sn"))        weather = F("переменная облачность, снег");
  else if (icon == F("bl"))            weather = F("метель");
  else if (icon == F("fg"))            weather = F("туман");
  else if (icon == F("ovc"))           weather = F("облачно");
  else if (icon == F("ovc-minus-ra"))  weather = F("облачно, временами дождь");
  else if (icon == F("ovc-minus-sn"))  weather = F("облачно, временами снег");
  else if (icon == F("ovc-ra"))        weather = F("облачно, дождь");
  else if (icon == F("ovc-sn"))        weather = F("облачно, снег");
  else if (icon == F("ovc-ts-ra"))     weather = F("облачно, дождь, гроза");
  else if (icon == F("skc"))           weather = F("ясно");  
}

#else

bool getWeather() {
  return false;
}

#endif
