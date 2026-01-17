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
  - [Zarys architektury](#arch-overview)
  - [Użyte API oraz funkcje systemowe](#arch-apis)
- [Testy](#tests)

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

### Środowisko testowe

**System operacyjny**: Debian GNU/Linux 11 (bullseye) (Torus)

**Zarządzanie kompilacją**: CMake

**Kompilator**: gcc version 8.5.0 (GCC) (Torus)

<a name="#running"></a>
## Uruchamianie symulacji

<a name="#arg-params"></a>
### Parametry przekazywane poprzez arguementy

| Parametr | Opis | Format | Akceptowany zakres wartości |
| --- | --- | --- | --- |
| Liczba miejsc | Całkowita liczba miejsc dostępnych na kierunku | Liczba całkowita | 1 ≤ x ≤ `MAX_PROC_COUNT`<sup>1</sup> |
| Czas rozpoczęcia egzaminu | Czas rozpoczęcia egzaminu (symulacji) | Ciąg znaków o formacie `HH:MM` (`H` - godzina; `M` - minuta) | Co najmniej aktualny czas (z dokładnością do godziny i minuty), co najwyzej `24:00` |

**Adnotacje:**

<sup>1</sup> `MAX_PROC_COUNT` = `dozwolona liczba procesów w systemie` * `0.9` / `10.5`

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
| Czas odpowiedzi T<sub>1..5</sub> w komisji A | Wyznaczane losowo aby uniknąć przekazywania dużej liczby argumentów do programu  | 0.25 ≤ T<sub>i</sub> ≤ 1 |
| Czas odpowiedzi T<sub>1..3</sub> w komisji B | Wyznaczane losowo aby uniknąć przekazywania dużej liczby argumentów do programu  | 0.25 ≤ T<sub>i</sub> ≤ 1 |

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
- Obsługę sygnału ewakuacji i przekazywanie sygnału do kandydatów oraz komisji
- Publikację wyników (listy rankingowej) po zakończeniu/przerwaniu egzaminu

<a name="#cand-type"></a>
### Kandydat (`candidate`)

Reprezentuje pojedynczego kandydata. Tworzone przed rozpoczęciem egzaminu przez proces dziekana. Kandydaci zajmują miejsca w komisjach oraz odpowiadają na pytania, a następnie otrzymują oceny od komisji.

<a name="#comm-type"></a>
### Komisja (`commission`)

Reprezentuje pojedynczą komisję (`A` lub `B`) - dwa procesy na całą symulacje. Tworzone przed rozpoczęciem egzaminu przez proces dziekana. Komisja składa się odpowiednio z 3 lub 5 wątków w zależności od typu (`A` lub `B`) odpowiedzialnych za tworzenie pytań dla kandydatów w ciągu kilku sekund (losowa liczba czasu z zakresu `2` do `5` sekund). Komisja odpowiada również za ocenianie odpowiedzi kandydatów, oceny przekazywane są do dziekana przez przewodniczącego komisji.

<a name="#tests"></a>
## Testy

### 0. Podstawowe testy 

**Kroki**

1. Uruchom symulację z odpowiednim zestawem parametrów
2. Zweryfikuj zachowanie symulacji dla danego zestawu parametrów
3. Zweryfikuj poprawne zakończenie symulacji oraz zwolnienie wykorzystanych zasobów

**Parametry**

10, dowolna poprawna godzina startu

**Oczekiwane zachowanie**

Symulacja zachowuje się zgodnie z opisem, przekazywane parametry są weryfikowane, a losowe poprawnie generowane. Symulacja nie pozostawia żadnych procesów "zombie" ani mechanizmów IPC.

### 1. Weryfikacja dopuszczenia do egzaminu

**Kroki**

1. Uruchom symulację z odpowiednim zestawem parametrów
2. Zweryfikuj listę rankingową oraz logi wg. poniższego opisu

**Parametry**

10, dowolna poprawna godzina startu

**Oczekiwane zachowanie**

Dziekan powinien poprawnie weryfikować możliwość podejścia do egzaminu przez kandydatów, tj. powinien sprawdzać czy każdy z kandydatów posiada zdaną maturę.

W logach znajduje się informacja o odrzuceniu kandydata po rozpoczęciu egzaminu, kandydat kończy działanie, np.:

```sh
[INFO] [2026-01-11 23:14:09.913] Exam has started
[INFO] [2026-01-11 23:14:09.916] [Candidate (id=27)] CandidateProcess::rejectionHandler()
[INFO] [2026-01-11 23:14:09.917] [Candidate (id=27)] Candidate process with pid 2236529 exiting with status 0
```

Dodatkowo w liście rankingowej znajduje się informacja o braku matury i niedopuszczeniu kandydata do egzaminu (`-1` jako wynik z cz. teoretycznej):

```
| ==== Lista Rankingowa ==== |
| PID | Numer | Matura | Cz. teoretyczna | Cz. praktyczna | Ostateczny wynik |
|-----|-------|--------|----------------|----------------|----------------|

...

| 2236529 | 27 | NIE | -1.000000 | 0.000000 | 0.000000 |
```

### 2. Dopuszczenie do części praktycznej

**Kroki**

1. Uruchom symulację z odpowiednim zestawem parametrów
2. Zweryfikuj listę rankingową oraz logi wg. poniższego opisu

**Parametry**

10, dowolna poprawna godzina startu

**Oczekiwane zachowanie**

W logach powinny znajdować się informacje o tym że kandydaci nie zdali cz. teoretycznej i opuszczają egzamin:

```sh
[INFO] [2026-01-11 23:42:29.937] [Candidate (id=48)] Candidate process with pid 2250660 failed to pass the exam
[INFO] [2026-01-11 23:42:29.938] [Candidate (id=48)] CandidateProcess::cleanup()
[INFO] [2026-01-11 23:42:29.939] [Candidate (id=48)] SharedMemoryManager::detach()
[INFO] [2026-01-11 23:42:29.941] [Candidate (id=48)] Detaching from shared memory
[INFO] [2026-01-11 23:42:29.942] [Candidate (id=48)] Candidate process with pid 2250660 exiting with status 0
```

Dodatkowo na liście rankingowej kandydaci którzy nie otrzymali przynajmniej `30%` z cz. teoretycznej powinni mieć wynik `0` z cz. praktycznej oraz z całego egzaminu:

```
| ==== Lista Rankingowa ==== |
| PID | Numer | Matura | Cz. teoretyczna | Cz. praktyczna | Ostateczny wynik |
|-----|-------|--------|----------------|----------------|----------------|

...

| 2236594 | 89 | TAK | 28.315670 | 0.000000 | 0.000000 |
| 2236596 | 91 | TAK | 19.934314 | 0.000000 | 0.000000 |
| 2236569 | 66 | TAK | 27.934578 | 0.000000 | 0.000000 |
| 2236590 | 86 | TAK | 27.908569 | 0.000000 | 0.000000 |
| 2236506 | 5 | TAK | 18.617539 | 0.000000 | 0.000000 |
| 2236555 | 52 | TAK | 19.745578 | 0.000000 | 0.000000 |
| 2236602 | 97 | TAK | 28.092428 | 0.000000 | 0.000000 |
| 2236587 | 83 | TAK | 21.945655 | 0.000000 | 0.000000 |
| 2236529 | 27 | NIE | -1.000000 | 0.000000 | 0.000000 |
| 2236551 | 48 | TAK | 28.575112 | 0.000000 | 0.000000 |
```

### 3. Pominięcie części teoretycznej

**Kroki**

1. Uruchom symulację z odpowiednim zestawem parametrów
2. Zweryfikuj listę rankingową oraz logi wg. poniższego opisu

**Parametry**

10, dowolna poprawna godzina startu

**Oczekiwane zachowanie**

Kandydaci którzy zdali poprzednio cz. teoretyczną nie podchodzą do komisji A, co jest uwzględnione w logach:

```sh
[INFO] [2026-01-11 23:50:00.294] [Candidate (id=79)] Candidate is retaking the exam, skipping commission A
[INFO] [2026-01-11 23:50:00.296] [Candidate (id=79)] CandidateProcess::waitForQuestions()
[INFO] [2026-01-11 23:50:00.297] [Candidate (id=79)] Candidate process with pid 2253982 waiting for questions from commission B
```

Dodatkowo w liście rankingowej kandydaci podchodzący ponownie powinni posiadać poprawny większy lub równy `30%` wynik z cz. teoretycznej:

```
| ==== Lista Rankingowa ==== |
| PID | Numer | Matura | Cz. teoretyczna | Cz. praktyczna | Ostateczny wynik |
|-----|-------|--------|----------------|----------------|----------------|

...

| 2253982 | 79 | TAK | 40.991052 | 43.439231 | 42.215142 |
```

### 4. Ocena odpowiedzi przez komisję

**Kroki**

1. Uruchom symulację z odpowiednim zestawem parametrów
2. Zweryfikuj listę rankingową oraz logi wg. poniższego opisu

**Parametry**

10, dowolna poprawna godzina startu

**Oczekiwane zachowanie**

W logach znajdują się informacje o zwiększającej się liczbie ocenionych kandydatów (zarówno dla komisji A jak i B):

```
[INFO] [2026-01-11 23:51:49.795] [Commission (type=A)] % of candidates graded: 37.623762%
[INFO] [2026-01-11 23:51:51.798] [Commission (type=A)] % of candidates graded: 38.613861%

...

[INFO] [2026-01-11 23:51:54.794] [Commission (type=B)] % of candidates graded: 36.633663%
[INFO] [2026-01-11 23:51:55.796] [Commission (type=B)] % of candidates graded: 37.623762%
```

Dodatkowo w liście rankingowej powinny znajdować się oceny kandydatów:

```
| ==== Lista Rankingowa ==== |
| PID | Numer | Matura | Cz. teoretyczna | Cz. praktyczna | Ostateczny wynik |
|-----|-------|--------|----------------|----------------|----------------|
| 2253997 | 94 | TAK | 68.879181 | 88.706723 | 78.792952 |
| 2253991 | 88 | TAK | 55.758786 | 94.541620 | 75.150203 |
| 2253932 | 29 | TAK | 76.930281 | 67.848525 | 72.389403 |
| 2253999 | 96 | TAK | 73.369928 | 69.452912 | 71.411420 |
| 2253910 | 7 | TAK | 62.004553 | 80.616040 | 71.310297 |
| 2254002 | 99 | TAK | 52.549674 | 83.475889 | 68.012781 |
| 2253960 | 57 | TAK | 64.074583 | 71.724840 | 67.899712 |
```

### 5. Ewakuacja budynku

**Kroki**

1. Uruchom symulację z odpowiednim zestawem parametrów
2. W trakcie działania symulacji wyślij do procesu dziekana sygnał ewakuacji
3. Zweryfikuj listę rankingową oraz logi wg. poniższego opisu

**Parametry**

10, dowolna poprawna godzina startu

**Oczekiwane zachowanie**

W logach pojawia się informacja o ewakuacji oraz o tym że wszystkie procesy są kończone:

```
[INFO] [2026-01-17 19:17:30.449] DeanProcess::evacuationHandler()

...


```
