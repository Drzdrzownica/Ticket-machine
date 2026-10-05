# Program symulujący biletomat i jego serwer
### Wstęp:
Napisanie niniejszego programu zostało mi oryginalnie zadane jako pytanie rekrutacyjne. Nie udało mi się go wtedy skończyć w terminie oraz nie byłem zadowolony z tego, co udało mi się napisać.
Program widoczny w tym repozytorium to to, co udało mi się osiągnąć bez presji czasu. Nie jest on jeszcze skończony, jako że używam go do nauki, zamierzam go poszerzyć, lecz spełnia on już parametry oryginalnie zadanego mi zadania.

### Opis:
Program symuluje biletomat przyjmujący gotówkę i drukujący bilety. W przypadku wprowadzenia zbyt dużej ilości gotówki wydaje resztę w możliwie najmniejszej ilości monet/banknotów.
Nie działa on bez połączenia z serwerem, który to zatwierdza lub odmawia transakcji, oraz decyduje jakie bilety są dostępne.
Wkładanie monet do maszyny symulowane jest poprzez wpisywanie nominału w centach. Wybrałem dolary jako walutę, ponieważ projekt wykonany jest w języku angielskim.

### Kompilacja
Upewnij się że masz zainstlowane QT oraz CMake.

#### Linux:
Wprowadź następujące komendy do konsoli w folderze projektu:

```
cmake -S . -B build
cmake --build build --config Release
```
Programy znajdują się w folderze 'build'

#### Windows:
Wprowadź następujące komendy do konsoli w folderze projektu. <br>
<b>Zastąp ścieżkę w pierwszek komendzie lokacją twojej instalacji QT</b>
```
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/qt/6.8.3/msvc2022_64
cmake --build build --config Release
cmake --install build --config Release
```
Programy znajdują się w folderze dist/bin

### Wykonane części:
- UI.
- Komunikacja pomiędzy klientem i serwerem.
- W pełni funkcjonalny proces transakcji zarówno ze strony serwera, jak i clienta.
- Maszyny logują całą komunikację, logowanie czułych danych wymaga definicję odpowiedniego Macro, które nie może zostać zdefiniowane w wersji release.
- Server pre-rezerwuje jedną sztukę biletu podczas transakcji, by inny użytkownik nie mógł go kupić, gdy inny jest w trakcie.
- Client prawidłowo wylicza i wydaje resztę.

### Do zrobiena:
- Timeout pre-rezerwacji, gdy użytkownik spędza za dużo czasu na transakcje.
- Podłączenie serwera do bazy danych by sprzedaże mogły być zarejestrowane.
- Podłączenie klienta (klientów) do baz danych i rejestracja ilości nominałów posiadanych przez każdą maszynę.
- Przełączenie/opcja przełączenia logów z konsoli do odpowiednich plików.
- Lokalizacja na więcej języków (choć tego nie będzie mi się pewnie chciało)

### Disclaimer:
Jako że program ten ma służyć jako dowód, że potrafię programować i znam oraz rozumiem zagadnienia zawarte w kodzie, napisałem go oczywiście ja osobiście, to jest, nie został sporządzony przez AI. 
To powiedziawszy, używałem AI do zalezienia literówek i błędów w kodzie oraz jako pomoc w znalezieniu konkretnych informacji w dokumentacji QT. Poza tym CMakeLists.txt zostało napisane mocno polegając na AI. Chciałbym umieć pisać je sam, lecz jest to coś, co robi się tak rzadko, że ciężko jest, by wiedza się ostała.
