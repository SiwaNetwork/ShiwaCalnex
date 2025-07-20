# OpenTimeInstrument - Web и Enhanced GUI интерфейсы

## Обзор

Добавлены новые интерфейсы для работы с OpenTimeInstrument:

1. **Web интерфейс** - современный веб-интерфейс с интерактивными графиками
2. **Enhanced GUI** - расширенная GTK версия с встроенными графиками

Оба интерфейса поддерживают:
- Мониторинг 4 каналов TIE измерений в реальном времени
- Интерактивные графики для визуализации данных
- Статистику измерений (минимум, максимум, среднее)
- Экспорт данных в CSV формат
- Очистку данных

## Web интерфейс

### Возможности

- **Современный UI** с Bootstrap дизайном
- **Интерактивные графики** с использованием Plotly.js
- **Real-time обновления** через WebSocket
- **Адаптивный дизайн** для разных размеров экрана
- **Сводный график** всех каналов
- **Индивидуальные графики** для каждого канала
- **Статистическая панель** с метриками
- **Экспорт данных** в CSV

### Установка зависимостей

```bash
# Установка Python зависимостей
pip3 install -r requirements.txt

# Или через Makefile
make -f Makefile_enhanced_web install-web-deps
```

### Запуск

```bash
# Простой запуск
python3 web_interface.py

# Или через удобный скрипт
./launch_web.sh

# Или через Makefile
make -f Makefile_enhanced_web run-web
```

Откройте браузер и перейдите на `http://localhost:5000`

### Архитектура web интерфейса

```
web_interface.py          # Flask приложение с WebSocket
├── templates/
│   └── index.html        # HTML шаблон с графиками
├── static/               # Статические файлы (CSS, JS)
└── requirements.txt      # Python зависимости
```

### API endpoints

- `GET /` - Главная страница
- `GET /api/devices` - Список PTP устройств
- `POST /api/start_measurement` - Запуск измерений
- `POST /api/stop_measurement` - Остановка измерений
- `GET /api/measurement_data` - Получение данных
- `POST /api/clear_data` - Очистка данных
- `GET /api/export_data` - Экспорт CSV

### WebSocket события

- `measurement_update` - Новое измерение
- `data_cleared` - Данные очищены
- `status` - Обновление статуса

## Enhanced GUI интерфейс

### Возможности

- **GTK+ интерфейс** с графиками Cairo
- **Real-time графики** для каждого канала
- **Сводный график** всех каналов
- **Вкладки** для индивидуальных графиков
- **Цветовая дифференциация** каналов
- **Статистика** с автообновлением
- **Экспорт данных** с временными метками

### Установка зависимостей

```bash
# Установка GTK+ зависимостей
sudo apt-get install build-essential pkg-config libgtk2.0-dev

# Или через Makefile
make -f Makefile_enhanced_web install-deps
```

### Сборка и запуск

```bash
# Сборка
make -f Makefile_enhanced_web gui_interface_enhanced

# Запуск
./gui_interface_enhanced

# Или через удобный скрипт
./launch_gui_enhanced.sh

# Или через Makefile
make -f Makefile_enhanced_web run-gui
```

### Архитектура Enhanced GUI

```c
GUI_INTERFACE_ENHANCED.c
├── ChannelData структура      # Хранение данных каналов
├── Cairo графики             # Рендеринг графиков
├── GTK+ виджеты              # Пользовательский интерфейс
└── Real-time обновления      # Мониторинг измерений
```

### Особенности реализации

- **Циркулярный буфер** для хранения до 1000 точек на канал
- **Автоматическое масштабирование** графиков
- **Цветовая схема** для различения каналов
- **Сетка и легенда** на графиках
- **Многопоточность** для обновлений в реальном времени

## Сравнение интерфейсов

| Особенность | CLI | Basic GUI | Enhanced GUI | Web Interface |
|-------------|-----|-----------|--------------|---------------|
| Графики | ❌ | ❌ | ✅ | ✅ |
| Real-time | ✅ | ✅ | ✅ | ✅ |
| Статистика | ❌ | ✅ | ✅ | ✅ |
| Экспорт | ✅ | ❌ | ✅ | ✅ |
| Удаленный доступ | ❌ | ❌ | ❌ | ✅ |
| Интерактивность | ❌ | ✅ | ✅ | ✅ |
| Мобильная версия | ❌ | ❌ | ❌ | ✅ |

## Файлы проекта

### Новые файлы

```
web_interface.py              # Web интерфейс Flask
templates/index.html          # HTML шаблон
GUI_INTERFACE_ENHANCED.c      # Расширенный GUI
requirements.txt              # Python зависимости
Makefile_enhanced_web         # Расширенный Makefile
launch_web.sh                 # Скрипт запуска web
launch_gui_enhanced.sh        # Скрипт запуска GUI
```

### Обновленные файлы

```
README_WEB_GUI.md            # Эта документация
```

## Использование

### 1. Подготовка системы

```bash
# Клонирование или обновление
cd /path/to/OpenTimeInstrument

# Установка всех зависимостей
make -f Makefile_enhanced_web install-deps
```

### 2. Сборка всех компонентов

```bash
make -f Makefile_enhanced_web all
```

### 3. Выбор интерфейса

#### Web интерфейс (рекомендуется)
```bash
./launch_web.sh
# Откройте http://localhost:5000
```

#### Enhanced GUI
```bash
./launch_gui_enhanced.sh
```

#### Basic GUI (оригинальный)
```bash
make -f Makefile_enhanced_web run-gui-basic
```

#### CLI
```bash
make -f Makefile_enhanced_web run-cli
```

## Технические детали

### Обработка данных

1. **OpenTimeInstrument** читает PTP timestamps
2. **Вычисляет TIE** (Time Interval Error) в наносекундах  
3. **Выводит** в формате: `*****Measurement channel X: Y ns`
4. **Интерфейсы** парсят этот вывод и строят графики

### Формат данных

```
Timestamp,Channel,Value_ns
2024-01-15T10:30:45.123Z,1,150
2024-01-15T10:30:46.124Z,1,-75
2024-01-15T10:30:47.125Z,2,200
```

### Масштабирование графиков

- **Автоматическое** определение минимума/максимума
- **Буферизация** последних 1000 точек
- **Плавное обновление** при добавлении новых данных

## Устранение неполадок

### Web интерфейс

```bash
# Проверка Python зависимостей
python3 -c "import flask, flask_socketio, plotly"

# Проверка портов
netstat -tulpn | grep :5000

# Логи
python3 web_interface.py  # Смотрите вывод в консоли
```

### Enhanced GUI

```bash
# Проверка GTK
pkg-config --exists gtk+-2.0 && echo "GTK OK"

# Проверка Cairo
ldd gui_interface_enhanced | grep cairo

# Права доступа к PTP устройствам
ls -la /dev/ptp*
```

### Общие проблемы

1. **Нет доступа к /dev/ptp*** - добавьте пользователя в группу или используйте sudo
2. **Ошибки сборки** - установите все зависимости через `install-deps`
3. **Графики не отображаются** - проверьте JavaScript в браузере

## Производительность

### Web интерфейс
- **Обновления**: каждые 100мс через WebSocket
- **Буфер**: 1000 точек на канал
- **Пропускная способность**: ~40 измерений/сек на канал

### Enhanced GUI  
- **Обновления**: каждые 100мс через GTK timer
- **Рендеринг**: Cairo с аппаратным ускорением
- **Память**: ~4MB для данных графиков

## Расширение функциональности

### Добавление нового канала

1. Увеличьте `NUM_CHANNELS` в `GUI_INTERFACE_ENHANCED.c`
2. Добавьте цвет в `channel_colors` массив
3. Обновите `web_interface.py` для поддержки нового канала

### Изменение размера буфера

1. Измените `MAX_DATA_POINTS` в исходном коде
2. Обновите `maxlen` в Python коде
3. Пересоберите приложения

### Новые метрики

1. Добавьте поля в структуру `ChannelData`
2. Обновите функции расчета статистики
3. Добавьте отображение в UI

## Лицензия

Эти расширения следуют той же лицензии, что и основной проект OpenTimeInstrument.