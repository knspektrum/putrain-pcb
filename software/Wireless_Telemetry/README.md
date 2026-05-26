# Wireless_Telemetry
Wireless telemetry system with u-blox gps via LoRa 

#UWAGA
Jeżeli program się nie otwiera w Arduino IDLE spróbować otworzyć w Notepad ++ badź innym notataniku

Korzystałam Arduino IDLE
wybór płytki: ESP32S3 Dev Module
Należy sprawdzić jaki com jest czyli jaki port został przydzielony do danego wyjścia 
Ja sprawdzam:
Menadzer urządzeń > (po podłaczeniu jakiegokolowiek kabla) Porty COM i LPT

Adudiono IDLE:
Tools > COM > wybór tego do którego jest podłączona płytka

Gdy chcemy wgrać kod to po wykonaniu powyższych kroków naciskamy strzałkę -> (upload)

Aby odczytać wiadomości należy podłączyć nadajnik do prądu (zasilić go)
Odbiornik do komputera i podłączyć się do np puTTY

Wybór:
Connection type - Serial
Serial line - wybieramy COM ten sam co w IDLE
Speed - 115200
Open

W Kody/Wireless_Telemetry-main    -> dla konkurecji z rozsrawianiem repeaterów wzdłuż toru
receiver.ino - docelowy program do odbierania danych 
repeater.ino - docelowy program do przesyłania danych
sender_2500ms_GPS_UART.ino - program do nadawania telemetrii z przejazdu po UART dostajemy dane od pojazdu szynowego do ramki dodajemy dane z GPS i są one nadawane
sender_test.ino - testowy program z fałszywą ramką jedynie co się zmienia to counter

Jak sprawdzić działanie:
x osoba siedzi na miejscu gdzie docelowo ma być odbierany sygnal z odbiornikiem podłączonym do komputera
NALEŻY upewnić się czy nadajnik umieszczony na lakomotywie (jezeli jest w poblizu ) nie dostaje prądu
wraz z nadajnaikiem fąlszywym idziemy wzdluż toru 
jesetsemy na łączu z osoba z odbornikiem jej zadaniem jest dawanie znac czy counter sie zwieksza jezeli sie nie zwieksza to osoba daje znac
wtedy ktos z nadajnikiem staje w miejscu/przesuwa sie do momentu gdy stracony został zasieg
osoba z repeater (włącza go ) i probuje tak ustawic aby odbieornik zow odbieral gdy odbiera to idziemy dalej
