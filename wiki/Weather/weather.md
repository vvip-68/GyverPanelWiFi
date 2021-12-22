# Настройка сервиса погоды
В настоящее время система способна получать сведения о текущей погоде из двух источников: погодного сервиса **Yandex** и погодного сервиса **OpenServiceMap**.
В тех странах, где по решению правительства заблокирован доступ к сервисам **Yandex**, можно настроить получение текущей погоды с **OpenServiceMap**.  

Найдите в файле **a_def_hard.h** блок с вашими настройками для устройства, в нем строки определения - используется ли сервисы погоды вообще:  
```
#define USE_WEATHER 1         // 1 - использовать получение информации о текущей погоде; 0 - не использовать 
```
Если использование сервиса погоды включено, прошивка будет иметь возможность получать погоду и с Yandex и с OpenWeatherMap.   
Какой именно сервис погоды используется в данный момент определяется настройками в программе управления на смартфоне.

## Настройка погоды с Yandex

Перейдите на сайт [Yandex](http://yandex.com/), введте в строке поиска любой текст и нажмите кнопку "Найти"  

![Yandex](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/001.png)

В адресной строке браузера найдите часть URL с кодом вашего города.  
Если этого не произошло и вы видите сообщение, что данный сайт заблокирован - в вашей стране получить сведения о погоде с сервиса Yandex не получится.  
Попробуйте настроить получение данных с **OpenWeatherMap**.  

![Yandex](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/002.png)

Запомните найденный код. После загрузки прошивки в микроконтроллер, подключитесь к устройству со смартфона и
вставьте этот код в поле "Код региона" на странице основных настроек.  

Скачать файл, содержащий список кодов городов можно здесь:  
https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/yandex.ru-yaca-geo.c2n.pdf  
или [здесь](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/towns.xlsx)  

## Настройка погоды с OpenWeatherMap

Перейдите на сайт [OpenWeatherMap](http://openweathermap.org/)  
Слева от строки поиска и кнопки "Search" найдите значок со стрелочкой навигации и нажмите на него  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/004.png)

В адресной строке браузера найдите часть URL с кодом вашего города  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/005.png)

Запомните найденный код. После загрузки прошивки в микроконтроллер, подключитесь к устройству со смартфона и
вставьте этот код в поле "Код региона" на странице основных настроек.  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/006.png)

Убедитесь в мониторе порта, что устройство соединяется с сервисом и получает необходимые данные о текущей погоде.  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/007.png)

Если вместо сообщения об успешном получении данных погоды вы видите ошибку (обычно с кодом 401) и/или сообщение о неверном ключе API, выполните следующие действия:  

- Перейдите по [ссылке](https://home.openweathermap.org/users/sign_in) и войдите в свою учетную запись на cервисе OpenWeatgerMap или
создайте ее, если у вас еще нет учетной записи.  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/008.png)

- Заполните форму, указав в полях свой логин, пароль, оставьте действующий email, согласитесь с условиями и нажмите кнопку "Create Account"  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/009.png)

- Загляните в свой почтовый ящик - вам придет письмо с просьбой подтвердить регистрацию на сервисе OpenServiceMap  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/010.png)

- Нажмите на кнопку "Verify your email". Вам на почту придет следующее письмо, в котором указан созданный для вас ключ API  

![OpenWeatherMap](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Weather/011.png)

- В файле a_def_soft.h в строке 406 найдите следующее:
```
// API-идентификатор сервиса получения погоды  
#define WEATHER_API_KEY "6a4ba421859c9f4166697758b68d889b"
```
и замените ключ на полученный вами в ответном письме. Как правило, ключ активируется в течении 10 минут, реже - до двух часов.  
Дождитесь активации ключа, когда переход по ссылке примера обращения к API из письма в браузере будет возвращать не ошибку, а
JSON с информацией о текущей погоде.  
```
{"coord":{"lon":92.79,"lat":56.01},"weather":[{"id":803,"main":"Clouds","description":"broken clouds","icon":"04d"}],"base":"stations",  
"main":{"temp":-1,"feels_like":-6.3,"temp_min":-1,"temp_max":-1,"pressure":1023,"humidity":80},"visibility":10000,"wind":{"speed":4,"deg":270},  
"clouds":{"all":75},"dt":1604373691,"sys":{"type":1,"id":8957,"country":"RU","sunrise":1604365212,"sunset":1604397872},  
"timezone":25200,"id":1502026,"name":"Krasnoyarsk","cod":200}
```
Загрузите прошивку с новым ключом в устройство. Убедитесь в мониторе порта, что в логах есть информация об успешной загрузке погоды с погодного сервиса.

