#!/bin/bash

# Скрипт для тестирования интерфейсов OpenTimeInstrument
# Автор: OpenTimeInstrument
# Версия: 1.0

echo "=== Тестирование интерфейсов OpenTimeInstrument ==="
echo ""

# Функция для вывода статуса
print_status() {
    if [ $1 -eq 0 ]; then
        echo "✓ $2"
    else
        echo "✗ $2"
    fi
}

# Проверка наличия файлов
echo "1. Проверка наличия файлов..."

REQUIRED_FILES=("OpenTimeInstrument" "cli_interface" "gui_interface")
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file найден"
    else
        echo "✗ $file не найден"
        echo "Выполните: sudo ./install.sh"
        exit 1
    fi
done

# Проверка прав на выполнение
echo ""
echo "2. Проверка прав на выполнение..."

for file in "${REQUIRED_FILES[@]}"; do
    if [ -x "$file" ]; then
        echo "✓ $file исполняемый"
    else
        echo "✗ $file не исполняемый"
        chmod +x "$file"
        print_status $? "Права на выполнение установлены для $file"
    fi
done

# Тест основной программы
echo ""
echo "3. Тест основной программы..."

echo "Проверка справки..."
./OpenTimeInstrument -h > /dev/null 2>&1
print_status $? "Справка работает"

echo "Проверка возможностей устройства..."
./OpenTimeInstrument -d /dev/ptp0 -c > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Устройство /dev/ptp0 доступно"
else
    echo "⚠ Устройство /dev/ptp0 недоступно (это нормально для тестирования)"
fi

# Тест CLI интерфейса
echo ""
echo "4. Тест CLI интерфейса..."

echo "Запуск CLI интерфейса (тест меню)..."
timeout 5s ./cli_interface > /dev/null 2>&1
if [ $? -eq 124 ]; then
    echo "✓ CLI интерфейс запускается (таймаут)"
else
    print_status $? "CLI интерфейс работает"
fi

# Тест GUI интерфейса
echo ""
echo "5. Тест GUI интерфейса..."

echo "Проверка GTK..."
if pkg-config --exists gtk+-2.0; then
    echo "✓ GTK2 доступен"
    
    echo "Запуск GUI интерфейса (тест окна)..."
    timeout 3s ./gui_interface > /dev/null 2>&1 &
    GUI_PID=$!
    sleep 2
    if kill -0 $GUI_PID 2>/dev/null; then
        kill $GUI_PID
        echo "✓ GUI интерфейс запускается"
    else
        echo "✗ GUI интерфейс не запускается"
    fi
else
    echo "✗ GTK2 недоступен"
fi

# Тест скриптов
echo ""
echo "6. Тест скриптов..."

if [ -f "setup_timecard.sh" ]; then
    echo "Проверка скрипта настройки Time Card..."
    if [ -x "setup_timecard.sh" ]; then
        echo "✓ setup_timecard.sh исполняемый"
    else
        chmod +x setup_timecard.sh
        print_status $? "Права на выполнение установлены для setup_timecard.sh"
    fi
else
    echo "⚠ setup_timecard.sh не найден"
fi

if [ -f "check_system.sh" ]; then
    echo "Проверка скрипта проверки системы..."
    if [ -x "check_system.sh" ]; then
        echo "✓ check_system.sh исполняемый"
    else
        chmod +x check_system.sh
        print_status $? "Права на выполнение установлены для check_system.sh"
    fi
else
    echo "⚠ check_system.sh не найден"
fi

# Проверка документации
echo ""
echo "7. Проверка документации..."

DOCS=("README_RU.md" "ИНСТРУКЦИЯ.md")
for doc in "${DOCS[@]}"; do
    if [ -f "$doc" ]; then
        echo "✓ $doc найден"
        # Проверка размера файла
        SIZE=$(stat -c%s "$doc")
        if [ $SIZE -gt 100 ]; then
            echo "  ✓ Файл содержит данные ($SIZE байт)"
        else
            echo "  ⚠ Файл слишком маленький ($SIZE байт)"
        fi
    else
        echo "✗ $doc не найден"
    fi
done

# Проверка Makefile
echo ""
echo "8. Проверка Makefile..."

if [ -f "Makefile_enhanced" ]; then
    echo "✓ Makefile_enhanced найден"
    
    echo "Проверка целей Makefile..."
    make -f Makefile_enhanced help > /dev/null 2>&1
    print_status $? "Makefile работает"
else
    echo "✗ Makefile_enhanced не найден"
fi

# Итоговый отчет
echo ""
echo "=== Итоговый отчет ==="

echo "Основная программа:"
if [ -f "OpenTimeInstrument" ] && [ -x "OpenTimeInstrument" ]; then
    echo "  ✓ Готова к использованию"
else
    echo "  ✗ Не готова"
fi

echo "CLI интерфейс:"
if [ -f "cli_interface" ] && [ -x "cli_interface" ]; then
    echo "  ✓ Готов к использованию"
else
    echo "  ✗ Не готов"
fi

echo "GUI интерфейс:"
if [ -f "gui_interface" ] && [ -x "gui_interface" ] && pkg-config --exists gtk+-2.0; then
    echo "  ✓ Готов к использованию"
else
    echo "  ✗ Не готов"
fi

echo "Скрипты:"
if [ -f "setup_timecard.sh" ] && [ -x "setup_timecard.sh" ]; then
    echo "  ✓ setup_timecard.sh готов"
else
    echo "  ✗ setup_timecard.sh не готов"
fi

if [ -f "check_system.sh" ] && [ -x "check_system.sh" ]; then
    echo "  ✓ check_system.sh готов"
else
    echo "  ✗ check_system.sh не готов"
fi

echo ""
echo "=== Рекомендации ==="

if [ ! -f "OpenTimeInstrument" ] || [ ! -x "OpenTimeInstrument" ]; then
    echo "• Выполните: sudo ./install.sh"
fi

if [ ! -d "/sys/class/timecard" ]; then
    echo "• Убедитесь, что Time Card подключен"
fi

if ! pkg-config --exists gtk+-2.0; then
    echo "• Установите GTK2: sudo apt-get install libgtk2.0-dev"
fi

echo ""
echo "Тестирование завершено!"