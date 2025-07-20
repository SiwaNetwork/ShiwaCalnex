#!/bin/bash

# Скрипт запуска расширенного GUI интерфейса OpenTimeInstrument

echo "============================================="
echo "OpenTimeInstrument Enhanced GUI Interface"
echo "============================================="

# Проверка зависимостей
echo "Проверка зависимостей..."

# Проверка GTK
if ! pkg-config --exists gtk+-2.0; then
    echo "Ошибка: GTK+ 2.0 не установлен"
    echo "Установите GTK+: sudo apt-get install libgtk2.0-dev"
    exit 1
fi

# Проверка основной программы
if [ ! -f "./OpenTimeInstrument" ]; then
    echo "Основная программа не найдена. Попытка сборки..."
    make -f Makefile_enhanced_web OpenTimeInstrument
    if [ $? -ne 0 ]; then
        echo "Ошибка сборки основной программы"
        exit 1
    fi
fi

# Проверка расширенного GUI
if [ ! -f "./gui_interface_enhanced" ]; then
    echo "Расширенный GUI не найден. Попытка сборки..."
    make -f Makefile_enhanced_web gui_interface_enhanced
    if [ $? -ne 0 ]; then
        echo "Ошибка сборки GUI интерфейса"
        echo "Убедитесь, что установлены все зависимости:"
        echo "sudo apt-get install build-essential pkg-config libgtk2.0-dev"
        exit 1
    fi
fi

echo "Запуск расширенного GUI интерфейса..."
echo ""
echo "Возможности:"
echo "- Мониторинг 4 каналов в реальном времени"
echo "- Графики для каждого канала"
echo "- Сводный график всех каналов"
echo "- Статистика измерений"
echo "- Экспорт данных в CSV"
echo ""

# Запуск GUI
./gui_interface_enhanced