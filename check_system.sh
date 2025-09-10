#!/bin/bash

# Скрипт для проверки системы и зависимостей
# Автор: OpenTimeInstrument
# Версия: 1.0

echo "=== Проверка системы для OpenTimeInstrument ==="
echo ""

# Проверка операционной системы
echo "1. Проверка операционной системы..."
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "✓ Linux система обнаружена"
else
    echo "✗ Неподдерживаемая операционная система: $OSTYPE"
    exit 1
fi

# Проверка архитектуры
echo ""
echo "2. Проверка архитектуры..."
ARCH=$(uname -m)
echo "Архитектура: $ARCH"

# Проверка прав доступа
echo ""
echo "3. Проверка прав доступа..."
if [ "$EUID" -eq 0 ]; then
    echo "✓ Запущено с правами root"
else
    echo "⚠ Запущено без прав root (некоторые операции могут не работать)"
fi

# Проверка необходимых пакетов
echo ""
echo "4. Проверка необходимых пакетов..."

# Проверка gcc
if command -v gcc &> /dev/null; then
    GCC_VERSION=$(gcc --version | head -n1)
    echo "✓ GCC найден: $GCC_VERSION"
else
    echo "✗ GCC не найден"
    echo "Установите: sudo apt-get install build-essential"
fi

# Проверка make
if command -v make &> /dev/null; then
    MAKE_VERSION=$(make --version | head -n1)
    echo "✓ Make найден: $MAKE_VERSION"
else
    echo "✗ Make не найден"
    echo "Установите: sudo apt-get install build-essential"
fi

# Проверка pkg-config
if command -v pkg-config &> /dev/null; then
    echo "✓ pkg-config найден"
else
    echo "✗ pkg-config не найден"
    echo "Установите: sudo apt-get install pkg-config"
fi

# Проверка GTK
if pkg-config --exists gtk+-2.0; then
    GTK_VERSION=$(pkg-config --modversion gtk+-2.0)
    echo "✓ GTK2 найден: $GTK_VERSION"
else
    echo "✗ GTK2 не найден"
    echo "Установите: sudo apt-get install libgtk2.0-dev"
fi

# Проверка библиотек
echo ""
echo "5. Проверка библиотек..."

# Проверка librt
if ldconfig -p | grep -q "librt"; then
    echo "✓ librt найден"
else
    echo "✗ librt не найден"
fi

# Проверка устройств PTP
echo ""
echo "6. Проверка устройств PTP..."

PTP_DEVICES=$(ls /dev/ptp* 2>/dev/null)
if [ -n "$PTP_DEVICES" ]; then
    echo "✓ PTP устройства найдены:"
    for device in $PTP_DEVICES; do
        echo "  $device"
    done
else
    echo "✗ PTP устройства не найдены"
    echo "Убедитесь, что Time Card подключен и драйвер загружен"
fi

# Проверка Time Card
echo ""
echo "7. Проверка Time Card..."

if [ -d "/sys/class/timecard" ]; then
    echo "✓ Директория /sys/class/timecard найдена"
    
    # Проверка SMA
    for i in {1..4}; do
        if [ -f "/sys/class/timecard/ocp0/sma$i" ]; then
            echo "  ✓ SMA$i найден"
        else
            echo "  ✗ SMA$i не найден"
        fi
    done
else
    echo "✗ Директория /sys/class/timecard не найдена"
    echo "Убедитесь, что Time Card подключен и драйвер загружен"
fi

# Проверка исходных файлов
echo ""
echo "8. Проверка исходных файлов..."

REQUIRED_FILES=("OpenTimeInstrument.c" "Makefile.txt")
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file найден"
    else
        echo "✗ $file не найден"
    fi
done

# Проверка скомпилированных файлов
echo ""
echo "9. Проверка скомпилированных файлов..."

if [ -f "OpenTimeInstrument" ]; then
    echo "✓ OpenTimeInstrument скомпилирован"
else
    echo "✗ OpenTimeInstrument не скомпилирован"
    echo "Выполните: make"
fi

# Рекомендации
echo ""
echo "=== Рекомендации ==="

if ! command -v gcc &> /dev/null || ! command -v make &> /dev/null; then
    echo "• Установите build-essential: sudo apt-get install build-essential"
fi

if ! pkg-config --exists gtk+-2.0; then
    echo "• Установите GTK2: sudo apt-get install libgtk2.0-dev"
fi

if [ ! -d "/sys/class/timecard" ]; then
    echo "• Убедитесь, что Time Card подключен и драйвер загружен"
fi

if [ ! -f "OpenTimeInstrument" ]; then
    echo "• Скомпилируйте программу: make"
fi

echo ""
echo "Проверка завершена!"