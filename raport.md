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

<a name="basic-info"></a>
## Podstawowe informacje

**Autor**: Patryk Mleczek

**Temat**: 17 - Egzamin wstępny

<a name="environment"></a>
## Środowisko

<a name="dev-env"></a>
### Środowisko developmentu

**System operacyjny**: macOS 26

**Zarządzanie kompilacją**: CMake

**Kompilator**: Apple clang version 17.0.0 (clang-1700.6.3.2)

### Środowisko testowe

**System operacyjny**: Debian GNU/Linux 11 (bullseye) (Torus)

**Zarządzanie kompilacją**: CMake

**Kompilator**: gcc version 8.5.0 (GCC) (Torus)

<a name="running"></a>
## Uruchamianie symulacji

Uruchamianie bez kompilacji:

```
./dean <liczba miejsc> <godzina rozpoczęcia>
```

Kompilacja + uruchomienie:

```
./scripts/build_and_run.sh <liczba miejsc> <godzina rozpoczęcia>
```

<a name="arg-params"></a>
### Parametry przekazywane poprzez arguementy

| Parametr | Opis | Format | Akceptowany zakres wartości |
| --- | --- | --- | --- |
| Liczba miejsc | Całkowita liczba miejsc dostępnych na kierunku | Liczba całkowita | 1 ≤ x ≤ `MAX_PROC_COUNT`<sup>1</sup> |
| Czas rozpoczęcia egzaminu | Czas rozpoczęcia egzaminu (symulacji) | Ciąg znaków o formacie `HH:MM` (`H` - godzina; `M` - minuta) | Co najmniej aktualny czas (z dokładnością do godziny i minuty), co najwyzej `24:00` |

**Adnotacje:**

<sup>1</sup> `MAX_PROC_COUNT` = `dozwolona liczba procesów w systemie` * `0.9` / `10.5`

<a name="random-params"></a>
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

<a name="arch"></a>
## Architektura

<a name="proc-types"></a>
### Typy procesów

Symulacja reprezentowana jest poprzez trzy typy procesów:

- Dziekan (`dean`) - proces reprezentujący dziekana
- Kandydat (`candidate`) - proces reprezentujący pojedynczego kandydata
- Komisja (`commission`) - proces reprezentujący komisję

<a name="dean-type"></a>
### Dziekan (`dean`)

Główny proces symulacji - jeden na całą symulację. Uruchamiany jako pierwszy proces (uzywany do uruchamiania całej symulacji) oraz kończy wykonywanie jako ostatni spośród wszystkich procesów. Odpowiedzialny za:

- Spawnowanie pozostałych procesów
- Inicjalizację mechanizmów IPC oraz ich stanu
- Destrukcję mechanizmów IPC
- Weryfikację mozliwosci przystąpienia do egzaminu przez kandydatów
- Rozpoczyna egzamin o określonej godzinie
- Obsługę sygnału ewakuacji i przekazywanie sygnału do kandydatów oraz komisji
- Publikację wyników (listy rankingowej) po zakończeniu/przerwaniu egzaminu

<a name="cand-type"></a>
### Kandydat (`candidate`)

Reprezentuje pojedynczego kandydata. Tworzone przed rozpoczęciem egzaminu przez proces dziekana. Kandydaci zajmują miejsca w komisjach oraz odpowiadają na pytania, a następnie otrzymują oceny od komisji.

<a name="comm-type"></a>
### Komisja (`commission`)

Reprezentuje pojedynczą komisję (`A` lub `B`) - dwa procesy na całą symulacje. Tworzone przed rozpoczęciem egzaminu przez proces dziekana. Komisja składa się odpowiednio z 3 lub 5 wątków w zależności od typu (`A` lub `B`) odpowiedzialnych za tworzenie pytań dla kandydatów w ciągu kilku sekund (losowa liczba czasu z zakresu `2` do `5` sekund). Komisja odpowiada również za ocenianie odpowiedzi kandydatów, oceny przekazywane są do dziekana przez przewodniczącego komisji.

<a name="arch-overview"></a>
### Zarys architektury

### Przebieg symulacji*

1. Symulacja startowana jest poprzez uruchomienie procesu dziekana z dwoma parametrami startowymi: liczbą miejsc oraz godziną rozpoczęcia
2. Proces dziekana weryfikuje przekazane parametry, generuje parametry wyznaczane losowo oraz rejestru odpowiednie handlery sygnałów (w tym sygnały o ewakuacji)
3. Proces dziekana spawnuje pozostałe procesy: kandydatów oraz komisje
4. Następuje rozpoczęcie egzaminu oraz weryfikacja zdania matury
5. Procesy kandydatów, któzy nie zdali matury otrzymują odpowiedni syngał i są kończone
6. Komisje tworzą odpowiednio 5 lub 3 wątki i rozpoczynają główną pętle generującą pytania oraz oceniającą odpowiedzi
7. Kandydaci poprzez semafory zajmują miejsca w komisji A, otrzymują pytania, odpowiadają na nie i są oceniani
    1. Kandydaci którzy podchodzą do egzaminu ponownie nie podchodzą do komisji A
8. Jeżeli kandydat nie otrzymał min. 30% (średnia z 5 ocen członków komisji) opuszcza egzamin (proces jest kończony)
9. Kandydaci przechodzą do komisji B gdzie analogicznie są oceniani przez jej członków
10. Kandydaci kończą swoje procesy
11. Wraz z obsłużeniem wszystkich kandydatów komisje kończą działanie
12. Po zakończenu działania procesów komisji dziekan publikuje liste rankingową oraz kończy swoje działanie

**\*** - w momencie otrzymania sygnału o ewakuacji wszystkie procesy przerywają swoje wykonywanie

### Dodatkowe informacje

- Logi z działania programu zapisywane są do pliku `simulation.log` (synchronizacja dostępu poprzez semafor)
- Błędy w procesach "dzieciach" dziekana są propagowwane do dziekana, a następnie do wszystkich dzieci, aby w razie krytycznych błędów przerywana była cała symulacja
- Mechanizmy IPC, logowania, itp. są wyabstrahowane do osobnych klas wrapperów - w celu ujednolicenia implementacji

<a name="arch-apis"></a>
### Użyte API oraz funkcje systemowe

### Tworzenie i obsługa plików

- `open()` ([ResultsWriter.cpp:24](https://github.com/pmleczek/so-project/blob/main/src/common/output/ResultsWriter.cpp?plain=1#L24), [Logger.cpp:36](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L36))
- `close()` ([ResultsWriter.cpp:54](https://github.com/pmleczek/so-project/blob/main/src/common/output/ResultsWriter.cpp?plain=1#L54), [Logger.cpp:54](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L54))
- `write` ([ResultsWriter.cpp:41](https://github.com/pmleczek/so-project/blob/main/src/common/output/ResultsWriter.cpp?plain=1#L41), [Logger.cpp:99](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L99))
- `unlink` ([ResultsWriter.cpp:16](https://github.com/pmleczek/so-project/blob/main/src/common/output/ResultsWriter.cpp?plain=1#L16), [Logger.cpp:126](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L126))

### Tworzenie procesów

- `fork()` ([DeanProcess.cpp:244](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L244), [DeanProcess.cpp:257](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L257), [DeanProcess.cpp:321](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L321))
- `execlp()` ([DeanProcess.cpp:250](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L250), [DeanProcess.cpp:263](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L263), [DeanProcess.cpp:329](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L329))
- `waitpid()` ([DeanProcess.cpp:213](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L213), [DeanProcess.cpp:223](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L223))
- `exit()` ([DeanProcess.cpp:34](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L34), [DeanProcess.cpp:45](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L45), [DeanProcess.cpp:459](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L459), [DeanProcess.cpp:462](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L462), [DeanProcess.cpp:480](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L480), [DeanProcess.cpp:483](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L483), [CommissionProcess.cpp:28](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L28), [CommissionProcess.cpp:39](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L39), [CommissionProcess.cpp:84](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L84), [CommissionProcess.cpp:120](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L120), [CommissionProcess.cpp:253](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L253), [CommissionProcess.cpp:363](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L363), [CandidateProcess.cpp:24](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L24), [CandidateProcess.cpp:35](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L35), [CandidateProcess.cpp:122](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L122), [CandidateProcess.cpp:353](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L353), [CandidateProcess.cpp:393](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L393), [CandidateProcess.cpp:411](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L411), [BaseProcess.cpp:25](https://github.com/pmleczek/so-project/blob/main/src/common/process/BaseProcess.cpp?plain=1#L25), [BaseProcess.cpp:35](https://github.com/pmleczek/so-project/blob/main/src/common/process/BaseProcess.cpp?plain=1#L35), [BaseProcess.cpp:49](https://github.com/pmleczek/so-project/blob/main/src/common/process/BaseProcess.cpp?plain=1#L49))

### Tworzenie i obsługa wątków

- `pthread_create()` ([CommissionProcess.cpp:250](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L250))
- `pthread_join()` ([CommissionProcess.cpp:264](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L264))
- `pthread_exit()` ([CommissionProcess.cpp:315](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L315))
- `pthread_mutex_lock()` ([MutexWrapper.cpp:14](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/MutexWrapper.cpp?plain=1#L14))
- `pthread_mutex_unlock()` ([MutexWrapper.cpp:28](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/MutexWrapper.cpp?plain=1#L28))
- `pthread_mutexattr_init()` ([Memory.cpp:29](https://github.com/pmleczek/so-project/blob/main/src/common/utils/Memory.cpp?plain=1#L29))
- `pthread_mutexattr_setpshared()` ([Memory.cpp:35](https://github.com/pmleczek/so-project/blob/main/src/common/utils/Memory.cpp?plain=1#L35))
- `pthread_mutexattr_destroy()` ([Memory.cpp:37](https://github.com/pmleczek/so-project/blob/main/src/common/utils/Memory.cpp?plain=1#L37), [Memory.cpp:47](https://github.com/pmleczek/so-project/blob/main/src/common/utils/Memory.cpp?plain=1#L47), [Memory.cpp:58](https://github.com/pmleczek/so-project/blob/main/src/common/utils/Memory.cpp?plain=1#L58))
- `pthread_mutex_init()` ([Memory.cpp:45](https://github.com/pmleczek/so-project/blob/main/src/common/utils/Memory.cpp?plain=1#L45))

### Obsługa sygnałów

- `signal()` ([BaseProcess.cpp:40](https://github.com/pmleczek/so-project/blob/main/src/common/process/BaseProcess.cpp?plain=1#L40))
- `kill()` ([CommissionProcess.cpp:113](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L113), [CandidateProcess.cpp:115](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L115), [DeanProcess.cpp:394](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L394), [ProcessRegistry.cpp:53](https://github.com/pmleczek/so-project/blob/main/src/common/process/ProcessRegistry.cpp?plain=1#L53), [ProcessRegistry.cpp:57](https://github.com/pmleczek/so-project/blob/main/src/common/process/ProcessRegistry.cpp?plain=1#L57), [ProcessRegistry.cpp:62](https://github.com/pmleczek/so-project/blob/main/src/common/process/ProcessRegistry.cpp?plain=1#L62))

### Synchronizacja procesów (wątków)

Obsługa semaforów realizowana jest poprzez klasę `SemaphoreManager`. Bezpośrednie użycia funkcji związanych z semaforami:

- `sem_open()` ([SemaphoreManager.cpp:15](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L15), [SemaphoreManager.cpp:24](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L24), [SemaphoreManager.cpp:44](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L44))
- `sem_close()` ([SemaphoreManager.cpp:62](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L62))
- `sem_unlink()` ([SemaphoreManager.cpp:19](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L19), [SemaphoreManager.cpp:76](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L76))
- `sem_wait()` ([SemaphoreManager.cpp:89](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L89))
- `sem_post()` ([SemaphoreManager.cpp:102](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SemaphoreManager.cpp?plain=1#L102))
- `ftok()` - ([SharedMemoryManager.cpp:139](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L139))

Użycia klasy `SemaphoreManager`:

- `SemaphoreManager::create()` ([DeanProcess.cpp:129](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L129), [DeanProcess.cpp:130](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L130), [DeanProcess.cpp:131](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L131))
- `SemaphoreManager::open()` ([CommissionProcess.cpp:79](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L79), [CandidateProcess.cpp:164](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L164), [Logger.cpp:82](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L82))
- `SemaphoreManager::close()` ([CommissionProcess.cpp:328](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L328), [CandidateProcess.cpp:177](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L177), [Logger.cpp:46](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L46))
- `SemaphoreManager::unlink()` ([DeanProcess.cpp:432](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L432), [DeanProcess.cpp:433](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L433), [DeanProcess.cpp:434](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L434))
- `SemaphoreManager::wait()` ([CandidateProcess.cpp:168](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L168), [Logger.cpp:89](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L89))
- `SemaphoreManager::post()` ([CommissionProcess.cpp:162](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L162), [CommissionProcess.cpp:216](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L216), [CommissionProcess.cpp:407](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L407), [CandidateProcess.cpp:172](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L172), [Logger.cpp:104](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L104), [Logger.cpp:118](https://github.com/pmleczek/so-project/blob/main/src/common/output/Logger.cpp?plain=1#L118))

### Segmenty pamięci dzielonej

Obsługa pamięci dzielonej realizowana jest poprzez klasę `SharedMemoryManager`. Bezpośrednie użycia funkcji związanych z semaforami:

- `ftok()` - ([SharedMemoryManager.cpp:139](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L139))
- `shmget()` ([SharedMemoryManager.cpp:44](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L44), [SharedMemoryManager.cpp:56](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L56), [SharedMemoryManager.cpp:79](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L79))
- `shmat()` ([SharedMemoryManager.cpp:61](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L61), [SharedMemoryManager.cpp:85](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L85))
- `shmdt()` ([SharedMemoryManager.cpp:22](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L22), [SharedMemoryManager.cpp:101](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L101))
- `shmctl()` ([SharedMemoryManager.cpp:47](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L47), [SharedMemoryManager.cpp:119](https://github.com/pmleczek/so-project/blob/main/src/common/ipc/SharedMemoryManager.cpp?plain=1#L119))

Użycia klasy `SharedMemoryManager`:

- `SharedMemoryManager::initialize()` ([DeanProcess.cpp:118](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L118))
- `SharedMemoryManager::attach()` ([CommissionProcess.cpp:75](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L75), [CandidateProcess.cpp:90](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L90))
- `SharedMemoryManager::detach()` ([CommissionProcess.cpp:327](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp?plain=1#L327), [CandidateProcess.cpp:366](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp?plain=1#L366))
- `SharedMemoryManager::destroy()` ([DeanProcess.cpp:431](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp?plain=1#L431))
- `SharedMemoryManager::data()` - używane w wielu miejscach do dostępu do danych w pamięci współdzielonej ([CommissionProcess.cpp](https://github.com/pmleczek/so-project/blob/main/src/commission/CommissionProcess.cpp), [CandidateProcess.cpp](https://github.com/pmleczek/so-project/blob/main/src/candidate/CandidateProcess.cpp), [DeanProcess.cpp](https://github.com/pmleczek/so-project/blob/main/src/dean/DeanProcess.cpp), [ProcessRegistry.cpp](https://github.com/pmleczek/so-project/blob/main/src/common/process/ProcessRegistry.cpp), [Memory.cpp](https://github.com/pmleczek/so-project/blob/main/src/common/utils/Memory.cpp), [ResultsWriter.cpp](https://github.com/pmleczek/so-project/blob/main/src/common/output/ResultsWriter.cpp))

<a name="tests"></a>
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
[INFO] [2026-01-17 20:06:34.290] DeanProcess::evacuationHandler()
[INFO] [2026-01-17 20:06:34.292] Evacuation signal received
[INFO] [2026-01-17 20:06:34.293] Propagating signal: 15 to all processes

...

[INFO] [2026-01-17 20:06:34.294] [Commission (type=A)] CommissionProcess::terminationHandler()
```

Lista rankingowa zawiera informacje o ewakuacji i dzieli kandydatów na tych którym udało się przejść przez obie komisje i tych którym nie:

```
| ==== Lista Rankingowa (Ewakuacja) ==== |
| PID | Numer | Matura | Cz. teoretyczna | Cz. praktyczna | Ostateczny wynik |
|-----|-------|--------|----------------|----------------|----------------|
 === Ukonczyli egzamin ===
| 7211 | 2 | TAK | 65.283215 | 67.841817 | 66.562516 |
| 7212 | 3 | TAK | 48.068178 | 81.922641 | 64.995409 |
| 7222 | 13 | TAK | 60.383721 | 57.336182 | 58.859952 |
| 7226 | 17 | TAK | 60.365415 | 56.085368 | 58.225392 |

...

 === Nie ukończyli egzaminu ===
| 7210 | 1 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |
| 7215 | 6 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |
| 7216 | 7 | TAK | 29.810387 | 0.000000 | NIE UKONCZONO |
| 7217 | 8 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |
| 7218 | 9 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |
| 7220 | 11 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |
| 7223 | 14 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |
| 7224 | 15 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |
| 7225 | 16 | TAK | 0.000000 | 0.000000 | NIE UKONCZONO |

...
```
