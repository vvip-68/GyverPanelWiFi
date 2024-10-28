
#if (USE_WEATHER == 1)     

bool getWeather() { 
  
  // Пока включена отладка позиционирования часов - запросы на температуру не выполнять
  if (debug_hours >= 0 && debug_mins >= 0) return true;
  
  if (!wifi_connected || useWeather == 0) return false;  

  DEBUGLN();
  DEBUGLN(F("Запрос текущей погоды"));

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(384);
  String out;
  doc["act"] = F("WEATHER");
  doc["region"] = useWeather == 1 ? regionID : regionID2;
  #endif

  bool   error = false;

  String payload;
  String status(25);
  String regId(useWeather == 1 ? regionID : regionID2);

  #if defined(ESP32)
    WiFiClientSecure *w_client = new WiFiClientSecure; 
    // Ignore SSL certificate validation
    w_client->setInsecure();
  #else
    //std::unique_ptr<BearSSL::WiFiClientSecure>w_client(new BearSSL::WiFiClientSecure());
    //w_client->setInsecure();
    WiFiClient *w_client = new WiFiClient;
  #endif

  if (w_client) 
  {
    {
      HTTPClient https;

      //String request = "https://yandex.com/time/sync.json?geo=62";
      //String request = "https://api.openweathermap.org/data/2.5/weather?id=1502026&lang=ru&units=metric&appid=6a4ba421859c9f4166697758b68d889b";

      //String request = "http://194.58.103.72/http2https.php?https://yandex.com/time/sync.json?geo=62&lang=ru";
      //String request = "http://api.openweathermap.org/data/2.5/weather?id=1502026&lang=ru&units=metric&appid=6a4ba421859c9f4166697758b68d889b";
      //String request = "http://api.weatherbit.io/v2.0/current?city_id=1502026&key=4b3b38ff98a14fbeb01b6dd5bc409c9b&lang=ru"
      //String request = "http://api.open-meteo.com/v1/forecast?latitude=56.010563&longitude=92.852572&current=temperature_2m,weather_code&daily=sunrise,sunset&timeformat=unixtime&forecast_days=1"

      /*
        Скрипт шлюза http2https от Сотнег. Спасибо, добрый человек!
        <?
          $url = $_SERVER['QUERY_STRING'];
          $url_check1 = 'https://api.weather.yandex.ru/'; // этот домен разрешён

          if (strncasecmp($url, $url_check1, strlen($url_check1)) == 0)
            {
              $ch = curl_init($url);
              curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
              curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
              curl_setopt($ch, CURLOPT_HEADER, false);
              $html = curl_exec($ch);
              curl_close($ch);
              echo $html;
            }
            else echo 'nope';
        ?>
      */

      String request(150);

      #if defined(ESP32)
        // В ESP32 достаточно памяти, чтобы работал https протокол - используем его
        if (useWeather == 1) {
          request  = F("https://yandex.com/time/sync.json?geo="); request += String(regionID); 
          request += F("&lang="); request += WTR_LANG_YA; 
        } else if (useWeather == 2) {    
          request  = F("https://api.openweathermap.org/data/2.5/weather?id="); request += String(regionID2); 
          request += F("&units=metric&lang="); request += WTR_LANG_OWM;
          request += F("&appid="); request += OWM_WEATHER_API_KEY;
        }  
        String protocol = F("HTTPS");
      #else
        // В ESP8266 недостаточно памяти, соединение по https протоколу завершается ошибкой подключения к серверу - используем получение погоды по http
        if (useWeather == 1) {
          // С 28.07.2024 Яндекс перестал отдавать погоду по HTTP, перенаправляет на HTTPS запрос, но мы его обработать не можем :(
          // Добрый человек Сотнег поделился скриптом для шлюза http2https 
          // Добрый человек Zordog, предоставил доступ к своему серверу, на который установил скрипит http2https от Сотнег
          // Погода от Яндекс по http снова работает!
          request  = F("http://194.58.103.72/http2https.php?https://yandex.com/time/sync.json?geo="); request += String(regionID); 
          request += F("&lang="); request += WTR_LANG_YA; 
        } else if (useWeather == 2) {    
          request  = F("http://api.openweathermap.org/data/2.5/weather?id="); request += String(regionID2); 
          request += F("&units=metric&lang="); request += WTR_LANG_OWM;
          request += F("&appid="); request += OWM_WEATHER_API_KEY;
        }  
        String protocol = F("HTTP");
      #endif

      if (https.begin(*w_client, request.c_str())) {
        DEBUGLOG(printf,"[%s] GET...\n", protocol.c_str());
        // start connection and send HTTP header
        int httpCode = https.GET();
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          DEBUGLOG(printf, "[%s] GET... code: %d\n", protocol.c_str(), httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            payload = https.getString();
            // DEBUGLN(payload);
          } else {
            error = true;
            status = F("unexpected answer");    // http.errorToString(httpCode)
            DEBUGLOG(printf, "[%s] GET... failed, error: %s\n", protocol.c_str(), https.errorToString(httpCode).c_str());
          }
        } else {
          error = true;
          status = F("connection error");
          DEBUGLOG(printf, "[%s] Unable to connect, error: %s\n", protocol.c_str(), https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        error = true;
        status = F("connection error");
        DEBUGLOG(printf, "[%s] Unable to connect", protocol);
      }
    }

    w_client->stop();   
    delete w_client;

  } else {
    status = F("connection error");
    error = true;
  }

  if (error) {
    DEBUG(F("Ошибка сервера погоды: "));
    DEBUGLN(status);
    
    #if (USE_MQTT == 1)
      doc["result"] = F("ERROR");
      doc["status"] = status;
      serializeJson(doc, out);      
      SendMQTT(out, TOPIC_WTR);
    #endif    

    return false;
  } 

  // Нам не нужно доставать все данные из ответа. Задаем фильтр - это существенно уменьшит размер требуемой памяти 
  DynamicJsonDocument jsn(512);
  {
  
    StaticJsonDocument<200> filter;

    if (useWeather == 1) {
      // Yandex
      filter["clocks"][regId]["weather"]["temp"] = true;  // Достаём температуру - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> temp
      filter["clocks"][regId]["skyColor"] = true;         // Рекомендованный цвет фона
      filter["clocks"][regId]["isNight"] = true;
      filter["clocks"][regId]["weather"]["icon"] = true;  // Достаём иконку - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> icon
      filter["clocks"][regId]["name"] = true;             // Город
      filter["clocks"][regId]["sunrise"] = true;          // Время рассвета
      filter["clocks"][regId]["sunset"] = true;           // Время заката
    } else {
      // OpenWeatherMap
      filter["main"]["temp"] = true;                      // Температура -> main -> temp
      filter["weather"][0]["icon"] = true;                // Достаём иконку -> weather[0] -> icon
      filter["name"] = true;                              // Город
      filter["weather"][0]["description"] = true;         // Строка погодных условий на языке, указаном в запросе
      filter["weather"][0]["id"] = true;                  // Уточненный код погодных условий
      filter["sys"]["sunrise"] = true;                    // Время рассвета
      filter["sys"]["sunset"] = true;                     // Время заката    
    }
    
    // Parse JSON object
    DeserializationError jsn_error = deserializeJson(jsn, payload, DeserializationOption::Filter(filter));

    if (jsn_error) {
      DEBUG(F("JSON не разобран: "));
      DEBUGLN(jsn_error.c_str());

      #if (USE_MQTT == 1)
      doc["result"] = F("ERROR");
      doc["status"] = F("json error");
      serializeJson(jsn, out);      
      SendMQTT(out, TOPIC_WTR);
      #endif
      
      return false;
    }
  }

  String town, sunrise, sunset;
  
  if (useWeather == 1) {
    /*
    Yandex: {"time":1597989853200,
             "clocks":
              { "62":
                { "id":62,
                   "name":"Krasnoyarsk",
                   "offset":25200000,
                   "offsetString":"UTC+7:00",
                   "showSunriseSunset":true,
                   "sunrise":"05:31",
                   "sunset":"20:10",
                   "isNight":false,
                   "skyColor":"#57bbfe",
                   "weather":
                     {"temp":25,
                      "icon":"bkn-d",
                      "link":"https://yandex.ru/pogoda/krasnoyarsk"
                     },
                   "parents": [
                     {"id":11309,
                      "name":"Krasnoyarsk Krai"
                     },
                     {"id":225,
                      "name":"Russia"
                     }
                   ]
                }
              }
            }
    */        
    temperature  = jsn["clocks"][regId]["weather"]["temp"].as<int8_t>();  // Достаём температуру - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> temp
    skyColor     = jsn["clocks"][regId]["skyColor"].as<String>();         // Рекомендованный цвет фона
    isNight      = jsn["clocks"][regId]["isNight"].as<bool>();
    icon         = jsn["clocks"][regId]["weather"]["icon"].as<String>();  // Достаём иконку - Четвёртый уровень вложенности пары ключ/значение clocks -> значение RegionID -> weather -> icon
    town         = jsn["clocks"][regId]["name"].as<String>();             // Город
    sunrise      = jsn["clocks"][regId]["sunrise"].as<String>();          // Время рассвета
    sunset       = jsn["clocks"][regId]["sunset"].as<String>();           // Время заката
    weather_ok   = skyColor.length() == 7;                                // #57bbfe

    if (sunrise.length() > 0) {
      dawn_hour   = sunrise.substring(0,2).toInt();
      dawn_minute = sunrise.substring(3,5).toInt();
    }

    if (sunset.length() > 0) {
      dusk_hour   = sunset.substring(0,2).toInt();
      dusk_minute = sunset.substring(3,5).toInt();
    }

    // Для срабатывания триггера на изменение значений
    set_temperature(temperature);
  } else if (useWeather == 2) {
    /*
    OpenWeatherMap: 
    { "coord":{"lon":92.8672,"lat":56.0184},
      "weather":[{"id":804,"main":"Clouds","description":"пасмурно","icon":"04d"}],
      "base":"stations",
      "main":{"temp":1.9,"feels_like":-2.83,"temp_min":1.9,"temp_max":1.9,"pressure":1018,"humidity":64,"sea_level":1018,"grnd_level":1000},
      "visibility":10000,
      "wind":{"speed":5.56,"deg":267,"gust":8.55},
      "clouds":{"all":100},
      "dt":1728543379,
      "sys":{"type":1,"id":8957,"country":"RU","sunrise":1728518969,"sunset":1728558074},
      "timezone":25200,
      "id":1502026,
      "name":"Красноярск",
      "cod":200    
      }                   
    */

    float tmp    = jsn["main"]["temp"].as<float>();
    temperature  = int(round(tmp));                                       // Температура -> main -> temp
    icon         = jsn["weather"][0]["icon"].as<String>();                // Достаём иконку -> weather[0] -> icon
    isNight      = icon.endsWith("n");                                    // Иконка вида "XXn" - ночная "XXd" - дневная
    town         = jsn["name"].as<String>();                              // Город
    weather_ok   = icon.length() == 3;                                    // Иконка вида "XXn" - ночная "XXd" - дневная
    weather      = jsn["weather"][0]["description"].as<String>();         // Строка погодных условий (английский)
    weather_code = jsn["weather"][0]["id"].as<int16_t>();                 // Уточненный код погодных условий
    sunrise      = jsn["sys"]["sunrise"].as<String>();                    // Время рассвета
    sunset       = jsn["sys"]["sunset"].as<String>();                     // Время заката

    time_t dawn_time = (time_t)(sunrise.toInt());
    time_t dusk_time = (time_t)(sunset.toInt());

    dawn_hour   = hour(dawn_time) + timeZoneOffset;
    dawn_minute = minute(dawn_time);
    dusk_hour   = hour(dusk_time) + timeZoneOffset;
    dusk_minute = minute(dusk_time);

    if (dawn_hour >= 24) dawn_hour -= 24;
    if (dusk_hour >= 24) dusk_hour -= 24;
    
    sunrise = padNum(dawn_hour,2) + ":" + padNum(dawn_minute,2);
    sunset = padNum(dusk_hour,2) + ":" + padNum(dusk_minute,2);
    
    // Для срабатывания триггера на изменение значений
    set_temperature(temperature);
    set_weather(weather);
  } 
  
  if (!weather_ok) {
    DEBUG(F("JSON не содержит данных о погоде"));  
    #if (USE_MQTT == 1)
    doc["result"] = F("ERROR");
    doc["status"] = F("no data");
    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_WTR);
    #endif  
    return false;
  }

  if (useWeather == 1)
    decodeWeather();        // Yandex
  else  
    decodeWeather2();       // OpenWeatherMap
  
  weather_time = millis();  // запомнить время получения погоды с сервера
  init_weather = true;      // Флаг - погода получена
  refresh_weather = false;
  weather_t = 0; 
  weather_cnt = 0;
  
  DEBUG(F("Погода получена: "));
  if (useWeather == 1)
    DEBUGLN(F("Yandex"));
  else  
    DEBUGLN(F("OpenWeatherMap"));
  DEBUG(F("Город: "));
  DEBUGLN(town);
  DEBUG(F("Сейчас: "));
  DEBUG(weather + ", "); 
  if (temperature > 0) DEBUG("+"); 
  DEBUGLN(String(temperature) + "ºC"); // 'º'
  DEBUGLN(String(F("Код иконки: '")) + icon + "'");
  if (useWeather == 1)
    DEBUGLN(String(F("Цвет неба: '")) + skyColor + "'");
  else
    DEBUGLN(String(F("Код погоды: ")) + String(weather_code));
  DEBUGLN(dayTime);
  DEBUG(F("Рассвет: "));
  DEBUGLN(sunrise);
  DEBUG(F("Закат: "));
  DEBUGLN(sunset);
  DEBUGLN();
  
  #if (USE_MQTT == 1)
  doc["result"] = F("OK");
  doc["status"] = weather;
  doc["temp"]   = temperature;
  doc["night"]  = isNight;
  doc["icon"]   = icon;
  doc["town"]   = town;
  doc["server"] = (useWeather == 1 ? String(F("Yandex")) : String(F("OpenWeatherMap")) );
  if (useWeather == 1)
    doc["sky"]    = skyColor;      // для Yandex
  else
    doc["code"]   = weather_code;  // для OpenWeatherMap
  doc["sunrise"]  = sunrise;
  doc["sunset"]   = sunset;
  serializeJson(doc, out);      
  SendMQTT(out, TOPIC_WTR);
  #endif

  if (thisMode == MC_WEATHER) {
    loadingFlag = true;
  }

  return true;
}

/*
Код расшифровки иконки от Yandex. Возможные значения:
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
  ovc — пасмурно
  ovc-minus-ra — пасмурно, временами дождь
  ovc-minus-sn — пасмурно, временами снег
  ovc-ra — пасмурно, дождь
  ovc-sn — пасмурно, снег
  ovc-ts-ra — пасмурно, дождь, гроза
  skc-d — ясно (день)
  skc-n — ясно (ночь)
*/

void decodeWeather(){  
  bool hasDay   = icon.endsWith("-d");
  bool hasNight = icon.endsWith("-n");
  String ico = icon;
  
  dayTime = isNight ? F("Темное время суток") : F("Светлое время суток");

  if (hasDay || hasNight) {
    ico = icon.substring(0, icon.length() - 2);
  }

  if      (ico == F("bkn-minus-ra"))    set_weather(F("облачно с прояснениями, небольшой дождь"));
  else if (ico == F("bkn-minus-sn"))    set_weather(F("облачно с прояснениями, небольшой снег"));
  else if (ico == F("bkn-minus-ra-sn")) set_weather(F("облачно с прояснениями, небольшой снег с дождем"));
  else if (ico == F("bkn"))             set_weather(F("переменная облачность"));
  else if (ico == F("bkn-ra"))          set_weather(F("переменная облачность, дождь"));
  else if (ico == F("bkn-sn"))          set_weather(F("переменная облачность, снег"));
  else if (ico == F("bkn-ra-sn"))       set_weather(F("переменная облачность, снег с дождем"));
  else if (ico == F("bl"))              set_weather(F("метель"));
  else if (ico == F("fg"))              set_weather(F("туман"));
  else if (ico == F("ovc"))             set_weather(F("пасмурно"));
  else if (ico == F("ovc-minus-ra"))    set_weather(F("пасмурно, временами дождь"));
  else if (ico == F("ovc-minus-sn"))    set_weather(F("пасмурно, временами снег"));
  else if (ico == F("ovc-minus-ra-sn")) set_weather(F("пасмурно, временами снег с дождем"));
  else if (ico == F("ovc-ra"))          set_weather(F("пасмурно, дождь"));
  else if (ico == F("ovc-ra-sn"))       set_weather(F("пасмурно, снег с дождем"));
  else if (ico == F("ovc-sn"))          set_weather(F("пасмурно, снег"));
  else if (ico == F("ovc-ts-ra"))       set_weather(F("пасмурно, дождь c грозой"));
  else if (ico == F("skc"))             set_weather(F("ясно"));  
}

void decodeWeather2(){  
  bool hasDay   = icon.endsWith("d");
  bool hasNight = icon.endsWith("n");
  
  dayTime = isNight ? F("Темное время суток") : F("Светлое время суток");

  // Расшифровка погоды при указании в запросе "&lang=ru" сразу возвращается на нужном языке и нет
  // надобности расшифровывать код. Если почему-то расшифровка оказалась пуста - создать ее из кода погодных условия.
  if (weather.length() > 0) return;
  
  // https://openweathermap.org/weather-conditions#How-to-get-icon-URL
  switch (weather_code) {
    case 200: set_weather(F("Гроза, небольшой дождь")); break;         // thunderstorm with light rain
    case 201: set_weather(F("Дождь с грозой")); break;                 // thunderstorm with rain
    case 202: set_weather(F("Гроза, ливни")); break;                   // thunderstorm with heavy rain
    case 210: set_weather(F("Небольшая гроза")); break;                // light thunderstorm
    case 211: set_weather(F("Гроза")); break;                          // thunderstorm
    case 212: set_weather(F("Сильная гроза")); break;                  // heavy thunderstorm
    case 221: set_weather(F("Прерывистые грозы")); break;              // ragged thunderstorm
    case 230: set_weather(F("Гроза, небольшой дождь")); break;         // thunderstorm with light drizzle
    case 231: set_weather(F("Гроза с дождем")); break;                 // thunderstorm with drizzle
    case 232: set_weather(F("Гроза с проливным дождем")); break;       // thunderstorm with heavy drizzle
    case 300: set_weather(F("Мелкий дождь")); break;                   // light intensity drizzle
    case 301: set_weather(F("Моросящий дождь")); break;                // drizzle
    case 302: set_weather(F("Сильный дождь")); break;                  // heavy intensity drizzle
    case 310: set_weather(F("Небольшой дождь")); break;                // light intensity drizzle rain
    case 311: set_weather(F("Моросящий дождь")); break;                // drizzle rain
    case 312: set_weather(F("Сильный дождь")); break;                  // heavy intensity drizzle rain
    case 313: set_weather(F("Ливень, дождь и морось")); break;         // shower rain and drizzle
    case 314: set_weather(F("Сильный ливень, дождь и морось")); break; // heavy shower rain and drizzle
    case 321: set_weather(F("Моросящий дождь")); break;                // shower drizzle  
    case 500: set_weather(F("Небольшой дождь")); break;                // light rain
    case 501: set_weather(F("Умеренный дождь")); break;                // moderate rain
    case 502: set_weather(F("Ливень")); break;                         // heavy intensity rain
    case 503: set_weather(F("Проливной дождь")); break;                // very heavy rain
    case 504: set_weather(F("Проливной дождь")); break;                // extreme rain
    case 511: set_weather(F("Град")); break;                           // freezing rain
    case 520: set_weather(F("Небольшой дождь")); break;                // light intensity shower rain
    case 521: set_weather(F("Моросящий дождь")); break;                // shower rain
    case 522: set_weather(F("Сильный дождь")); break;                  // heavy intensity shower rain
    case 531: set_weather(F("Временами дождь")); break;                // ragged shower rain
    case 600: set_weather(F("Небольшой снег")); break;                 // light snow
    case 601: set_weather(F("Снег")); break;                           // Snow
    case 602: set_weather(F("Снегопад")); break;                       // Heavy snow
    case 611: set_weather(F("Слякоть")); break;                        // Sleet
    case 612: set_weather(F("Легкий снег")); break;                    // Light shower sleet
    case 613: set_weather(F("Ливень, снег")); break;                   // Shower sleet
    case 615: set_weather(F("Мокрый снег")); break;                    // Light rain and snow
    case 616: set_weather(F("Дождь со снегом")); break;                // Rain and snow
    case 620: set_weather(F("Небольшой снегопад")); break;             // Light shower snow
    case 621: set_weather(F("Снегопад, метель")); break;               // Shower snow
    case 622: set_weather(F("Сильный снегопад")); break;               // Heavy shower snow
    case 701: set_weather(F("Туман")); break;                          // mist
    case 711: set_weather(F("Дымка")); break;                          // Smoke
    case 721: set_weather(F("Легкий туман")); break;                   // Haze
    case 731: set_weather(F("Пыльные вихри")); break;                  // sand/ dust whirls
    case 741: set_weather(F("Туман")); break;                          // fog
    case 751: set_weather(F("Песчаные вихри")); break;                 // sand
    case 761: set_weather(F("Пыльные вихри")); break;                  // dust
    case 762: set_weather(F("Вулканический пепел")); break;            // volcanic ash
    case 771: set_weather(F("Шквалистый ветер")); break;               // squalls
    case 781: set_weather(F("Торнадо")); break;                        // tornado
    case 800: set_weather(F("Ясно")); break;                           // clear sky
    case 801: set_weather(F("Небольшая облачность")); break;           // few clouds: 11-25%
    case 802: set_weather(F("Переменная облачность")); break;          // scattered clouds: 25-50%
    case 803: set_weather(F("Облачно с прояснениями")); break;         // broken clouds: 51-84%
    case 804: set_weather(F("Пасмурно")); break;                       // overcast clouds: 85-100%
  }
}

void decodeWeather3() {  
  /* Расшифровка кода погоды от OpenMeteo
    Code	      Description
    0	          Clear sky
    1, 2, 3	    Mainly clear, partly cloudy, and overcast
    45, 48	    Fog and depositing rime fog
    51, 53, 55	Drizzle: Light, moderate, and dense intensity
    56, 57    	Freezing Drizzle: Light and dense intensity
    61, 63, 65	Rain: Slight, moderate and heavy intensity
    66, 67	    Freezing Rain: Light and heavy intensity
    71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
    77	        Snow grains
    80, 81, 82	Rain showers: Slight, moderate, and violent
    85, 86	    Snow showers slight and heavy
    95  	      Thunderstorm: Slight or moderate
    96, 99  	  Thunderstorm with slight and heavy hail  
  */  
}

// Строка цвета, соответствующая температуре
int getTemperatureColor(int8_t temp) {
  if (temp <= -30) return cold_less_30;
  if (temp <= -20) return cold_29_20;
  if (temp <= -10) return cold_19_10;
  if (temp <= -4)  return cold_9_4;
  if (temp <=  3)  return zero_3_3;
  if (temp <=  9)  return hot_4_9;
  if (temp <= 19)  return hot_10_19;
  if (temp <= 29)  return hot_20_29;
  return hot_30_great;  
}

// Получить индекс иконки в массиве иконок погоды
int8_t getWeatherFrame(String icon) {
  if (icon == "skc-d") return 0;                                    // Ясно, день
  if (icon == "skc-n") return 1;                                    // Ясно, ночь
  if (icon == "bkn-d") return 2;                                    // Переменная облачность, день
  if (icon == "bkn-n") return 3;                                    // Переменная облачность, ночь
  if (icon == "bkn-minus-ra-d") return 4;                           // Облачно с прояснениями, небольшой дождь, день
  if (icon == "bkn-minus-ra-n") return 5;                           // Облачно с прояснениями, небольшой дождь, ночь
  if (icon == "bkn-minus-sn-d") return 6;                           // Облачно с прояснениями, небольшой снег, день
  if (icon == "bkn-minus-sn-n") return 7;                           // Облачно с прояснениями, небольшой снег, ночь
  if (icon == "bkn-minus-ra-sn-d") return temperature >= 0 ? 4 : 6; // Облачно с прояснениями, небольшой снег, день
  if (icon == "bkn-minus-ra-sn-n") return temperature >= 0 ? 5 : 7; // Облачно с прояснениями, небольшой снег, ночь
  if (icon == "bkn-ra-d") return 8;                                 // Переменная облачность, дождь, день
  if (icon == "bkn-ra-n") return 9;                                 // Переменная облачность, дождь, ночь
  if (icon == "bkn-sn-d") return 10;                                // Переменная облачность, снег, день
  if (icon == "bkn-sn-n") return 11;                                // Переменная облачность, снег, ночь
  if (icon == "bkn-ra-sn-d") return temperature >= 0 ? 8 : 10;      // Переменная облачность, снег, день
  if (icon == "bkn-ra-sn-n") return temperature >= 0 ? 9 : 11;      // Переменная облачность, снег, ночь
  if (icon == "bl") return 12;                                      // Метель
  if (icon == "fg-d") return 13;                                    // Туман
  if (icon == "ovc") return 14;                                     // Пасмурно
  if (icon == "ovc-minus-ra") return 15;                            // Пасмурно, временами дождь
  if (icon == "ovc-minus-sn") return 16;                            // Пасмурно, временами снег
  if (icon == "ovc-minus-ra-sn") return temperature >= 0 ? 15 : 16; // Пасмурно, временами снег
  if (icon == "ovc-ra") return 17;                                  // Пасмурно, дождь
  if (icon == "ovc-ra-sn") return temperature >= 0 ? 17 : 18;       // Пасмурно, дождь, снег
  if (icon == "ovc-sn") return 18;                                  // Пасмурно, снег
  if (icon == "ovc-ts-ra") return 19;                               // Пасмурно, дождь, гроза 
  return -1;
}

int8_t getWeatherFrame2(String icon) {
  // https://openweathermap.org/weather-conditions#How-to-get-icon-URL
  bool hasDay   = icon.endsWith("d");
  switch (weather_code) {
    case 200: return 19;                         // weather  = F("Гроза, небольшой дождь"); break;         // thunderstorm with light rain
    case 201: return 19;                         // weather  = F("Дождь с грозой"); break;                 // thunderstorm with rain
    case 202: return 19;                         // weather  = F("Гроза, ливни"); break;                   // thunderstorm with heavy rain
    case 210: return 19;                         // weather  = F("Небольшая гроза"); break;                // light thunderstorm
    case 211: return 19;                         // weather  = F("Гроза"); break;                          // thunderstorm
    case 212: return 19;                         // weather  = F("Сильная гроза"); break;                  // heavy thunderstorm
    case 221: return 19;                         // weather  = F("Прерывистые грозы"); break;              // ragged thunderstorm
    case 230: return 19;                         // weather  = F("Гроза, небольшой дождь"); break;         // thunderstorm with light drizzle
    case 231: return 19;                         // weather  = F("Гроза с дождем"); break;                 // thunderstorm with drizzle
    case 232: return 19;                         // weather  = F("Гроза с проливным дождем"); break;       // thunderstorm with heavy drizzle
    case 300: return hasDay ? 8 : 9;             // weather  = F("Мелкий дождь"); break;                   // light intensity drizzle
    case 301: return hasDay ? 8 : 9;             // weather  = F("Моросящий дождь"); break;                // drizzle
    case 302: return hasDay ? 8 : 9;             // weather  = F("Сильный дождь"); break;                  // heavy intensity drizzle
    case 310: return hasDay ? 8 : 9;             // weather  = F("Небольшой дождь"); break;                // light intensity drizzle rain
    case 311: return hasDay ? 8 : 9;             // weather  = F("Моросящий дождь"); break;                // drizzle rain
    case 312: return hasDay ? 8 : 9;             // weather  = F("Сильный дождь"); break;                  // heavy intensity drizzle rain
    case 313: return hasDay ? 8 : 9;             // weather  = F("Ливень, дождь и морось"); break;         // shower rain and drizzle
    case 314: return hasDay ? 8 : 9;             // weather  = F("Сильный ливень, дождь и морось"); break; // heavy shower rain and drizzle
    case 321: return hasDay ? 8 : 9;             // weather  = F("Моросящий дождь"); break;                // shower drizzle  
    case 500: return hasDay ? 4 : 5;             // weather  = F("Небольшой дождь"); break;                // light rain
    case 501: return hasDay ? 4 : 5;             // weather  = F("Умеренный дождь"); break;                // moderate rain
    case 502: return hasDay ? 4 : 5;             // weather  = F("Ливень"); break;                         // heavy intensity rain
    case 503: return hasDay ? 4 : 5;             // weather  = F("Проливной дождь"); break;                // very heavy rain
    case 504: return hasDay ? 4 : 5;             // weather  = F("Проливной дождь"); break;                // extreme rain
    case 511: return hasDay ? 10 : 11;           // weather  = F("Ледяной дождь"); break;                  // freezing rain
    case 520: return hasDay ? 8 : 9;             // weather  = F("Небольшой дождь"); break;                // light intensity shower rain
    case 521: return hasDay ? 8 : 9;             // weather  = F("Моросящий дождь"); break;                // shower rain
    case 522: return hasDay ? 8 : 9;             // weather  = F("Сильный дождь"); break;                  // heavy intensity shower rain
    case 531: return hasDay ? 8 : 9;             // weather  = F("Временами дождь"); break;                // ragged shower rain
    case 600: return hasDay ? 6 : 7;             // weather  = F("Небольшой снег"); break;                 // light snow
    case 601: return 16;                         // weather  = F("Снег"); break;                           // Snow
    case 602: return 18;                         // weather  = F("Снегопад"); break;                       // Heavy snow
    case 611: return temperature > 0 ? 15 : 16;  // weather  = F("Слякоть"); break;                        // Sleet
    case 612: return hasDay ? 10 : 11;           // weather  = F("Легкий снег"); break;                    // Light shower sleet
    case 613: return temperature > 0 ? 17 : 18;  // weather  = F("Ливень, снег"); break;                   // Shower sleet
    case 615: return temperature > 0 ? 17 : 18;  // weather  = F("Небольшой дождь со снегом"); break;      // Light rain and snow
    case 616: return temperature > 0 ? 17 : 18;  // weather  = F("Дождь со снегом"); break;                // Rain and snow
    case 620: return 16;                         // weather  = F("Небольшой снег"); break;                 // Light shower snow
    case 621: return 12;                         // weather  = F("Небольшой снег, метель"); break;         // Shower snow
    case 622: return 18;                         // weather  = F("Сильный снегопад"); break;               // Heavy shower snow
    case 701: return 13;                         // weather  = F("Туман"); break;                          // mist
    case 711: return 13;                         // weather  = F("Дымка"); break;                          // Smoke
    case 721: return 13;                         // weather  = F("Легкий туман"); break;                   // Haze
    case 731: return 13;                         // weather  = F("Пыльные вихри"); break;                  // sand/ dust whirls
    case 741: return 13;                         // weather  = F("Туман"); break;                          // fog
    case 751: return 13;                         // weather  = F("Песчаные вихри"); break;                 // sand
    case 761: return 13;                         // weather  = F("Пыльные вихри"); break;                  // dust
    case 762: return 13;                         // weather  = F("Вулканический пепел"); break;            // volcanic ash
    case 771: return 13;                         // weather  = F("Шквалистый ветер"); break;               // squalls
    case 781: return 13;                         // weather  = F("Торнадо"); break;                        // tornado
    case 800: return hasDay ? 0 : 1;             // weather  = F("Ясно"); break;                           // clear sky
    case 801: return hasDay ? 2 : 3;             // weather  = F("Небольшая облачность"); break;           // few clouds: 11-25%
    case 802: return hasDay ? 2 : 3;             // weather  = F("Переменная облачность"); break;          // scattered clouds: 25-50%
    case 803: return hasDay ? 2 : 3;             // weather  = F("Облачно с прояснениями"); break;         // broken clouds: 51-84%
    case 804: return 14;                         // weather  = F("Пасмурно"); break;                       // overcast clouds: 85-100%
    default:  return -1;
  }  
}

#else

bool getWeather() {
  return false;
}

#endif

uint8_t fade_weather_phase = 0;        // Плавная смена картинок: 0 - плавное появление; 1 - отображение; 2 - затухание
uint8_t fade_step = 0;
uint8_t weather_frame_num = 0;
int8_t  weather_text_x, weather_text_y;

void weatherRoutine() {

  #if (USE_WEATHER == 1)     

    // Есть ли возможность отрисовки температуры большим шрифтом?
    bool big_font = c_size == 2 && (pWIDTH > image_desc.frame_width + 10 || pHEIGHT > image_desc.frame_height + 7);

    uint8_t t = abs(temperature);
    uint8_t dec_t = t / 10;
    uint8_t edc_t = t % 10;

    // ширина текста температуры
    int8_t temp_width = t == 0
      ? (big_font ? 9 : 5)      // Цифра 0 и знак градусов и пробел между ними
      : (big_font ? 15 : 11);   // +/- две цифры градусов

    if (t != 0 && dec_t == 0) temp_width -= (big_font ? 6 : 4); // Если десятки градусов - их не отображать
    if (big_font) {                               // больш шрифт
      if (dec_t == 1) temp_width -= 2;            // 1 занимает 3 колонки а не 5
      if (edc_t == 1) temp_width -= 2;            // 1 занимает 3 колонки а не 5
    } else {
      if (dec_t == 1) temp_width -= 1;            // 1 занимает 2 колонки а не 3
      if (edc_t == 1) temp_width -= 1;            // 1 занимает 2 колонки а не 3
    }

    // Если температура 0 - нужно рисовать знак градуса или букву 'c'
    // Если температура другая - для большого шрифта, если позволяет место - рисовать знак градуса. Если не позволяет - не рисовать.
    bool need_deg = (t == 0) || (big_font && t != 0);
    if (need_deg) temp_width += (big_font ? (t == 0 ? 0 : 4) : (t == 0 ? 3 : 0));    
  #endif
  
  if (loadingFlag) {
    loadingFlag = false;
    // modeCode = MC_WEATHER;
    
    FastLED.clear();                   // очистить экран

    // Загрузить описатель массива изображений погоды
    loadDescriptor(&animation_weather);
    frames_in_image = sizeof(weather_array) / sizeof(weather_array[0]);

    // Координаты вывода изображения - центрировать
    image_desc.options = 1+2+4+16;             // Центрировать по вертикали/горизонтали, есть прозрачные пиксели, перед отрисовкой кадра - заливать цветом
    image_desc.transparent_color = 0x000000;   // Прозрачные пиксели - черные
    image_desc.background_color = 0x000000;    // Заливка - черная (?)
    image_desc.draw_frame_interval = 2500;     // Интервал перехода к следующей картинке
    image_desc.draw_row_interval = 0;          // Рисовка - картинка целиком
    image_desc.move_type = 0;                  // Нет движения картинки

    flip_x = false;
    flip_y = false;
          
    fade_weather_phase = init_weather ? 1 : 0;                         // плавное появление картинки
  }  

  // Если совсем задержки нет - матрица мерцает от постоянного обновления
  delay(5);

  #if (USE_WEATHER == 1)     
    if (useWeather > 0) {
      pos_x = (pWIDTH - image_desc.frame_width - temp_width) / 2 + 1;
      pos_y = (pHEIGHT - image_desc.frame_height - (big_font ? 7 : 5)) / 2 + (big_font ? 8 : 6);
      while(pos_x < 0) pos_x++;
      while(pos_y + image_desc.frame_height > pHEIGHT) pos_y--;
      // Если знак градуса не обязателен к рисованию и он не влазит в ширину - уменьшить ширину текста температуры на знакоместо градуса
      // Исключение - если цифры температуры целиком ниже картинки - знак градуса можно оставить
      if (need_deg && big_font && t != 0 && ((image_desc.frame_width + temp_width) > pWIDTH) && ((image_desc.frame_height + 7) > pHEIGHT)) {
        need_deg = false;
        temp_width -= 4;
        pos_x += 2;
      }
      #if (DEVICE_TYPE == 0)
        weather_text_x = pos_x + (image_desc.frame_width / 2) - 2 - (big_font ? (edc_t != 1 ? 0 : -1) : (edc_t != 1 ? 0 : 1)); 
      #else
        weather_text_x = pos_x + image_desc.frame_width - (big_font ? (edc_t != 1 ? 0 : -1) : (edc_t != 1 ? 0 : 1)); 
      #endif
      if (!big_font && dec_t != 0) weather_text_x--;
      if (!big_font && edc_t == 0) weather_text_x--;
      weather_text_x -= (big_font ? 1 : 0);
      weather_text_y = pos_y - (big_font ? 7 : 5);
    } else {
      pos_x = (pWIDTH - image_desc.frame_width) / 2;
      pos_y = (pHEIGHT - image_desc.frame_height) / 2;      
    }
    while(pos_x < 0) pos_x++;
    while(pos_y < 0) pos_y++;
    if (useWeather > 0) {
      while(weather_text_x + temp_width - 1 > pWIDTH) weather_text_x--;
      while(weather_text_y <= 0) { 
        weather_text_y++;
        if (pos_y + image_desc.frame_height < pHEIGHT) pos_y++;
      }
    }
  #else
    pos_x = (pWIDTH - image_desc.frame_width) / 2;
    pos_y = (pHEIGHT - image_desc.frame_height) / 2;
    weather_frame_num = 0;      
  #endif        

  // Если погода отключена или еще не получена - просто рисуем картинки по кругу
  // Если погода получена - находим индекс отрисовываемой картинки в соответствии с полученной иконкой погоды
  #if (USE_WEATHER == 1)
    if (useWeather > 0 && init_weather) {
      int8_t fr = useWeather == 1 ? getWeatherFrame(icon) : getWeatherFrame2(icon);
      if (fr >= 0) {
        weather_frame_num = fr;
      }
    }
  #endif

  // Нарисовать картинку
  loadImageFrame(weather_array[weather_frame_num]);
  
  uint8_t spd = map8(255-getEffectSpeedValue(MC_WEATHER), 2, 24);   

  // Если находимся в фазе 0 - плавное появление картинки - затенить только что отрисованную картинку, постепенно уменьшая затенение
  if (fade_weather_phase == 0) {
    fade_step += spd;
    if ((uint16_t)fade_step + spd >= 255) {
      fade_weather_phase = 1;
      last_draw_frame = millis();
    } else {  
      fader(255 - fade_step);
    }
  } else

  // Если находимся в фазе 1 - отображение - считаем сколько времени уже отображается, не пора ли переходить к фазе затухания и следующему кадру
  if (fade_weather_phase == 1) {
    if (!init_weather) {
      if (millis() - last_draw_frame > image_desc.draw_frame_interval) {
        fade_weather_phase = 2;
        fade_step = 0;
      }
    } else {
      // Чтобы картинка при известной погоде не выглядела статично - придаем ей некоторое "дыхание"
      uint8_t beat = beatsin8(10, 25, 155);
      fader(beat);
    }
  } else
  
  // Если находимся в фазе 2 - плавное затухание картинки - затенить только что отрисованную картинку, постепенно увеличивая затенение
  if (fade_weather_phase == 2) {
    fade_step += spd;
    if ((uint16_t)fade_step + spd >= 255) {
      fillAll(CRGB(image_desc.background_color));
      fade_step = 0;
      fade_weather_phase = 0;
      weather_frame_num = random8(0,19);
      /*
      weather_frame_num++;
      if (weather_frame_num >= frames_in_image) {
        weather_frame_num = 0;
      } 
      */     
    } else {  
      fader(fade_step);
    }
  }

  #if (USE_WEATHER == 1)     

  // Если температура известна - нарисовать температуру
  if (useWeather > 0 && init_weather) {
    
    // Получить цвет отображения значения температуры
    CRGB color = useTemperatureColor ? CRGB(getTemperatureColor(temperature)) : CRGB::White;
    int16_t temp_x = weather_text_x + temp_width;
    int16_t temp_y = weather_text_y;
    
    // Для правильного позиционирования - рисуем справа налево
    // Нужно ли рисовать букву "c" в малом шрифте или знак градуса в большом шрифте?
    if (need_deg) {
      if (big_font) {
        temp_x -= 4;  
        // Для больших часов рисуем значок градуса
        for(uint8_t i = 0; i < 2; i++) drawPixelXY(getClockX(temp_x), temp_y + 4 + i, color);      
        drawPixelXY(getClockX(temp_x + 1), temp_y+3, color);      
        drawPixelXY(getClockX(temp_x + 1), temp_y+6, color);      
        for(uint8_t i = 0; i < 2; i++) drawPixelXY(getClockX(temp_x+2), temp_y + 4 + i, color);      
      } else {
        temp_x -= 3;  
        // При температуре = 0 - рисуем маленький значок C
        for(uint8_t i = 0; i < 3; i++) {
          drawPixelXY(getClockX(temp_x), temp_y + i, color);      
        }
        drawPixelXY(getClockX(temp_x + 1), temp_y, color);      
        drawPixelXY(getClockX(temp_x + 1), temp_y + 2, color);      
      }
    }

    // Единицы градусов
    temp_x -= (edc_t == 1 ? 3 : 4);
    if (big_font) {
      temp_x -= 2;
      drawDigit5x7(edc_t, getClockX(temp_x), temp_y, color);
    } else {
      drawDigit3x5(edc_t, getClockX(temp_x), temp_y, color);
    }
    temp_x += (dec_t == 0 && edc_t == 1 ? 1 : 0);

    // Десятки градусов
    if (dec_t != 0) {
      temp_x -= (dec_t == 1 ? 3 : 4);
      if (big_font) {
        temp_x -= 2;
        drawDigit5x7(dec_t, getClockX(temp_x), temp_y, color);
      } else {
        drawDigit3x5(dec_t, getClockX(temp_x), temp_y, color);
      }
      temp_x += (dec_t == 1 ? 1 : 0);
    }
            
    // Нарисовать '+' или '-' если температура не 0
    // Горизонтальная черта - общая для '-' и '+'
    if (temperature != 0) {
      uint8_t dy = big_font ? 2 : 0;
      temp_x -= 4;
      for(uint8_t i = 0; i < 3; i++) {
        drawPixelXY(getClockX(temp_x + i), temp_y + 2 + dy, color);      
      }      
      // Для плюса - вертикальная черта
      if (temperature > 0) {
        drawPixelXY(getClockX(temp_x + 1), temp_y + 1 + dy, color);
        drawPixelXY(getClockX(temp_x + 1), temp_y + 3 + dy, color);
      }
    }    
  }
  
  #endif  
}
