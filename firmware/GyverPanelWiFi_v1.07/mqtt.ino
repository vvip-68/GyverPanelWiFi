#if (USE_MQTT == 1)

String mqtt_client() {
  return String(MQTT_CLIENT_ID) + "-" + String(DEVICE_ID);
}

String mqtt_topic(String topic) {
  // Если для MQTT сервера требуется, чтобы топик начинался с user_id - топик MQTT сообщения формируется как MQTT_USER + "/" + MQTT_CLIENT_ID + "-" - DEVICE_ID + "/" + TOPIC_XXX
  // Если для MQTT сервера НЕ требуется, чтобы топик начинался с user_id - топик MQTT сообщения формируется как MQTT_CLIENT_ID + "-" - DEVICE_ID + "/" + TOPIC_XXX
  // где TOPIC_XXX - один из TOPIC_CMD, TOPIC_DTA, TOPIC_NFO, TOPIC_ERR, TOPIC_ACK
  String ret_topic = "";
  if (MQTT_USE_PREFIX == 1) {
    ret_topic = String(mqtt_user) + "/";
  }
  return ret_topic + mqtt_client() + "/" + topic;
}

void checkMqttConnection() {
  // Проверка чаще чем раз в 20 секунд мешает приложению на смартфоне общаться с программой - запрос блокирующий и при неответе MQTT сервера
  // слишком частые попытки подключения к MQTT серверу не дают передаваться данным из / в приложение - приложение "отваливается"
  if (useMQTT && !mqtt.connected() && (mqtt_conn_last == 0 || (millis() - mqtt_conn_last > 20000))) {
    if (!mqtt_connecting) {
      Serial.print(F("\nПодключаемся к MQTT-серверу '"));
      Serial.print(mqtt_server);
      Serial.print(":");
      Serial.print(mqtt_port);
      Serial.print(F("'; ClientID -> '"));
      Serial.print(mqtt_client());
      Serial.print(F("' ..."));
    }
    mqtt_conn_last = millis();
    if (mqtt.connect(mqtt_client().c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(F("\nПодключение к MQTT-серверу выполнено.\n"));
      mqtt.subscribe(mqtt_topic(TOPIC_CMD).c_str());        
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
  if (mqtt.connected()) {
    processOutQueue();
  }
}

void SendMQTT(String &message) {
  if (!useMQTT) return;
  if (mqtt.connected() && millis() - mqtt_send_last > MQTT_SEND_DELAY) {
    Serial.print(F("MQTT >> ")); 
    Serial.println(message); 
    mqtt.beginPublish(mqtt_topic(TOPIC_DTA).c_str(), message.length(), false);
    mqtt.print(message.c_str());
    mqtt.endPublish();
    mqtt_send_last = millis();
  } else {
    putOutQueue(mqtt_topic(TOPIC_DTA).c_str(), message.c_str());
  }  
}

void NotifyInfo(String &message) {
  if (!useMQTT) return;
  String data = "NFO:" + message;
  if (mqtt.connected() && millis() - mqtt_send_last > MQTT_SEND_DELAY) {
    Serial.print(F("MQTT >> ")); 
    Serial.println(data); 
    mqtt.beginPublish(mqtt_topic(TOPIC_NFO).c_str(), data.length(), false);
    mqtt.print(data.c_str());
    mqtt.endPublish();
    mqtt_send_last = millis();
  } else {
    putOutQueue(mqtt_topic(TOPIC_NFO).c_str(), data.c_str());
  }  
}

void NotifyError(String &message) {
  if (!useMQTT) return;
  String data = "ERR:" + message;
  if (mqtt.connected() && millis() - mqtt_send_last > MQTT_SEND_DELAY) {
    Serial.print(F("MQTT >> ")); 
    Serial.println(data); 
    mqtt.beginPublish(mqtt_topic(TOPIC_ERR).c_str(), data.length(), false);
    mqtt.print(data.c_str());
    mqtt.endPublish();
    mqtt_send_last = millis();
  } else {
    putOutQueue(mqtt_topic(TOPIC_ERR).c_str(), data.c_str());
  }  
}

void NotifyAck() {
  if (!useMQTT) return;
  String data = F("ACK");
  if (mqtt.connected() && millis() - mqtt_send_last > MQTT_SEND_DELAY) {    
    Serial.print(F("MQTT >> ")); 
    Serial.println(data); 
    mqtt.publish(mqtt_topic(TOPIC_ACK).c_str(), data.c_str());
    mqtt_send_last = millis();
  } else {
    putOutQueue(mqtt_topic(TOPIC_ACK).c_str(), data.c_str());
  }  
}

void notifyUnknownCommand(const char* text) {
  if (!useMQTT) return;
  DynamicJsonDocument doc(512);
  String out;
  doc["message"] = F("unknown command");
  doc["text"]    = String(F("неизвестная команда '")) + String(text) + String("'");
  serializeJson(doc, out);      
  NotifyError(out);
}

void putOutQueue(const char* topic, const char* message) {
  if (!useMQTT) return;
  if (outQueueLength < QSIZE_OUT) {
    outQueueLength++;
    tpcQueue[outQueueWriteIdx] = String(topic);
    outQueue[outQueueWriteIdx] = String(message);
    outQueueWriteIdx++;
    if (outQueueWriteIdx >= QSIZE_OUT) outQueueWriteIdx = 0;
  }
}

void processOutQueue() {
  if (!useMQTT) {
    outQueueReadIdx = 0;
    outQueueWriteIdx = 0;
    outQueueLength = 0;
    return;
  }
  if (mqtt.connected() && outQueueLength > 0 && millis() - mqtt_send_last > MQTT_SEND_DELAY) {    
    String topic = tpcQueue[outQueueReadIdx];
    String message = outQueue[outQueueReadIdx];
    outQueueReadIdx++;
    if (outQueueReadIdx >= QSIZE_OUT) outQueueReadIdx = 0;
    outQueueLength--;
    Serial.print(F("MQTT >> ")); 
    Serial.println(message); 
    mqtt.beginPublish(topic.c_str(), message.length(), false);
    mqtt.print(message.c_str());
    mqtt.endPublish();
    mqtt_send_last = millis();
  }  
}

#endif
