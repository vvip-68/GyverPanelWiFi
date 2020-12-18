#if (USE_MQTT == 1)

// Набор параметров, который требуется отправлять MQTT-клиенту на изменение их состояния
// STATE_KEYS начинается с '|' и заканчивается на '|' для удобства поиска / проверки наличия ключа в строке,
// которые должны быть удалены перед использованием далее для перебора ключей
// Если вам не нужен на стороне MQTT клиента полный перечент параметров - оставьте только те, что вам нужны
#define STATE_KEYS "|W|H|DM|PS|PD|IT|AL|RM|PW|BR|WU|WT|WR|WS|WC|WN|WZ|EF|EN|UE|UT|UC|SE|SS|SQ|BE|CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF|TM|AW|AT|AD|AE|MX|MU|MD|MV|MA|MB|MP|AU|AN|AA|NW|NA|IP|QZ|QA|QP|QS|QU|QW|QD|QR|TE|TI|TS|CT|C2|OM|ST|AM1T|AM1A|AM2T|AM2A|AM3T|AM3A|AM4T|AM4A|"

// Получение ID клиента при подключениик серверу
String mqtt_client() {
  return String(MQTT_CLIENT_ID) + "-" + String(DEVICE_ID);
}

// Формирование топика сообщения
String mqtt_topic(String topic) {
  String ret_topic = mqtt_prefix;
  if (ret_topic.length() > 0 && !ret_topic.endsWith("/")) ret_topic += "/";
  return ret_topic + topic;
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
      Serial.print(mqtt_client());
      Serial.print(F("' ..."));
    }
    mqtt_topic_subscribed = false;
    mqtt_conn_last = millis();

    String topic = mqtt_topic(TOPIC_PWR);
    String json_offline = "{\"power\":\"offline\",\"type\":\"hard\"}";
    String json_online = "{\"power\":\"online\",\"type\":\"hard\"}";

    if (mqtt.connect(mqtt_client().c_str(), mqtt_user, mqtt_pass, topic.c_str(), 0, true, json_offline.c_str())) {
      Serial.println(F("\nПодключение к MQTT-серверу выполнено."));
      if (outQueueLength > 0) {
        Serial.print(F("Сообщений в очереди отправки: "));  
        Serial.println(outQueueLength);  
      }
      putOutQueue(topic, json_online, true);
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

  // Если строк в textLines очень много (LT) или много файлов эффектов с длинными именами (ДА) - они могут не поместиться в JsonDocument в 2048 байт
  // Тогда можно увеличить размер документа дл 3072 байт. На ESP32 где много оперативы это пройдет безболезненно, на ESP8266 могут начаться падения 
  // при нехватке памяти - malloc() не сможет выделить память. Тогда уменьшать количество текста бегущей строки, а  имена файлам эффектов давать короткие
  //  Менее 2048 бвйт в режиме пакетной отправки состояния параметров выделяьть нельзя - они не влезут в буфер документа
  int16_t doc_size = mqtt_state_packet || keys == "LF" || keys == "LT" ? 2048 : 128;   

  DynamicJsonDocument doc(doc_size);
  DynamicJsonDocument value_doc(128);
  JsonVariant value;

  String out, key, s_tmp;
  int16_t pos_start = 0;
  int16_t pos_end = keys.indexOf('|', pos_start);
  int16_t len = keys.length();
  if (pos_end < 0) pos_end = len;
   
  // Строка keys содержит ключи запрашиваемых данных, разделенных знаком '|', например "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
  while (pos_start < len && pos_end >= pos_start) {
    if (pos_end > pos_start) {      
      key = keys.substring(pos_start, pos_end);
      if (key.length() > 0) {
        value_doc.clear();
        value = value_doc.to<JsonVariant>();
        s_tmp = getStateValue(key, thisMode, &value);                
        if (s_tmp.length() > 0) {
          if (key == "PS") {
            // Изменение параметра программного вкл/выкл панели отправляется дополнительно в отдельный топик
            String pwr_state = isTurnedOff ? "{\"power\":\"offline\",\"type\":\"soft\"}" : "{\"power\":\"online\",\"type\":\"soft\"}";
            putOutQueue(mqtt_topic(TOPIC_PWR), pwr_state, true);
          } 
          else if (key == "LE" || key == "LF" || key == "LT" || key == "S1" || key == "S2") {
            // Получение длинных списков - в value_doc недостаточно места - getStateValue() в value вернет null, но возвращаемое значение - нужная нам строка
            doc[key] = s_tmp;
          }
          // Если режим отправки сообщений - каждый параметр индивидуально - отправить полученный параметр отдельным сообщением          
          else if (immediate) {
            // Топик сообщения - основной топик плюс ключ (имя параметра)
            s_tmp = topic + "/" + key;      
            out = getKVP(key, value);
            putOutQueue(mqtt_topic(s_tmp), out, true);
          } else {
            doc[key] = value;
          }
        }
      }      
    }
    pos_start = pos_end + 1;
    pos_end = keys.indexOf('|', pos_start);
    if (pos_end < 0) pos_end = len;
  }

  // Если режим отправки состояния пакетами - отправить клиенту сформированный пакет
  if (!immediate) {
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
  SendCurrentState("LE", String(TOPIC_STC) + "/LE", false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета 

  #if (USE_MP3 == 1)  
  // Список звуков будильника
  SendCurrentState("S1", String(TOPIC_STC) + "/S1", false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  
  // Список звуков рассвета
  SendCurrentState("S2", String(TOPIC_STC) + "/S2", false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
  #endif  

  // Отправить список строк
  SendCurrentState("LT", String(TOPIC_STC) + "/LT", false);    // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета

  #if (USE_SD == 1)  
    // Отправить список файлов, загруженный с SD-карточки
    SendCurrentState("LF", String(TOPIC_STC) + "/LF", false);  // false - т.к. хотя и один параметр, но обязательно требуется большой буфер пакета
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
