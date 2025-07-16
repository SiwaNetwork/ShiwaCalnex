#!/bin/bash

# Скрипт автоматической установки OpenTimeInstrument
# Автор: OpenTimeInstrument
# Версия: 1.0

set -e  # Остановка при ошибке

echo "=== Установка OpenTimeInstrument ==="
echo ""

# Проверка прав доступа
if [ "$EUID" -ne 0 ]; then
    echo "Ошибка: Этот скрипт должен быть запущен с правами root"
    echo "Используйте: sudo ./install.sh"
    exit 1
fi

# Функция для вывода статуса
print_status() {
    if [ $1 -eq 0 ]; then
        echo "✓ $2"
    else
        echo "✗ $2"
        exit 1
    fi
}

# Обновление пакетов
echo "1. Обновление пакетов..."
apt-get update -qq
print_status $? "Пакеты обновлены"

# Установка зависимостей
echo ""
echo "2. Установка зависимостей..."

# Проверка и установка build-essential
if ! dpkg -l | grep -q build-essential; then
    echo "Установка build-essential..."
    apt-get install -y build-essential
    print_status $? "build-essential установлен"
else
    echo "✓ build-essential уже установлен"
fi

# Проверка и установка pkg-config
if ! dpkg -l | grep -q pkg-config; then
    echo "Установка pkg-config..."
    apt-get install -y pkg-config
    print_status $? "pkg-config установлен"
else
    echo "✓ pkg-config уже установлен"
fi

# Проверка и установка GTK3
if ! pkg-config --exists gtk+-3.0; then
    echo "Установка GTK2..."
    apt-get install -y libgtk3.0-dev
    print_status $? "GTK3 установлен"
else
    echo "✓ GTK3 уже установлен"
fi

# Сборка программы
echo ""
echo "3. Сборка программы..."

# Проверка наличия исходных файлов
if [ ! -f "OpenTimeInstrument.c" ]; then
    echo "✗ Файл OpenTimeInstrument.c не найден"
    exit 1
fi

if [ ! -f "Makefile.txt" ]; then
    echo "✗ Файл Makefile.txt не найден"
    exit 1
fi

# Создание улучшенного Makefile
echo "Создание улучшенного Makefile..."
cat > Makefile_enhanced << 'EOF'
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -lrt
GTK_LIBS = `pkg-config --cflags --libs gtk+-3.0`

all: OpenTimeInstrument cli_interface gui_interface

OpenTimeInstrument: OpenTimeInstrument.c
	$(CC) $(CFLAGS) $(LIBS) OpenTimeInstrument.c -o OpenTimeInstrument

cli_interface: CLI_INTERFACE.c
	$(CC) $(CFLAGS) $(LIBS) CLI_INTERFACE.c -o cli_interface

gui_interface: GUI_INTERFACE.c
	$(CC) $(CFLAGS) GUI_INTERFACE.c $(GTK_LIBS) -o gui_interface

install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential libgtk2.0-dev

clean:
	rm -f OpenTimeInstrument cli_interface gui_interface *.log

help:
	@echo "Доступные цели:"
	@echo "  all          - собрать все версии"
	@echo "  OpenTimeInstrument - оригинальная программа"
	@echo "  cli_interface - улучшенный CLI интерфейс"
	@echo "  gui_interface - простой GUI интерфейс"
	@echo "  install-deps - установить зависимости"
	@echo "  clean        - очистить сборку"
	@echo "  help         - показать эту справку"

.PHONY: all clean install-deps help
EOF
print_status $? "Makefile_enhanced создан"

# Создание CLI интерфейса
echo "Создание CLI интерфейса..."
cat > CLI_INTERFACE.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void show_menu() {
    printf("\n=== OpenTimeInstrument CLI ===\n");
    printf("1. Начать измерение (бесконечный режим)\n");
    printf("2. Начать измерение (N событий)\n");
    printf("3. Настроить Time Card\n");
    printf("4. Проверить устройство\n");
    printf("5. Показать возможности PTP\n");
    printf("6. Получить время PTP\n");
    printf("7. Выход\n");
    printf("Выберите опцию: ");
}

int main() {
    char choice[10];
    char device[256] = "/dev/ptp1";
    
    while (1) {
        show_menu();
        if (fgets(choice, sizeof(choice), stdin) != NULL) {
            choice[strcspn(choice, "\n")] = 0;
            
            switch (choice[0]) {
                case '1': {
                    char command[512];
                    sprintf(command, "./OpenTimeInstrument -d %s -e -1", device);
                    system(command);
                    break;
                }
                case '2': {
                    printf("Введите количество событий: ");
                    int events;
                    scanf("%d", &events);
                    char command[512];
                    sprintf(command, "./OpenTimeInstrument -d %s -e %d", device, events);
                    system(command);
                    break;
                }
                case '3': {
                    system("echo IN: TS1 >> /sys/class/timecard/ocp0/sma1");
                    system("echo IN: TS2 >> /sys/class/timecard/ocp0/sma2");
                    system("echo IN: TS3 >> /sys/class/timecard/ocp0/sma3");
                    system("echo IN: TS4 >> /sys/class/timecard/ocp0/sma4");
                    printf("Time Card настроен\n");
                    break;
                }
                case '4': {
                    char command[512];
                    sprintf(command, "ls -la %s", device);
                    system(command);
                    break;
                }
                case '5': {
                    char command[512];
                    sprintf(command, "./OpenTimeInstrument -d %s -c", device);
                    system(command);
                    break;
                }
                case '6': {
                    char command[512];
                    sprintf(command, "./OpenTimeInstrument -d %s -g", device);
                    system(command);
                    break;
                }
                case '7':
                    return 0;
                default:
                    printf("Неверный выбор\n");
                    break;
            }
        }
    }
    return 0;
}
EOF
print_status $? "CLI_INTERFACE.c создан"

# Создание GUI интерфейса
echo "Создание GUI интерфейса..."
cat > GUI_INTERFACE.c << 'EOF'
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *device_entry;
static GtkWidget *status_label;
static GtkWidget *log_text;
static GtkTextBuffer *log_buffer;

void add_log_message(const char *message) {
    gtk_text_buffer_insert_at_cursor(log_buffer, message, -1);
}

void on_start_clicked(GtkWidget *widget, gpointer data) {
    const char *device_path = gtk_entry_get_text(GTK_ENTRY(device_entry));
    
    char command[512];
    sprintf(command, "./OpenTimeInstrument -d %s -e -1", device_path);
    
    add_log_message("Запуск измерений...\n");
    gtk_label_set_text(GTK_LABEL(status_label), "Измерение активно");
    
    sprintf(command, "%s &", command);
    system(command);
}

void on_setup_clicked(GtkWidget *widget, gpointer data) {
    add_log_message("Настройка Time Card...\n");
    
    system("echo IN: TS1 >> /sys/class/timecard/ocp0/sma1");
    system("echo IN: TS2 >> /sys/class/timecard/ocp0/sma2");
    system("echo IN: TS3 >> /sys/class/timecard/ocp0/sma3");
    system("echo IN: TS4 >> /sys/class/timecard/ocp0/sma4");
    
    add_log_message("Time Card настроен\n");
}

void on_check_clicked(GtkWidget *widget, gpointer data) {
    const char *device_path = gtk_entry_get_text(GTK_ENTRY(device_entry));
    
    char command[512];
    sprintf(command, "ls -la %s", device_path);
    system(command);
    
    add_log_message("Проверка устройства завершена\n");
}

void on_window_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OpenTimeInstrument GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    GtkWidget *vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    GtkWidget *device_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), device_hbox, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(device_hbox), gtk_label_new("Устройство:"), FALSE, FALSE, 0);
    device_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(device_entry), "/dev/ptp1");
    gtk_box_pack_start(GTK_BOX(device_hbox), device_entry, TRUE, TRUE, 0);
    
    GtkWidget *check_button = gtk_button_new_with_label("Проверить");
    g_signal_connect(check_button, "clicked", G_CALLBACK(on_check_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(device_hbox), check_button, FALSE, FALSE, 0);
    
    GtkWidget *buttons_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), buttons_hbox, FALSE, FALSE, 0);
    
    GtkWidget *setup_button = gtk_button_new_with_label("Настроить Time Card");
    g_signal_connect(setup_button, "clicked", G_CALLBACK(on_setup_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), setup_button, FALSE, FALSE, 0);
    
    GtkWidget *start_button = gtk_button_new_with_label("Начать измерение");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), start_button, FALSE, FALSE, 0);
    
    status_label = gtk_label_new("Готов к работе");
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
    
    GtkWidget *log_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), log_scrolled, TRUE, TRUE, 0);
    
    log_text = gtk_text_view_new();
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_text));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_text), FALSE);
    gtk_container_add(GTK_CONTAINER(log_scrolled), log_text);
    
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
EOF
print_status $? "GUI_INTERFACE.c создан"

# Сборка всех компонентов
echo ""
echo "4. Сборка всех компонентов..."
make -f Makefile_enhanced all
print_status $? "Все компоненты собраны"

# Установка прав на выполнение
echo ""
echo "5. Установка прав на выполнение..."
chmod +x OpenTimeInstrument cli_interface gui_interface setup_timecard.sh check_system.sh
print_status $? "Права на выполнение установлены"

# Создание документации
echo ""
echo "6. Создание документации..."

cat > README_RU.md << 'EOF'
# OpenTimeInstrument - Инструмент для измерения времени PTP

## Описание

OpenTimeInstrument - это программное обеспечение для работы с PTP (Precision Time Protocol) Time Card. Программа читает временные метки с входов Time Card и выводит измерения TIE (Time Interval Error) в формате, распознаваемом Calnex CAT.

## Установка

### Автоматическая установка
```bash
sudo ./install.sh
```

### Ручная установка
```bash
# Установка зависимостей
sudo apt-get update
sudo apt-get install -y build-essential libgtk3.0-dev

# Сборка
make -f Makefile_enhanced all
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

## Устранение неполадок

### Устройство недоступно
```bash
sudo chmod 666 /dev/ptp1
```

### Ошибка настройки Time Card
```bash
sudo chmod 666 /sys/class/timecard/ocp0/sma*
```

### Ошибки сборки
```bash
sudo apt-get install build-essential libgtk3.0-dev
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
EOF
print_status $? "README_RU.md создан"

cat > ИНСТРУКЦИЯ.md << 'EOF'
# Инструкция по использованию OpenTimeInstrument

## Быстрый старт

### 1. Автоматическая установка
```bash
sudo ./install.sh
```

### 2. Настройка Time Card
```bash
sudo ./setup_timecard.sh
```

### 3. Проверка системы
```bash
./check_system.sh
```

### 4. Запуск измерений
```bash
./OpenTimeInstrument -d /dev/ptp1 -e -1
```

## CLI интерфейс

### Использование улучшенного CLI
```bash
./cli_interface
```

### Возможности CLI интерфейса
- Интерактивное меню
- Настройка Time Card
- Проверка устройства
- Запуск измерений
- Просмотр возможностей PTP

## GUI интерфейс

### Использование GUI
```bash
./gui_interface
```

### Возможности GUI интерфейса
- Графический интерфейс
- Настройка устройства
- Проверка доступности
- Запуск/остановка измерений
- Просмотр логов

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

## Заключение

OpenTimeInstrument предоставляет удобные интерфейсы для работы с PTP Time Card:

- **CLI** - для автоматизации и скриптов
- **Улучшенный CLI** - для интерактивной работы
- **GUI** - для визуального управления

Все интерфейсы предоставляют одинаковую функциональность, но с разным уровнем удобства использования.
EOF
print_status $? "ИНСТРУКЦИЯ.md создан"

echo ""
echo "=== Установка завершена успешно! ==="
echo ""
echo "Доступные команды:"
echo "  ./OpenTimeInstrument -d /dev/ptp1 -e -1  # Основная программа"
echo "  ./cli_interface                           # CLI интерфейс"
echo "  ./gui_interface                           # GUI интерфейс"
echo "  sudo ./setup_timecard.sh                  # Настройка Time Card"
echo "  ./check_system.sh                         # Проверка системы"
echo ""
echo "Документация:"
echo "  README_RU.md                              # Основная документация"
echo "  ИНСТРУКЦИЯ.md                             # Инструкция по использованию"
echo ""
echo "Удачной работы с OpenTimeInstrument!"