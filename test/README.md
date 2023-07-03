This directory is intended for PlatformIO Test Runner and project tests.

Unit Testing is a software testing method by which individual units of
source code, sets of one or more MCU program modules together with associated
control data, usage procedures, and operating procedures, are tested to
determine whether they are fit for use. Unit testing finds problems early
in the development cycle.

More information about PlatformIO Unit Testing:

- https://docs.platformio.org/en/latest/advanced/unit-testing/index.html

// Topics for sending sensor readings
"Readings/Temperature"
Sıcaklığı okumak için bu topic e subscribe olmak gerek.

"Readings/Humidity"
Nemi okumak için bu topic e subscribe olmak gerek.

"Readings/PM25"
PM12 değerini okumak için bu topic e subscribe olmak gerek.

// Topics for receiving commands
"Commands/StopAll"
Değer ne olursa olsun bu topic e gelen her mesajda fan da humidifier da durur. Gerek görülürse değer kontrolü eklenilenilir.

"Commands/StopFan"
Değer ne olursa olsun bu topic e gelen her mesajda fan durur. Gerek görülürse değer kontrolü eklenilenilir.

"Commands/StopHumidifier"
Değer ne olursa olsun bu topic e gelen her mesajda humidifier durur. Gerek görülürse değer kontrolü eklenilenilir.

"Commands/StartAll"
Değer ne olursa olsun bu topic e gelen her mesajda fan da humidifier da önceden belirlenmiş bir hız varsa çalışaya başlar ve modu manuele alır. Gerek görülürse değer kontrolü eklenilenilir.

"Commands/StartFan"
Bu topic e gelen değerin 0-255 arası olması gerekir. Bu değere göre fan hızı ayarlanır ve modu manuele alır. örn: %50 = 128, %100 = 255

"Commands/StartHumidifier"
Bu topic e gelen değerin 0-255 arası olması gerekir. Bu değere göre humidifier şiddeti ayarlanır ve modu manuele alır. örn: %50 = 128, %100 = 255

"Commands/SetMode"
0 - Manuel,
1 - Auto,
2 - Sleep,
3 - Turbo,
4 - EnergySaving
Şeklinde değerler bekler bunlar dışında kalanları manuel kabul eder

"Commands/SetTimerDate"
belirsiz diye beklemede

"Commands/SetTimerDuration"
belirsiz diye beklemede
