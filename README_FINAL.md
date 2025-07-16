# OpenTimeInstrument - Полная документация

## Описание проекта

OpenTimeInstrument - это программное обеспечение для работы с PTP (Precision Time Protocol) Time Card. Программа читает временные метки с входов Time Card и выводит измерения TIE (Time Interval Error) в формате, распознаваемом Calnex CAT.

## Структура проекта

```
OpenTimeInstrument/
├── OpenTimeInstrument.c          # Основной исходный код
├── Makefile.txt                  # Оригинальный Makefile
├── README.md                     # Оригинальная документация
├── Аннотация.docx               # Аннотация проекта
│
├── CLI_INTERFACE.c              # Улучшенный CLI интерфейс
├── GUI_INTERFACE.c              # GUI интерфейс
├── Makefile_enhanced            # Улучшенный Makefile
│
├── setup_timecard.sh            # Скрипт настройки Time Card
├── check_system.sh              # Скрипт проверки системы
├── install.sh                   # Скрипт автоматической установки
├── uninstall.sh                 # Скрипт удаления
├── test_interface.sh            # Скрипт тестирования
│
├── README_RU.md                 # Документация на русском
├── ИНСТРУКЦИЯ.md               # Инструкция по использованию
└── README_FINAL.md              # Эта документация
```

## Компоненты системы

### 1. Основная программа
- **OpenTimeInstrument.c** - Основной исходный код
- **OpenTimeInstrument** - Скомпилированная программа

### 2. CLI интерфейс
- **CLI_INTERFACE.c** - Исходный код CLI интерфейса
- **cli_interface** - Скомпилированный CLI интерфейс

**Возможности CLI интерфейса:**
- Интерактивное меню
- Настройка Time Card
- Проверка устройства
- Запуск измерений
- Просмотр возможностей PTP

### 3. GUI интерфейс
- **GUI_INTERFACE.c** - Исходный код GUI интерфейса
- **gui_interface** - Скомпилированный GUI интерфейс

**Возможности GUI интерфейса:**
- Графический интерфейс
- Настройка устройства
- Проверка доступности
- Запуск/остановка измерений
- Просмотр логов

### 4. Скрипты автоматизации

#### setup_timecard.sh
Автоматическая настройка Time Card:
```bash
sudo ./setup_timecard.sh
```

#### check_system.sh
Проверка системы и зависимостей:
```bash
./check_system.sh
```

#### install.sh
Автоматическая установка всех компонентов:
```bash
sudo ./install.sh
```

#### uninstall.sh
Удаление программы:
```bash
sudo ./uninstall.sh
```

#### test_interface.sh
Тестирование всех интерфейсов:
```bash
./test_interface.sh
```

## Установка

### Автоматическая установка (рекомендуется)
```bash
sudo ./install.sh
```

### Ручная установка
```bash
# 1. Установка зависимостей
sudo apt-get update
sudo apt-get install -y build-essential libgtk2.0-dev

# 2. Сборка всех компонентов
make -f Makefile_enhanced all

# 3. Установка прав на выполнение
chmod +x OpenTimeInstrument cli_interface gui_interface
chmod +x setup_timecard.sh check_system.sh install.sh uninstall.sh test_interface.sh
```

## Использование

### 1. Настройка Time Card
```bash
sudo ./setup_timecard.sh
```

### 2. Проверка системы
```bash
./check_system.sh
```

### 3. Основные команды

#### Бесконечное измерение:
```bash
./OpenTimeInstrument -d /dev/ptp1 -e -1
```

#### Измерение N событий:
```bash
./OpenTimeInstrument -d /dev/ptp1 -e 100
```

#### Проверка возможностей устройства:
```bash
./OpenTimeInstrument -d /dev/ptp1 -c
```

#### Получение времени PTP:
```bash
./OpenTimeInstrument -d /dev/ptp1 -g
```

### 4. CLI интерфейс
```bash
./cli_interface
```

### 5. GUI интерфейс
```bash
./gui_interface
```

## Параметры командной строки

- `-d <устройство>` - Указать PTP устройство (по умолчанию: /dev/ptp0)
- `-e <число>` - Количество внешних временных меток для чтения
  - `-1` - бесконечный режим
  - `N` - N измерений
- `-c` - Показать возможности PTP устройства
- `-g` - Получить время PTP
- `-h` - Показать справку

## Примеры использования

### Пример 1: Базовое измерение
```bash
# Настройка Time Card
sudo ./setup_timecard.sh

# Запуск измерений
./OpenTimeInstrument -d /dev/ptp1 -e -1
```

### Пример 2: Использование CLI интерфейса
```bash
./cli_interface
# В интерактивном меню выберите нужную опцию
```

### Пример 3: Использование GUI интерфейса
```bash
./gui_interface
# В GUI:
# 1. Введите устройство (например: /dev/ptp1)
# 2. Нажмите "Проверить" для проверки устройства
# 3. Нажмите "Настроить Time Card" для настройки
# 4. Нажмите "Начать измерение" для запуска
```

### Пример 4: Тестирование системы
```bash
# Проверка системы
./check_system.sh

# Тестирование интерфейсов
./test_interface.sh
```

## Устранение неполадок

### Проблема: Устройство недоступно
```bash
sudo chmod 666 /dev/ptp1
```

### Проблема: Ошибка настройки Time Card
```bash
sudo chmod 666 /sys/class/timecard/ocp0/sma*
```

### Проблема: Ошибки сборки
```bash
sudo apt-get install build-essential libgtk2.0-dev
```

### Проблема: GUI не запускается
```bash
sudo apt-get install libgtk2.0-dev
```

### Проблема: Скрипты не исполняются
```bash
chmod +x *.sh
```

## Формат выходных данных

Программа создает лог-файлы в формате Calnex CAT:

```
VER:;1;
DataType:;TIEDATA; Format:;CSV;
MeasType:;1pps TE Absolute;
Port:;A;
START:;DD/MM/YYYY HH:MM:SS;
PERIOD:;1;
value;
```

Файлы именуются как: `OpenTimeInstr-ChanN_DD-MM-YYYY_HHMMSS.log`

## Сборка отдельных компонентов

### Только основная программа
```bash
make -f Makefile_enhanced OpenTimeInstrument
```

### Только CLI интерфейс
```bash
make -f Makefile_enhanced cli_interface
```

### Только GUI интерфейс
```bash
make -f Makefile_enhanced gui_interface
```

### Все компоненты
```bash
make -f Makefile_enhanced all
```

## Очистка сборки
```bash
make -f Makefile_enhanced clean
```

## Справка по Makefile
```bash
make -f Makefile_enhanced help
```

## Требования к системе

### Минимальные требования
- Linux система
- GCC компилятор
- Make утилита
- Права доступа к `/dev/ptp*` устройствам

### Для GUI интерфейса
- GTK2 библиотеки
- X11 сервер (для GUI)

### Для полной функциональности
- Time Card с поддержкой PTP
- Права доступа к `/sys/class/timecard/`

## Безопасность

### Права доступа
- Скрипты установки и настройки требуют прав root
- Основная программа может работать без прав root
- GUI интерфейс может работать без прав root

### Рекомендации по безопасности
- Используйте скрипты только из доверенных источников
- Проверяйте права доступа перед выполнением
- Ограничивайте доступ к устройствам PTP при необходимости

## Поддержка

### Логирование
Все компоненты создают подробные логи:
- Основная программа: stdout/stderr
- CLI интерфейс: интерактивный вывод
- GUI интерфейс: встроенный лог
- Скрипты: подробный вывод с статусом

### Отладка
Для отладки используйте:
```bash
# Подробный вывод компиляции
make -f Makefile_enhanced all V=1

# Проверка системы
./check_system.sh

# Тестирование интерфейсов
./test_interface.sh
```

## Лицензия

Это программное обеспечение распространяется под лицензией GNU General Public License v2.

## Авторы

- Основная программа: на основе testptp
- CLI и GUI интерфейсы: OpenTimeInstrument Team
- Скрипты автоматизации: OpenTimeInstrument Team

## Версии

- v1.0 - Базовая версия с CLI интерфейсом
- v1.1 - Добавлен GUI интерфейс
- v1.2 - Улучшенный CLI с интерактивным режимом
- v1.3 - Полная автоматизация с скриптами

## Заключение

OpenTimeInstrument предоставляет полный набор инструментов для работы с PTP Time Card:

- **Основная программа** - для автоматизации и скриптов
- **CLI интерфейс** - для интерактивной работы
- **GUI интерфейс** - для визуального управления
- **Скрипты автоматизации** - для упрощения установки и настройки

Все компоненты интегрированы и работают вместе, предоставляя максимальное удобство использования.