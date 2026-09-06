# Инструкция и Архитектура проекта `disping`

> **Проект**: `disping` (Ultimate Network & FPS Gaming Optimizer)  
> **Репозиторий**: `https://github.com/winchesterstallone1-alt/disping`  
> **Стек**: C++20, x86-64 Assembly (NASM), Windows Native APIs (Winsock2, IPHlpAPI, WinMM, NTDLL, MMCSS, Advapi32).

---

## 1. Цели и задачи проекта

1. **Максимальное снижение пинга (минимальный пинг и джиттер)**:
   - Полное отключение задержек ACK и алгоритма Нейгла (`TcpAckFrequency = 1`, `TCPNoDelay = 1`, `TcpDelAckTicks = 0`).
   - Отключение сетевого троттлинга Windows (`NetworkThrottlingIndex = 0xffffffff`, `SystemResponsiveness = 0`).
   - Отключение агрегации пакетов (RSC) и прерываний сетевой карты (Interrupt Moderation = Disabled / Low) для мгновенной доставки сетевых пакетов CPU без ожидания пакетных очередей.
   - Динамический поиск оптимального MTU/MSS без фрагментации пакетов с помощью бинарного поиска DF (Don't Fragment).
   - Приоритизация игрового трафика через DSCP/QoS (Expedited Forwarding / DSCP 46).
   - Тестирование и авто-переключение на самый быстрый игровой DNS (Cloudflare, Google, Quad9, AdGuard Gaming).

2. **Оптимизация FPS и системного инпут-лага**:
   - Установка суб-миллисекундного таймера операционной системы (0.500 ms) через нативный вызов `NtSetTimerResolution` из `ntdll.dll`.
   - Настройка планировщика MMCSS (Multimedia Class Scheduler Service) с профилем `Games` (GPU Priority = 8, Priority = 6, High Scheduling Category).
   - Автоматическая привязка игровых процессов и сетевых потоков к высокопроизводительным физическим ядрам CPU (P-cores) без прыжков по ядрам и кешам L3.
   - Очистка Standby Memory List и Working Sets для предотвращения микро-фризов и статтеров из-за сброса кеша в pagefile.

3. **Низкоуровневые ассемблерные модули (x86-64 NASM)**:
   - `asm_fast_checksum_x64`: быстрый подсчет контрольной суммы RFC 1071 с развернутым 64-битным циклом и сложением с переносом (`adc`).
   - `asm_read_tsc_serialized`: сериализованное чтение счетчика тактов процессора (`cpuid` + `rdtsc`) для замеров задержек с суб-микросекундной точностью без накладных расходов вызовов ядра.
   - `asm_fast_memzero_nt`: нетемпоральное обнуление кольцевых буферов пакетов в обход L1/L2 кеша (`movntdq`).
   - `asm_jitter_accumulate`: ассемблерное вычисление джиттера по спецификации RFC 3550.

4. **Инструменты диагностики и телеметрии**:
   - Микросекундный ICMP/UDP пинг-движок.
   - Расчет среднего, минимального, максимального пинга, джиттера и процента потери пакетов.
   - Отрисовка графиков и гистограмм прямо в консоли (ASCII sparklines).
   - Многоточечный одновременный мониторинг игровых серверов (Valve Frankfurt, Riot Games EU, Cloudflare, Google DNS).

5. **Безопасность и откат (Rollback)**:
   - Автоматическое создание снимка всех изменяемых параметров в `disping_backup.json` и `disping_rollback.bat`.
   - Полное восстановление стандартных заводских настроек Windows в один клик.

---

## 2. Архитектура модулей

| Модуль | Файлы | Назначение |
|---|---|---|
| **ASM Core** | `src/asm/disping_routines.asm`, `include/disping_asm.h` | 64-битные ассемблерные оптимизации подсчета сумм, TSC таймеров, non-temporal очистки памяти |
| **Network Engine** | `src/network_optimizer.cpp`, `include/network_optimizer.hpp` | Твики TCP/IP стека, Nagle, TcpAckFrequency, NetworkThrottlingIndex, netsh |
| **Adapter Engine** | `src/adapter_optimizer.cpp`, `include/adapter_optimizer.hpp` | Настройка свойств сетевого адаптера (Interrupt Moderation, LSO, Flow Control, Buffer Size) |
| **QoS / DSCP** | `src/qos_optimizer.cpp`, `include/qos_optimizer.hpp` | Назначение DSCP 46 (EF) тегов игровым пакетам и конфигурация политик Windows QoS |
| **MTU Finder** | `src/mtu_optimizer.cpp`, `include/mtu_optimizer.hpp` | Бинарный поиск максимального нефрагментируемого MTU и установка оптимального значения |
| **DNS Optimizer** | `src/dns_optimizer.cpp`, `include/dns_optimizer.hpp` | Замер задержек DNS серверов и авто-выбор быстрейшего рендерера |
| **System Latency**| `src/system_latency_optimizer.cpp`, `include/system_latency_optimizer.hpp` | Таймер 0.5 мс (`NtSetTimerResolution`), MMCSS `Games` профиль, системная отзывчивость |
| **Process Affinity**| `src/process_optimizer.cpp`, `include/process_optimizer.hpp` | Оптимизация приоритета и привязка ядер для игр (CS2, Valorant, Dota, etc.) |
| **Memory Cleaner**| `src/memory_optimizer.cpp`, `include/memory_optimizer.hpp` | Очистка Standby List и рабочих наборов памяти |
| **Ping & Jitter** | `src/ping_monitor.cpp`, `include/ping_monitor.hpp` | Микросекундный ICMP/UDP мониторинг пинга и джиттера с визуализацией |
| **UDP Proxy** | `src/udp_proxy.cpp`, `include/udp_proxy.hpp` | Локальный быстрый UDP сокетный туннель с повышенным приоритетом сокетов |
| **Hardware Detector** | `src/hardware_detector.cpp`, `include/hardware_detector.hpp` | Детектирование Intel Hybrid (P/E-ядра), AMD X3D, наборов ISA, RAM и адаптивная маска аффинити |
| **ASM Dispatcher** | `src/disping_asm_dispatch.cpp`, `include/disping_asm.h` | Динамическая диспетчеризация инструкций CPU (AVX2 -> SSE4.2 -> SSE2 fallback) |
| **VPN Guard** | `src/vpn_guard.cpp`, `include/vpn_guard.hpp` | Защита от конфликтов с VPN и DPI обходами (Zapret winws, Incy, Happ, Wintun) |
| **Benchmark Runner** | `src/benchmark_runner.cpp`, `include/benchmark_runner.hpp` | Автоматизированный бенчмарк сравнения реальных характеристик ДО и ПОСЛЕ |
| **Backup & Restore**| `src/backup_manager.cpp`, `include/backup_manager.hpp` | Снятие бэкапа реестра/netsh, экспорт в json/bat, откат к стандарту |
| **UI & CLI** | `src/ui_console.cpp`, `include/ui_console.hpp`, `src/main.cpp` | Консольный интерфейс с ANSI цветами, меню и параметрами командной строки |

---

## 3. Инструкции по сборке и запуску

### Требования
- Windows 10/11 (x64)
- Компилятор MinGW-w64 GCC/G++ (ucrt64) версии 11+ (проверено на GCC 16.1.0)
- Ассемблер NASM (версия 2.15+, проверено на 3.01)
- CMake 3.20+ или `ninja`/`mingw32-make`

### Сборка через CMake & Ninja
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
mkdir build -Force
cd build
cmake -G "Ninja" ..
ninja
```

### Быстрая сборка через PowerShell скрипт
```powershell
.\scripts\build.ps1
```

### Запуск тестов
```powershell
.\scripts\run_tests.ps1
# или
.\build\disping_tests.exe
```

### Использование утилиты
```powershell
# Запуск интерактивного меню (требуются права Администратора для применения твиков)
.\build\disping.exe

# Быстрое применение всех экстремальных оптимизаций сети и FPS
.\build\disping.exe --all

# Только оптимизация сети
.\build\disping.exe --network

# Запуск монитора задержки и джиттера к определенному серверу
.\build\disping.exe --ping 1.1.1.1

# Поиск оптимального MTU
.\build\disping.exe --mtu

# Бенчмарк и авто-выбор DNS
.\build\disping.exe --dns

# Активация таймера 0.5мс в фоновом режиме
.\build\disping.exe --timer

# Восстановление заводских настроек
.\build\disping.exe --restore
```

---

## 4. План выполнения и верификации

1. [x] Проверка окружения, компилятора G++ и NASM.
2. [x] Составление архитектурного плана и файла инструкций (`INSTRUCTIONS.md`).
3. [ ] Написание ассемблерных процедур (`disping_routines.asm`) и заголовочных файлов.
4. [ ] Реализация сетевых и системных оптимизаторов на C++20.
5. [ ] Реализация монитора пинга/джиттера с микросекундной точностью и ASCII-графикой.
6. [ ] Реализация подсистемы бэкапа и безопасного отката.
7. [ ] Реализация интерактивного UI и CLI параметров.
8. [ ] Создание комплексного набора автотестов (`disping_tests.exe`) и запуск верификации.
9. [ ] Оформление документации (`README.md`, `README_RU.md`).
10. [ ] Инициализация git, коммит и публикация в репозиторий `https://github.com/winchesterstallone1-alt/disping`.
