#if (USE_MQTT == 1)

// Набор параметров, который требуется отправлять MQTT-клиенту на изменение их состояния
// STATE_KEYS начинается с '|' и заканчивается на '|' для удобства поиска / проверки наличия ключа в строке,
// которые должны быть удалены перед использованием далее для перебора ключей
// Если вам не нужен на стороне MQTT клиента полный перечень параметров - оставьте только те, что вам нужны

// Если вы не хотите отправлять пароли в канал MQTT - удалите из списка разрешенных ключей следующие:
//   AA - пароль точки доступа
//   NA - пароль подключения к сети
//   QW - пароль к MQTT брокеру

#define STATE_KEYS "|W|H|DM|PS|PD|IT|AL|RM|PW|BR|WU|WT|WR|WS|WC|WN|WZ|EF|EN|UE|UT|UC|SE|SS|SQ|BE|CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF|TM|AW|AT|AD|AE|MX|MU|MD|MV|MA|MB|MP|AU|AN|NW|IP|QZ|QA|QP|QS|QU|QR|TE|TI|TS|CT|C2|OM|ST|AM1T|AM1A|AM2T|AM2A|AM3T|AM3A|AM4T|AM4A|AM5A|AM6A|UP|FM|T1|T2|M0|M1|M2|M3|M4|M5|M6|M7|M8|M9|M10|M11|E1|E2|E3"

// Формирование топика сообщения
String mqtt_topic(String topic) {
  String ret_topic = mqtt_prefix;
  if (ret_topic.length() > 0 && !ret_topic.endsWith("/")) ret_topic += "/";
  return ret_topic + topic;
}

// Поместить сообщения для отправки на сервер в очередь
void putOutQueue(String topic, String message) {
  putOutQueue(topic, message, false);
}

void putOutQueue(String topic, String message, bool retain) {
  if (stopMQTT) return;
  bool ok = false;
  ok = mqtt.beginPublish(topic.c_str(), message.length(), retain);
  if (ok) {
    // Если инициация отправки была успешной - заполняем буфер отправки передаваемой строкой сообщения
    mqtt.print(message.c_str());
    // Завершаем отправку. Если пакет был отправлен - возвращается 1, если ошибка отправки - возвращается 0
    ok = mqtt.endPublish() == 1;
    if (ok) {
      // Отправка прошла успешно
      DEBUG(F("MQTT >> OK >> ")); 
      if (retain)
        DEBUG(F("[r] "));
      else   
        DEBUG(F("[ ] "));
      DEBUG(topic);
      DEBUG(F("\t >> ")); 
      DEBUGLN(message);
    }
  }
  // Если отправка не произошла и в очереди есть место - помещаем сообщение в очередь отправки
  if (!ok && outQueueLength < QSIZE_OUT) {
    outQueueLength++;
    tpcQueue[outQueueWriteIdx] = topic;
    outQueue[outQueueWriteIdx] = message;
    rtnQueue[outQueueWriteIdx] = retain;
    outQueueWriteIdx++;
    if (outQueueWriteIdx >= QSIZE_OUT) outQueueWriteIdx = 0;
  }
}

void SendMQTT(String &message, String topic) {
  if (stopMQTT) return;
  putOutQueue(mqtt_topic(topic), message);
}

void notifyUnknownCommand(const char* text) {
  if (stopMQTT) return;
  DynamicJsonDocument doc(256);
  String out;
  doc["message"] = F("unknown command");
  doc["text"]    = String(F("неизвестная команда '")) + String(text) + String("'");
  serializeJson(doc, out);      
  SendMQTT(out, TOPIC_ERR);
}

bool subscribeMqttTopicCmd() {
  bool ok = false;
  if (mqtt.connected()) {
    DEBUG(F("Подписка на topic='cmd' >> "));
    ok = mqtt.subscribe(mqtt_topic(TOPIC_CMD).c_str());
    if (ok) DEBUGLN(F("OK"));
    else    DEBUGLN(F("FAIL"));
  }
  return ok;
}

void checkMqttConnection() {

  // Ели нет соединения  интернетом - незачем проверять наличие подключения к MQTT-серверу
  if (!wifi_connected) return;

  if (!mqtt.connected() && millis() < nextMqttConnectTime) return;

  // Проверить - выполнена ли подписка на топик команд, если нет - подписаться
  if (!stopMQTT && !mqtt_topic_subscribed) {
    mqtt_topic_subscribed = subscribeMqttTopicCmd();
  }
  // Отправить сообщение из очереди, если очередь содержит сообщения
  // Проверка наличия подключения и защитного интервала отправки проверяется внутри вызова
  if (!stopMQTT && mqtt_topic_subscribed) {
    processOutQueue();
  }

  // Если связь с MQTT сервером не установлена - выполнить переподключение к серверу
  // Слишком частая проверка мешает приложению на смартфоне общаться с программой - запрос блокирующий и при неответе MQTT сервера
  // слишком частые попытки подключения к MQTT серверу не дают передаваться данным из / в приложение - приложение "отваливается"
  if (!stopMQTT && !mqtt.connected() && (mqtt_conn_last == 0 || (millis() - mqtt_conn_last > 2500))) {
    if (!mqtt_connecting) {
      DEBUG(F("\nПодключаемся к MQTT-серверу '"));
      DEBUG(mqtt_server);
      DEBUG(":");
      DEBUG(mqtt_port);
      DEBUG(F("'; ClientID -> '"));
      DEBUG(mqtt_client_name);
      DEBUG(F("' ..."));
    }
    mqtt_topic_subscribed = false;
    mqtt_conn_last = millis();

    String topic = mqtt_topic(TOPIC_PWR);
    uint32_t t = millis();

    if (mqtt.connect(mqtt_client_name.c_str(), mqtt_user, mqtt_pass, topic.c_str(), 0, true, "offline")) {
      stopMQTT = false;
      mqtt_connecting = false;      
      DEBUGLN(F("\nПодключение к MQTT-серверу выполнено."));
      if (outQueueLength > 0) {
        DEBUG(F("Сообщений в очереди отправки: "));  
        DEBUGLN(outQueueLength);  
      }
      putOutQueue(topic, "online", true);
    } else {      
      DEBUG(".");
      mqtt_connecting = true;
      mqtt_conn_cnt++;
      if (mqtt_conn_cnt == 80) {
        mqtt_conn_cnt = 0;
        DEBUGLN();
      }
    }

    // Если сервер недоступен и попытка соединения отвалилась по таймауту - следующую попытку подключения осуществлять не ранее чем через минут.
    // Попытка соединения - операция блокирующая и когда сервер недоступен - пауза примерно 18 секунд и все замирает.
    // Пока не знаю как уменьшить таймаут соединения, но так будет все замирать хотя бы раз в минуту
    if (millis() - t > MQTT_CONNECT_TIMEOUT) {
      nextMqttConnectTime = millis() + MQTT_RECONNECT_PERIOD;
    }
  }
  // Проверить необходимость отправки сообщения об изменении состояния клиенту MQTT
  if (!stopMQTT && mqtt.connected() && changed_keys.length() > 1) {
    // Удалить первый '|' и последний '|' и отправить значения по сформированному списку
    if (changed_keys[0] == '|') changed_keys = changed_keys.substring(1);
    if (changed_keys[changed_keys.length() - 1] == '|') changed_keys = changed_keys.substring(0, changed_keys.length()-1);
    SendCurrentState(changed_keys, TOPIC_STT);
    changed_keys = "";   
    // Если после отправки сообщений на MQTT-сервер флаг намерения useMQTT сброшен (не использовать MQTT),
    // а флаг результата - MQTT еще не остановлен - установить состояние "MQTT канал остановлен"
    if (!useMQTT && !stopMQTT) stopMQTT = true; 
  }
}

// Отправка в MQTT канал - текущие значения переменных
void SendCurrentState(String keys, String topic) {

  if (stopMQTT) return;

  if (keys[0] == '|') keys = keys.substring(1);
  if (keys[keys.length() - 1] == '|') keys = keys.substring(0, keys.length()-1);
  
  // Если строк в textLines очень много (LT) или много файлов эффектов с длинными именами (LF) - они могут не поместиться в JsonDocument в 2048 байт
  // Тогда можно увеличить размер документа дл 3072 байт. На ESP32 где много оперативы это пройдет безболезненно, на ESP8266 могут начаться падения 
  // при нехватке памяти - malloc() не сможет выделить память. Тогда уменьшать количество текста бегущей строки, а  имена файлам эффектов давать короткие
  // Менее 2048 байт в режиме пакетной отправки состояния параметров выделять нельзя - они не влезут в буфер документа

  bool have_big_size_key = 
    keys.indexOf("LE") >= 0 || 
    keys.indexOf("LF") >= 0 || 
    keys.indexOf("LT") >= 0 || 
    keys.indexOf("S1") >= 0 || 
    keys.indexOf("S2") >= 0 || 
    keys.indexOf("S3") >= 0 || 
    keys.indexOf("SQ") >= 0;
  
  int16_t doc_size = have_big_size_key ? 2048 : 128;   

  DynamicJsonDocument doc(doc_size);
  DynamicJsonDocument value_doc(128);
  JsonVariant value;

  String out, key, s_tmp;
  bool big_size_key, retain;
  int16_t pos_start = 0;
  int16_t pos_end = keys.indexOf('|', pos_start);
  int16_t len = keys.length();
  if (pos_end < 0) pos_end = len;
   
  // Строка keys содержит ключи запрашиваемых данных, разделенных знаком '|', например "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
  while (pos_start < len && pos_end >= pos_start) {
    if (pos_end > pos_start) {      
      key = keys.substring(pos_start, pos_end);
      // Все значения состояния кроме UpTime ("UP") отправляются с retain == true;
      retain = key != "UP";
      if (key.length() > 0) {
        value_doc.clear();
        value = value_doc.to<JsonVariant>();
        s_tmp = getStateValue(key, thisMode, &value);                
        if (s_tmp.length() > 0) {
          // Если режим отправки сообщений - каждый параметр индивидуально или ключ имеет значение большой длины - отправить полученный параметр отдельным сообщением  
          // Параметр "UP" также всегда отправляется отдельным сообщением
          big_size_key = key == "LE" || key == "LF" || key == "LT" || key == "S1" || key == "S2" || key == "S3" || key == "SQ";
          // Топик сообщения - основной топик плюс ключ (имя параметра)
          if (big_size_key) 
            out = s_tmp;
          else  
            out = value.isNull() ? "" : value.as<String>();
          s_tmp = topic + "/" + key;             
          putOutQueue(mqtt_topic(s_tmp), out, retain);
        }
      }      
    }
    pos_start = pos_end + 1;
    pos_end = keys.indexOf('|', pos_start);
    if (pos_end < 0) pos_end = len;
  }
}

// Получение строки пары ключ-значение в формате json;
String getKVP(String &key, JsonVariant &value) {
  String out;
  DynamicJsonDocument doc(256);
  doc[key] = value;
  serializeJson(doc, out);
  return out;  
}

// Отправка в MQTT канал  состояния всех параметров при старте прошивки
void mqttSendStartState() {
  if (stopMQTT) return;

  // Отправка неизменяемых списков - эффекты, звуки будильника и рассвета
  // Для отправки этих длинных строк используется тот же json-документ, который позже используется для отправки и хранения свойств состояния
  // поэтому отправка этих списков выполняется один раз при старте программы (с флагом retain), далее json-документ используется по назначению
  // Список эффектов
  SendCurrentState("LE", TOPIC_STT);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета 

  #if (USE_MP3 == 1)  
  // Список звуков будильника
  SendCurrentState("S1", TOPIC_STT);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  
  // Список звуков рассвета
  SendCurrentState("S2", TOPIC_STT);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета

  // Список звуков бегущей строки
  SendCurrentState("S3", TOPIC_STT);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  #endif  

  // Отправить список строк
  SendCurrentState("LT", TOPIC_STT);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета

  #if (USE_SD == 1)  
    // Отправить список файлов, загруженный с SD-карточки
    SendCurrentState("LF", TOPIC_STT);  // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  #endif  

  // Список параметров подлежащих отправке на сервер
  SendCurrentState(STATE_KEYS, TOPIC_STT);  
}

// Отправка сообщений из очереди на сервер
void processOutQueue() {
  if (stopMQTT) {
    outQueueReadIdx = 0;
    outQueueWriteIdx = 0;
    outQueueLength = 0;
    return;
  }

  if (mqtt.connected() && outQueueLength > 0) {    
    // Топик и содержимое отправляемого сообщения
    String topic = tpcQueue[outQueueReadIdx];
    String message = outQueue[outQueueReadIdx];
    bool   retain = rtnQueue[outQueueReadIdx];
    // Пытаемся отправить. Если инициализация отправки не удалась - возвращается false; Если удалась - true
    bool ok = mqtt.beginPublish(topic.c_str(), message.length(), retain);
    if (ok) {
      // Если инициация отправки была успешной - заполняем буфер отправки передаваемой строкой сообщения
      mqtt.print(message.c_str());
      // Завершаем отправку. Если пакет был отправлен - возвращается 1, если ошибка отправки - возвращается 0
      ok = mqtt.endPublish() == 1;
    }
    if (ok) {      
      // Отправка прошла успешно
      DEBUG(F("MQTT >> OK >> ")); 
      if (retain)
        DEBUG(F("[r] "));
      else   
        DEBUG(F("[ ] "));
      DEBUG(topic);
      DEBUG(F("\t >> ")); 
      DEBUGLN(message);
      // Извлекаем сообщение из очереди
      tpcQueue[outQueueReadIdx] = "";
      outQueue[outQueueReadIdx] = "";
      rtnQueue[outQueueReadIdx] = false;
      outQueueReadIdx++;
      if (outQueueReadIdx >= QSIZE_OUT) outQueueReadIdx = 0;
      outQueueLength--;
    } else {
      // Отправка не удалась
      DEBUG(F("MQTT >> FAIL >> ")); 
      DEBUG(topic);
      DEBUG(F("\t >> ")); 
      DEBUGLN(message);
    }
  }  
}

#endif
