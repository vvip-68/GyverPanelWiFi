## Схемы устройств для Wemos d1 mini и NodeMCU

### С сенсорной кнопкой
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/scheme.jpg)

### С обычной кнопкой
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/scheme_b.jpg)

### С SD картой
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/scheme_c.jpg)

### С корректировкой питания и отключением матрицы
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/scheme_e.jpg)

### С корректировкой питания
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/scheme_d.jpg)

### С полным набором компонент (отключение матрицы - MOSFET)
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/ESP8266_ALL.jpg)

Для данного варианта схемы ползователь ***RailWar*** (Evgeny) разработал печатную плату.  
PCB Layout смотри в папке **"pcb"** проекта - архив **"lamp8266.zip"**.

![PCB](https://github.com/vvip-68/GyverPanelWiFi/blob/master/pcb/lamp8266.png)

### С полным набором компонент (отключение матрицы - реле)
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/ESP8266_RELAY.jpg)

<br><br>

## Схемы устройств для ESP32

### С полным набором компонент
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/ESP32_ALL.JPG)

### С сенсорной кнопкой и отключением матрицы
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/ESP32_MOSFET.JPG)

### С обычной кнопкой
![SCHEME](https://github.com/vvip-68/GyverPanelWiFi/blob/master/schemes/ESP32_button_noRELAY.jpg)

<br><br>

При повторении устройств следует учитывать особенности подачи питающего напряжения на собранную схему.
От соблюдения правильных параметров питания элементов зависит насколько надежно и бесперебойно будет работать собранное вами устройство.  

```
# Рекомендация по питанию

Наиболее стабильная работа устройства обеспечивается при питании схемы напряжением 4.6-4.9 вольт (не выше!).
При этом корректирующий диод в зазрыв GND питания микроконтроллера ставить не нужно.
Для схем, использующих DFPlayer, питание на него (VCC) в этом случае подавать с общего плюса питания, 
а не с пина 3.3 вольта микроконтроллера, поскольку пин 3.3 вольта не обеспечивает достаточного 
для потребления DFPlayer'ом тока, что приведет к недостаточной громкости воспроизведения, а в худшем 
случае - выходу из строя микросхемы стабилизатора напряжения 3.3 вольта микроконтроллера.

```
Заметки о том, как обеспечивается правильное питание устройства - в [этой](https://github.com/vvip-68/GyverPanelWiFi/wiki/%D0%97%D0%B0%D0%BC%D0%B5%D1%82%D0%BA%D0%B8-%D0%BE-%D0%BF%D0%B8%D1%82%D0%B0%D0%BD%D0%B8%D0%B8) статье.

<br><br>

Благодарность [Дмитрию (7918514)](https://github.com/7918514) за предоставленные варианты схем и проверку их работоспособности.
Его вариант реализации (схемы, фотографии сборки и готового изделия, печатные платы) - все материалы доступны по [ссылке](https://disk.yandex.ru/d/fIo2UEuKpR54hg) в папке "Монстр".