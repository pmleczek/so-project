# Raport Końcowy

## Spis treści

- [Podstawowe informacje](#basic-info)
- [Środowisko](#environment)
  - [Środowisko developmentu](#dev-env)
- [Uruchamianie](#running)
  - [Parametry przekazywane poprzez arguementy](#arg-params)
  - [Parametry wyznaczane losowo](#random-params)
- [Architektura](#arch)
  - [Typy procesów](#proc-types)
    - [Dziekan](#dean-type)
    - [Kandydat](#cand-type)
    - [Komisja](#comm-type)

<a name="#basic-info"></a>
## Podstawowe informacje

**Autor**: Patryk Mleczek

**Temat**: 17 - Egzamin wstępny

<a name="#environment"></a>
## Środowisko

<a name="#dev-env"></a>
### Środowisko developmentu

**System operacyjny**: macOS 26

**Zarządzanie kompilacją**: CMake

**Kompilator**: Apple clang version 17.0.0 (clang-1700.6.3.2)

<a name="#running"></a>
## Uruchamianie symulacji

<a name="#arg-params"></a>
### Parametry przekazywane poprzez arguementy

| Parametr | Opis | Format | Akceptowany zakres wartości |
| --- | --- | --- | --- |
| Liczba miejsc | Całkowita liczba miejsc dostępnych na kierunku | Liczba całkowita | 1 ≤ x ≤ TODO |
| Czas rozpoczęcia egzaminu | Czas rozpoczęcia egzaminu (symulacji) | Ciąg znaków o formacie `HH:MM` (`H` - godzina; `M` - minuta) | Co najmniej aktualny czas (z dokładnością do godziny i minuty), co najwyzej `24:00` |

<a name="#random-params"></a>
### Parmaetry wyznaczane losowo

Część parametrów jest wyznaczana losowo, ponieważ niekoniecznie miałoby sens, aby były przekazywane w trakcie uruchamiania symulacji:

| Parametr | Opis | Zakres wartości |
| --- | --- | --- |
| Liczba kandydatów na jedno miejsce | Liczba kandydatów przypadających na jedno dostępne miejsce | 9.5 ≤ x ≤ 10.5  |
| Liczba kandydatów | Całkowita liczba kandydatów przystępujących do egzaminu. Obliczana w oparciu o przekazaną jako argument liczbę miejsc oraz powyzszy współczynnik liczby osób na miejsce  | - |
| % kandydatów niezdających matury | Część kandydatów, która nie zdała matury | 1.5% ≤ x ≤ 2.5% |
| Liczba kandydatów niezdających matury | Liczba kandydatów przystępujących do egzaminu, którzy nie zdali matury. Obliczana w oparciu o przekazaną jako argument liczbę miejsc oraz powyzszy współczynnik niezdających | - |
| % kandydatów powtarzających egzamin | Część kandydatów podchodząca do egzaminu ponownie (mają zdaną część teoretyczną) | 1.5% ≤ x ≤ 2.5% |
| Liczba kandydatów powtarzających egzamin | Liczba kandydatów przystępujących do egzaminu, którzy zdali poprzednio część teoretyczną. Obliczana w oparciu o przekazaną jako argument liczbę miejsc oraz powyzszy współczynnik powtarzania | - |

<a name="#arch"></a>
## Architektura

<a name="#proc-types"></a>
### Typy procesów

Symulacja reprezentowana jest poprzez trzy typy procesów:

- Dziekan (`dean`) - proces reprezentujący dziekana
- Kandydat (`candidate`) - proces reprezentujący pojedynczego kandydata
- Komisja (`commission`) - proces reprezentujący komisję

<a name="#dean-type"></a>
### Dziekan (`dean`)

Główny proces symulacji - jeden na całą symulację. Uruchamiany jako pierwszy proces (uzywany do uruchamiania całej symulacji) oraz kończy wykonywanie jako ostatni spośród wszystkich procesów. Odpowiedzialny za:

- Spawnowanie pozostałych procesów
- Inicjalizację mechanizmów IPC oraz ich stanu
- Destrukcję mechanizmów IPC
- Weryfikację mozliwosci przystąpienia do egzaminu przez kandydatów
- Rozpoczyna egzamin o określonej godzinie
- Obsługę sygnału ewakuacji

<a name="#cand-type"></a>
### Kandydat (`candidate`)

Reprezentuje pojedynczego kandydata

<a name="#comm-type"></a>
### Komisja (`commission`)

Reprezentuje pojedynczą komisję (`A` lub `B`) - dwa procesy na całą symulacje. Tworzone przed rozpoczęciem egzaminu przez proces dziekana. Komisja składa się odpowiednio z 3 lub 5 wątków w zależności od typu (`A` lub `B`) odpowiedzialnych za tworzenie pytań dla kandydatów.
