#!/bin/bash

# Скрипт для настройки Time Card
# Автор: OpenTimeInstrument
# Версия: 1.0

echo "=== Настройка Time Card ==="

# Проверка прав доступа
if [ "$EUID" -ne 0 ]; then
    echo "Ошибка: Этот скрипт должен быть запущен с правами root"
    echo "Используйте: sudo ./setup_timecard.sh"
    exit 1
fi

# Проверка наличия директории
if [ ! -d "/sys/class/timecard" ]; then
    echo "Ошибка: Директория /sys/class/timecard не найдена"
    echo "Убедитесь, что Time Card подключен и драйвер загружен"
    exit 1
fi

echo "Настройка SMA как Timestamper inputs..."

# Настройка SMA
echo "IN: TS1" > /sys/class/timecard/ocp0/sma1
if [ $? -eq 0 ]; then
    echo "✓ SMA1 настроен"
else
    echo "✗ Ошибка настройки SMA1"
fi

echo "IN: TS2" > /sys/class/timecard/ocp0/sma2
if [ $? -eq 0 ]; then
    echo "✓ SMA2 настроен"
else
    echo "✗ Ошибка настройки SMA2"
fi

echo "IN: TS3" > /sys/class/timecard/ocp0/sma3
if [ $? -eq 0 ]; then
    echo "✓ SMA3 настроен"
else
    echo "✗ Ошибка настройки SMA3"
fi

echo "IN: TS4" > /sys/class/timecard/ocp0/sma4
if [ $? -eq 0 ]; then
    echo "✓ SMA4 настроен"
else
    echo "✗ Ошибка настройки SMA4"
fi

echo ""
echo "=== Проверка настроек ==="

# Проверка настроек
for i in {1..4}; do
    if [ -f "/sys/class/timecard/ocp0/sma$i" ]; then
        echo "SMA$i: $(cat /sys/class/timecard/ocp0/sma$i)"
    else
        echo "SMA$i: не найден"
    fi
done

echo ""
echo "Time Card настроен успешно!"
echo "Теперь можно запускать OpenTimeInstrument"