#if (USE_MQTT == 1)

// Набор параметров, который требуется отправлять MQTT-клиенту на изменение их состояния
// STATE_KEYS начинается с '|' и заканчивается на '|' для удобства поиска / проверки наличия ключа в строке,
// которые должны быть удалены перед использованием далее для перебора ключей
// Если вам не нужен на стороне MQTT клиента полный перечент параметров - оставьте только те, что вам нужны
#define STATE_KEYS "|W|H|DM|PS|PD|IT|AL|RM|PW|BR|WU|WT|WR|WS|WC|WN|WZ|EF|EN|UE|UT|UC|SE|SS|SQ|BE|CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF|TM|AW|AT|AD|AE|MX|MU|MD|MV|MA|MB|MP|AU|AN|NW|IP|QZ|QA|QP|QS|QU|QD|QR|TE|TI|TS|CT|C2|OM|ST|AM1T|AM1A|AM2T|AM2A|AM3T|AM3A|AM4T|AM4A|AM5A|AM6A|UI|UP|"

// Формирование топика сообщения
String mqtt_topic(String topic) {
  String ret_topic = mqtt_prefix;
  if (ret_topic.length() > 0 && !ret_topic.endsWith("/")) ret_topic += "/";
  return ret_topic + host_name + "/" + topic;
}

// Поместить сообщения для отправки на сервер в очередь
void putOutQueue(String topic, String message, bool retain = false) {
  if (stopMQTT) return;
  bool ok = false;
  // Если в настройках сервера MQTT нет задержки между отправками сообщений - пытаемся отправить сразу без помещения в очередь
  if (mqtt_send_delay == 0) {
    ok = mqtt.beginPublish(topic.c_str(), message.length(), retain);
    if (ok) {
      // Если инициация отправки была успешной - заполняем буфер отправки передаваемой строкой сообщения
      mqtt.print(message.c_str());
      // Завершаем отправку. Если пакет был отправлен - возвращается 1, если ошибка отправки - возвращается 0
      ok = mqtt.endPublish() == 1;
      if (ok) {
        // Отправка прошла успешно
        Serial.print(F("MQTT >> OK >> ")); 
        Serial.print(topic);
        Serial.print(F("\t >> ")); 
        Serial.println(message);
      }
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
  if (mqtt.connected() && millis() - mqtt_send_last > mqtt_send_delay) {
    Serial.print(F("Подписка на topic='cmd' >> "));
    ok = mqtt.subscribe(mqtt_topic(TOPIC_CMD).c_str());
    if (ok) Serial.println(F("OK"));
    else    Serial.println(F("FAIL"));
    mqtt_send_last = millis();
  }
  return ok;
}

void checkMqttConnection() {

  // Ели нет оединения  интернетом - незачем проверять наличие подключения к MQTT-ерверу
  if (!wifi_connected) return;
  
  // Проверить - выполнена ли подписка на топик команд, если нет - подписаться
  if (!stopMQTT && !mqtt_topic_subscribed) {
    mqtt_topic_subscribed = subscribeMqttTopicCmd();
  }
  // Отправить сообщение из очереди, если очередь содержит сообщения
  // Проверка наличия подключения и защитны интервала отправки проверяется внутри вызова
  if (!stopMQTT && mqtt_topic_subscribed) {
    processOutQueue();
  }
  // Если связь с MQTT сервером не установлена - выполнить переподключение к серверу
  // Слишком частая проверка мешает приложению на смартфоне общаться с программой - запрос блокирующий и при неответе MQTT сервера
  // слишком частые попытки подключения к MQTT серверу не дают передаваться данным из / в приложение - приложение "отваливается"
  if (!stopMQTT && !mqtt.connected() && (mqtt_conn_last == 0 || (millis() - mqtt_conn_last > 2500))) {
    if (!mqtt_connecting) {
      Serial.print(F("\nПодключаемся к MQTT-серверу '"));
      Serial.print(mqtt_server);
      Serial.print(":");
      Serial.print(mqtt_port);
      Serial.print(F("'; ClientID -> '"));
      Serial.print(host_name);
      Serial.print(F("' ..."));
    }
    mqtt_topic_subscribed = false;
    mqtt_conn_last = millis();

    String topic = mqtt_topic(TOPIC_PWR);

    if (mqtt.connect(host_name.c_str(), mqtt_user, mqtt_pass, topic.c_str(), 0, true, "offline")) {
      Serial.println(F("\nПодключение к MQTT-серверу выполнено."));
      if (outQueueLength > 0) {
        Serial.print(F("Сообщений в очереди отправки: "));  
        Serial.println(outQueueLength);  
      }
      putOutQueue(topic, "online", true);
      mqtt_connecting = false;      
    } else {      
      Serial.print(".");
      mqtt_connecting = true;
      mqtt_conn_cnt++;
      if (mqtt_conn_cnt == 80) {
        mqtt_conn_cnt = 0;
        Serial.println();
      }
    }
  }

  // Проверить необходимость отправки сообщения об изменении состояния клиенту MQTT
  if (!stopMQTT && mqtt.connected() && changed_keys.length() > 1) {
    // Если пакетная отправка - нужно отправлять весь пакет, т.к сообщение статуса имеет флаг retain v всегда должно содержать полный набор параметров.
    // Если отправлять только изменившиеся - они заместят топик и он не будет содержать весь набор
    // Однако, если запрос оттправки состоит только из одного параметра - "UP" - отправлять только его, а не все ключи пакетом
    if (mqtt_state_packet && changed_keys != "|UP|") changed_keys = STATE_KEYS;
    // Удалить первый '|' и последний '|' и отправить значения по сформированному списку
    if (changed_keys[0] == '|') changed_keys = changed_keys.substring(1);
    if (changed_keys[changed_keys.length() - 1] == '|') changed_keys = changed_keys.substring(0, changed_keys.length()-1);
    SendCurrentState(changed_keys, TOPIC_STT, !mqtt_state_packet);
    changed_keys = "";   
    // Если после отправки сообщений на MQTT-сервер флаг намерения useMQTT сброшен (не использзовать MQTT),
    // а флаг результата - MQTT еще не остановлен - установить состояние "MQTT канал остановленЭ
    if (!useMQTT && !stopMQTT) stopMQTT = true; 
  }
}

// Отправка в MQTT канал - текущие значения переменных
void SendCurrentState(String keys, String topic, bool immediate) {

  if (stopMQTT) return;

  if (keys[0] == '|') keys = keys.substring(1);
  if (keys[keys.length() - 1] == '|') keys = keys.substring(0, keys.length()-1);
  
  // Если строк в textLines очень много (LT) или много файлов эффектов с длинными именами (LF) - они могут не поместиться в JsonDocument в 2048 байт
  // Тогда можно увеличить размер документа дл 3072 байт. На ESP32 где много оперативы это пройдет безболезненно, на ESP8266 могут начаться падения 
  // при нехватке памяти - malloc() не сможет выделить память. Тогда уменьшать количество текста бегущей строки, а  имена файлам эффектов давать короткие
  // Менее 2048 бвйт в режиме пакетной отправки состояния параметров выделяьть нельзя - они не влезут в буфер документа
  int16_t doc_size = !immediate || mqtt_state_packet ? 2048 : 128;   

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
          if (immediate || key == "UP") {
            // Топик сообщения - основной топик плюс ключ (имя параметра)
            if (big_size_key) 
              out =  s_tmp;
            else  
              out = value.isNull() ? "" : value.as<String>();
            s_tmp = topic + "/" + key;             
            putOutQueue(mqtt_topic(s_tmp), out, retain);
          } else {
            if (big_size_key) {
              out = "{\"" + key + "\":\"" + s_tmp + "\"}";
              s_tmp = topic + "/" + key;
              putOutQueue(mqtt_topic(s_tmp), out, retain);              
            } else {
              doc[key] = value;
            }
          }
        }
      }      
    }
    pos_start = pos_end + 1;
    pos_end = keys.indexOf('|', pos_start);
    if (pos_end < 0) pos_end = len;
  }

  // Если режим отправки состояния пакетами - отправить клиенту сформированный пакет
  if (!immediate && !doc.isNull()) {
    out = "";
    serializeJson(doc, out);  
    putOutQueue(mqtt_topic(topic), out, true);
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
  // Для отправи этих длинных строк используется тот же json-документ, который позже используется для отправки и хранения свойств состояния
  // поэтому отправка этих списков выполняется один раз при старте программы (с флагом retain), далее json-документ используется по назначению
  // Список эффектов
  SendCurrentState("LE", TOPIC_STT, false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета 

  #if (USE_MP3 == 1)  
  // Список звуков будильника
  SendCurrentState("S1", TOPIC_STT, false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  
  // Список звуков рассвета
  SendCurrentState("S2", TOPIC_STT, false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета

  // Список звуков бегущей строки
  SendCurrentState("S3", TOPIC_STT, false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  #endif  

  // Отправить список строк
  SendCurrentState("LT", TOPIC_STT, false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета

  #if (USE_SD == 1)  
    // Отправить список файлов, загруженный с SD-карточки
    SendCurrentState("LF", TOPIC_STT, false);  // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  #endif  

  // Список параметров подлежащих отправке на сервер
  SendCurrentState(STATE_KEYS, TOPIC_STT, !mqtt_state_packet);  
}

// Отправка сообщений из очереди на червер
void processOutQueue() {
  if (stopMQTT) {
    outQueueReadIdx = 0;
    outQueueWriteIdx = 0;
    outQueueLength = 0;
    return;
  }

  if (mqtt.connected() && outQueueLength > 0 && millis() - mqtt_send_last >= mqtt_send_delay) {    
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
      Serial.print(F("MQTT >> OK >> ")); 
      Serial.print(topic);
      Serial.print(F("\t >> ")); 
      Serial.println(message);
      // Извлекаем сообщение из очереди
      tpcQueue[outQueueReadIdx] = "";
      outQueue[outQueueReadIdx] = "";
      rtnQueue[outQueueReadIdx] = false;
      outQueueReadIdx++;
      if (outQueueReadIdx >= QSIZE_OUT) outQueueReadIdx = 0;
      outQueueLength--;
    } else {
      // Отправка не удалась
      Serial.print(F("MQTT >> FAIL >> ")); 
      Serial.print(topic);
      Serial.print(F("\t >> ")); 
      Serial.println(message);
    }
    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
  }  
}

#endif
