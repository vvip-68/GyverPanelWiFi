void SendMQTT(String &message) {
  if (!useMQTT) return;
  if (mqtt.connected()) {
    Serial.print(F("MQTT >> ")); 
    Serial.println(message); 
    mqtt.beginPublish(topic_dta.c_str(), message.length(), false);
    mqtt.print(message.c_str());
    mqtt.endPublish();
  } else {
    putOutQueue(topic_dta, message);
  }  
}

void NotifyInfo(String &message) {
  if (!useMQTT) return;
  String data = "NFO:" + message;
  if (mqtt.connected()) {
    Serial.print(F("MQTT >> ")); 
    Serial.println(data); 
    mqtt.beginPublish(topic_nfo.c_str(), data.length(), false);
    mqtt.print(data.c_str());
    mqtt.endPublish();
  } else {
    putOutQueue(topic_nfo, data);
  }  
}

void NotifyError(String &message) {
  if (!useMQTT) return;
  String data = "ERR:" + message;
  if (mqtt.connected()) {
    Serial.print(F("MQTT >> ")); 
    Serial.println(data); 
    mqtt.beginPublish(topic_err.c_str(), data.length(), false);
    mqtt.print(data.c_str());
    mqtt.endPublish();
  } else {
    putOutQueue(topic_err, data);
  }  
}

void notifyUnknownCommand(const char* text) {
  if (!useMQTT) return;
  DynamicJsonDocument doc(512);
  String out;
  doc["message"] = F("unknown command");
  doc["text"]    = String(F("неизвестная команда '")) + String(text) + String(F("'  "));
  serializeJson(doc, out);      
  NotifyError(out);
}

void NotifyAck() {
  if (!useMQTT) return;
  String data = F("ACK");
  if (mqtt.connected()) {    
    Serial.print(F("MQTT >> ")); 
    Serial.println(data); 
    mqtt.publish(topic_ack.c_str(), data.c_str());
  } else {
    putOutQueue(topic_ack, data);
  }  
}

void putOutQueue(String &topic, String &message) {
  if (!useMQTT) return;
  if (outQueueLength < QSIZE) {
    outQueueLength++;
    tpcQueue[outQueueWriteIdx] = topic;      
    outQueue[outQueueWriteIdx] = message;      
    outQueueWriteIdx++;
    if (outQueueWriteIdx >= QSIZE) outQueueWriteIdx = 0;
  }
}

void processOutQueue() {
  if (!useMQTT) {
    outQueueReadIdx = 0;
    outQueueWriteIdx = 0;
    outQueueLength = 0;
    return;
  }
  if (mqtt.connected() && outQueueLength > 0) {    
    String topic = tpcQueue[outQueueReadIdx];
    String message = outQueue[outQueueReadIdx];
    outQueueReadIdx++;
    if (outQueueReadIdx >= QSIZE) outQueueReadIdx = 0;
    outQueueLength--;
    Serial.print(F("MQTT >> ")); 
    Serial.println(message); 
    mqtt.beginPublish(topic.c_str(), message.length(), false);
    mqtt.print(message.c_str());
    mqtt.endPublish();
  }  
}
