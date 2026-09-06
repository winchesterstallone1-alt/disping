# ⚡ DisPing — Ultimate Gaming Network & Latency Optimization Engine

<p align="center">
  <b>Высокопроизводительный низкоуровневый оптимизатор сети, пинга и FPS для соревновательных игр</b><br>
  <i>x86-64 NASM Assembly • C++20 • Windows NT Kernel APIs • Sub-millisecond Timers</i>
</p>

---

## 🎯 О проекте

**DisPing** — это специализированный системный комплекс для Windows 10/11, спроектированный для достижения **минимально возможного инпут-лага, нулевого джиттера (packet jitter) и минимального отрицательного пинга** в сетевых играх (Counter-Strike 2, Valorant, Dota 2, Apex Legends, Rust, Warzone, Tarkov и др.).

Проект объединяет ассемблерные микрооптимизации критического пути, прямое взаимодействие с ядром Windows через недокументированные NT API (`ntdll.dll`), глубокую перенастройку стека TCP/IP, принудительное отключение троттлинга мультимедиа, разблокировку системного таймера до 0.500 мс (2000 Гц), аппаратный тюнинг сетевых плат, приоритизацию трафика QoS DSCP 46 (Expedited Forwarding) и очистку Standby-кэша оперативной памяти для ликвидации 1% / 0.1% Low FPS статтеров.

---

## 🚀 Ключевые возможности

### 1. ⚡ Ассемблерное ускорение (x86_64 NASM ASM)
- **`asm_fast_checksum_x64`**: Расчёт контрольных сумм сетевых пакетов (RFC 1071) с 64-байтным разворачиванием цикла и сложением с переносом (`adc`).
- **`asm_read_tsc_serialized`**: Чтение аппаратных меток времени CPU (`rdtscp` / `cpuid` + `rdtsc`) для замеров задержек с субмикросекундной точностью без влияния спекулятивного выполнения команд процессора.
- **`asm_calc_jitter_rfc3550`**: Аппаратный SIMD SSE2 расчёт сетевого джиттера по спецификации RFC 3550:
  $$J(i) = J(i-1) + \frac{|D(i-1, i)| - J(i-1)}{16}$$
- **`asm_fast_memzero_nt`**: Нетемпоральная очистка кольцевых сетевых буферов (`movntdq` + `sfence`), исключающая загрязнение кэша L1/L2 процессора пересылаемыми пакетами.

### 2. 🌐 Оптимизация сетевого стека TCP/IP
- **Отключение алгоритма Нагла (`TCPNoDelay = 1`)**: Пакеты отправляются немедленно без ожидания накопления данных в буфере сокета.
- **Отключение задержки подтверждений (`TcpAckFrequency = 1`, `TcpDelAckTicks = 0`)**: Windows подтверждает каждый принятый сегмент мгновенно, ликвидируя искусственную задержку в 100-200 мс.
- **Отключение сетевого троттлинга (`NetworkThrottlingIndex = 0xFFFFFFFF`)**: Снятие встроенного в Windows ограничения на обработку максимум 10 000 пакетов в секунду при воспроизведении звука/видео.
- **System Responsiveness (`SystemResponsiveness = 0`)**: Резервирование 100% ресурсов планировщика сети под игровое приложение вместо фоновых служб Windows.
- **Снятие лимитов соединений**: `MaxUserPort = 65534`, сокращение `TcpTimedWaitDelay` до 30 секунд.
- **Аппаратные разгрузки (Offloads)**: Настройка RSS (Receive Side Scaling), отключение RSC (Receive Segment Coalescing, дробящего игровые пакеты на пачки).

### 3. ⏱ Сверхвысокое разрешение системного таймера (0.500 мс)
- По умолчанию Windows работает с тикрейтом таймера 15.625 мс (64 Гц), из-за чего события приёма пакетов и отрисовки кадров задерживаются.
- DisPing использует прямое системное обращение `NtSetTimerResolution(5000, TRUE)` для перевода системных прерываний на частоту **2000 Гц (0.500 мс)**, что снижает инпут-лаг мыши, повышает плавность кадров и устраняет микрозадержки приёма UDP/TCP дейтаграмм.

### 4. 🎛 Аппаратный тюнинг сетевых адаптеров (NIC)
- **Отключение Interrupt Moderation**: Сетевая карта прерывает ядро процессора немедленно при поступлении пакета, без накопления пакетов в очереди адаптера.
- **Отключение Large Send Offload (LSOv2)**: Исключает фрагментацию и задержки пакетов на аппаратном уровне контроллера.
- **Максимизация дескрипторов колец (`Rx/Tx Descriptors = 2048`)**: Защита от потери пакетов при резких сетевых всплесках в мультиплеере.
- **Отключение энергосбережения сетевого чипа (Green Ethernet, EEE)**: Запрет на засыпание PHY-трансивера сетевой карты.
- **Отключение Flow Control (802.3x)**: Запрет управляющих Pause-фреймов, вызывающих спайки задержки.

### 5. 🏷 Политики QoS DSCP 46 (Expedited Forwarding)
- Активация `DisableUserTOSSetting = 0` и снятие блокировок NLA.
- Автоматическая маркировка пакетов игровых процессов тегом **DSCP 46 (0x2E, TOS 0xB8 - Expedited Forwarding)**.
- Домашние и магистральные роутеры обрабатывают данный трафик в приоритетной очереди с минимальной задержкой.

### 6. 🔍 Бинарный поиск оптимального MTU (Don't Fragment)
- Прощупывание сетевого маршрута ICMP-дейтаграммами с флагом `DF` (Don't Fragment) для нахождения точного размера нефрагментируемого пакета (MTU/MSS).
- Исключает фрагментацию пакетов на шлюзах провайдера.

### 7. 🚀 Бенчмарк DNS и автовыбор лучшего резолвера
- Замер прямого сокетного Round-Trip Time к ведущим игровым резолверам (Cloudflare Gaming, Google DNS, Quad9, AdGuard Gaming, OpenDNS).
- Автоматическая установка быстрейшего DNS на активный сетевой адаптер и очистка кэша Windows (`ipconfig /flushdns`).

### 8. 🧠 Буст процессов и изоляция ядер (P-Core Pinning)
- Автоматическое обнаружение запущенных игр (`cs2.exe`, `valorant.exe`, `dota2.exe`, `r5apex.exe` и др.).
- Повышение класса приоритета до `HIGH_PRIORITY_CLASS`.
- Привязка процесса к физическим производительным ядрам (P-Cores) с изоляцией Ядра 0 (Core 0), на котором Windows обрабатывает аппаратные прерывания DPC/ISR, защищая игру от лагов планировщика.

### 9. 🧹 Очистка Standby List и рабочих наборов памяти
- Принудительный сброс файлового кэша Windows Standby List через `NtSetSystemInformation(SystemMemoryListInformation)`.
- Предотвращает внезапный сброс страниц кэша в файл подкачки во время активного матча, спасая от просадок 1% и 0.1% FPS.

### 10. 🛡 Безопасность и откат изменений (1-Click Rollback)
- Создание резервной копии параметров в `disping_backup.json`.
- Автоматическая генерация автономного скрипта восстановления `disping_rollback.bat`.
- Встроенный механизм сброса всех настроек к заводским значениям Windows по одной кнопке `[R]`.

---

## 📂 Архитектура проекта

```
disping/
├── CMakeLists.txt                # Сборочная конфигурация CMake
├── Makefile                      # Альтернативный Makefile для MinGW
├── INSTRUCTIONS.md               # Архитектурный план и документация разработки
├── README.md                     # Документация проекта
├── include/                      # Заголовочные файлы
│   ├── disping_types.hpp         # Общие структуры и типы данных
│   ├── disping_asm.h             # Сигнатуры ассемблерных функций (NASM C ABI)
│   ├── registry_util.hpp         # Безопасная работа с реестром Windows
│   ├── network_optimizer.hpp     # Твики стека TCP/IP, Нагла, троттлинга
│   ├── adapter_optimizer.hpp     # Аппаратные параметры сетевых плат (NIC)
│   ├── qos_optimizer.hpp         # DSCP / TOS тегирование пакетов
│   ├── mtu_optimizer.hpp         # Определение MTU/MSS бинарным поиском
│   ├── dns_optimizer.hpp         # Сокетный бенчмарк и переключатель DNS
│   ├── system_latency_optimizer.hpp # Таймер 0.500 мс (NtSetTimerResolution), MMCSS
│   ├── process_optimizer.hpp     # Приоритеты и аффинити игр (P-Cores)
│   ├── memory_optimizer.hpp      # Очистка Standby-кэша и рабочих наборов RAM
│   ├── ping_monitor.hpp          # Микросекундная телеметрия пинга и RFC3550 джиттера
│   ├── udp_proxy.hpp             # Быстрый zero-loss UDP ретранслятор
│   ├── backup_manager.hpp        # Бэкап и откат до заводских настроек
│   └── ui_console.hpp            # ANSI-интерфейс, таблицы, спарклайны
├── src/                          # Исходный код C++20
│   ├── main.cpp                  # Точка входа, CLI парсер и главное меню
│   ├── registry_util.cpp
│   ├── network_optimizer.cpp
│   ├── adapter_optimizer.cpp
│   ├── qos_optimizer.cpp
│   ├── mtu_optimizer.cpp
│   ├── dns_optimizer.cpp
│   ├── system_latency_optimizer.cpp
│   ├── process_optimizer.cpp
│   ├── memory_optimizer.cpp
│   ├── ping_monitor.cpp
│   ├── udp_proxy.cpp
│   ├── backup_manager.cpp
│   ├── ui_console.cpp
│   └── asm/                      # Ассемблерные модули (NASM x86_64)
│       └── disping_routines.asm  # SIMD / Checksum / TSC / NT-Memzero
├── tests/                        # Набор верификационных тестов
│   └── test_main.cpp             # 8 комплексных тестов ASM и подсистем
└── scripts/                      # Скрипты автоматизации
    ├── build.ps1                 # Однокликовая сборка через CMake + Ninja
    └── run_tests.ps1             # Запуск тестов
```

---

## 🛠 Сборка и компиляция

### Требования
- **ОС**: Windows 10 или Windows 11 (x64)
- **Компилятор**: MinGW-w64 (GCC с поддержкой C++20) или MSVC
- **Ассемблер**: [NASM](https://www.nasm.us/) (3.x+)
- **Система сборки**: [CMake](https://cmake.org/) (3.20+) и [Ninja](https://ninja-build.org/)

### Быстрая сборка (PowerShell)
```powershell
.\scripts\build.ps1
```

Бинарные файлы будут скомпилированы со статической линковкой в папку `build/`:
- `build\disping.exe` — основное приложение
- `build\disping_tests.exe` — набор автоматических тестов

### Запуск тестов
```powershell
.\scripts\run_tests.ps1
```

Результат выполнения тестового пакета:
```
[*] Executing DisPing Test Suite...
========================================
   disping Automated Test Suite         
========================================
[RUNNING] Test_AssemblyChecksum... [PASS]
[RUNNING] Test_AssemblyTsc... [PASS]
[RUNNING] Test_AssemblyJitter... [PASS]
[RUNNING] Test_AssemblyMemzero... [PASS]
[RUNNING] Test_TimerResolutionQuery... [PASS]
[RUNNING] Test_SparklineGeneration... [PASS]
[RUNNING] Test_ProcessAffinityMaskCalculation... [PASS]
[RUNNING] Test_LocalPing... [PASS]
----------------------------------------
Tests Summary: Passed = 8, Failed = 0
----------------------------------------
[OK] All verification tests passed successfully!
```

---

## 💻 Использование

> ⚠️ **Важно**: Для применения сетевых твиков, настройки таймеров и изменения параметров реестра запускайте командную строку или терминал **от имени Администратора (Run as Administrator)**.

### Интерактивный режим
Запустите без параметров:
```powershell
.\build\disping.exe
```
Откроется полноэкранный терминальный интерфейс с выбором действий:
```
  [System Status]
  * Privileges:        [ADMINISTRATOR]
  * OS Timer Rate:     0.500 ms
  * Network Tweaks:    [ACTIVE - MAXIMUM PRIORITY]
  -----------------------------------------------------------------------

  AVAILABLE ACTIONS:

   [!] 1-CLICK EXTREME GAMING BOOST (Network, Adapters, 0.5ms Timer, MMCSS, QoS)
   [1] Apply TCP/IP Network Tweaks (Disable Nagle, ACK delay, Throttling)
   [2] Optimize Network Adapter Hardware (Interrupt Moderation, Buffers, LSO)
   [3] Lock 0.500 ms High-Resolution Timer & Boost MMCSS Gaming Profile
   [4] Detect & Boost Active Games (High Priority & P-Core Affinity Pinning)
   [5] Discover Optimal MTU/MSS (Don't Fragment Binary Search)
   [6] Benchmark & Auto-Select Fastest Gaming DNS Resolver
   [7] Run Microsecond Ping & Jitter Telemetry (Live Graph)
   [8] Clean Standby List & Process Working Sets (Fix Stutters)
   [9] Start Zero-Copy UDP Fast-Relay Proxy
   [B] Create System State Backup
   [R] Restore Original Windows Defaults (Safe Rollback)
   [T] Run Built-in Automated Verification Tests
   [Q] Exit disping
```

### Режим командной строки (CLI)
| Команда | Описание |
|---|---|
| `disping.exe --all` | Применить полный комплекс экстремальной оптимизации в один клик |
| `disping.exe --network` | Применить твики TCP/IP, Нагла и троттлинга |
| `disping.exe --adapter` | Настроить аппаратные параметры сетевых плат |
| `disping.exe --timer` | Запустить фоновый демон фиксации таймера 0.500 мс |
| `disping.exe --ping <ip>` | Микросекундный замер пинга, джиттера RFC 3550 и спарклайн |
| `disping.exe --mtu [ip]` | Определение и установка наилучшего MTU без фрагментации |
| `disping.exe --dns` | Замер задержек DNS и переключение на самый быстрый |
| `disping.exe --games` | Сканирование и буст приоритета/ядер активных игр |
| `disping.exe --clean-mem`| Очистка кэша Standby List и выгрузка памяти процессов |
| `disping.exe --restore` | Полный возврат системы к стандартным параметрам Windows |
| `disping.exe --version` | Сведения о версии и поддержке инструкций процессора |

---

## 🔒 Безопасность и откат

DisPing не устанавливает сомнительных сторонних драйверов уровня ядра, способных вызывать BSOD или бан в античитах (Vanguard, Easy Anti-Cheat, BattlEye, VAC). Все применяемые оптимизации используют легитимные документированные интерфейсы Windows и сертифицированные параметры сетевого стека Microsoft.

В любой момент вы можете вернуть систему в исходное состояние:
1. Выбрав пункт `[R]` в меню DisPing.
2. Запустив `disping.exe --restore`.
3. Или выполнив созданный скрипт `disping_rollback.bat`.

---

## 📄 Лицензия

MIT License. Свободно для личного и коммерческого использования.
Разработано с заботой о соревновательном гейминге и максимальной производительности.
