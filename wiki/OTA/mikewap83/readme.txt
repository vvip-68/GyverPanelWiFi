Добрый вечер, я новичок в Arduino и тоже стокнулся что не хватает памяти для скетча на ESP 32 4MB если включить все, методом проб и ошибок, 
нашел метод как переразметить структуру памяти

Вот инструкция
Открыть файл C:\Users\[ИМЯ пользователя]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\boards.txt

Добавить строки в разделе ## IDE 2.0 Seems to not update the value где идет описание PartitionScheme

esp32.menu.PartitionScheme.default_1_4MB=4MB with spiffs (1.408MB APP/1.152MB SPIFFS)
esp32.menu.PartitionScheme.default_1_4MB.build.partitions=default_1_4MB
esp32.menu.PartitionScheme.default_1_4MB.upload.maximum_size=1441792

Скопировать файл default_1_4MB.csv в папку C:\Users\[ИМЯ пользователя]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\tools\partitions

Перезапустить Arduino и выбрать в настройка размер памяти 4M with spiffs (1.408MB APP/1.152MB SPIFFS)

В Arduino IDE 2.x в меню "Инструменты" не появится новый вариант PartitionScheme, пока мы не удалим скешированную папку приложения, для этого 
необходимо удалить папку C:\Users\[ИМЯ пользователя]\AppData\Roaming\arduino-ide\

После этого получается 1.408MB на скетч, столько же для OTA обновлений и 1.152MB для хранения папки Data, может конечно и меньше нужно, 
знать бы какой размер нужен для хранения папки Data и настроек, так бы может быть и больше под скетч можно было выделить