#!/bin/bash

# Скрипт удаления OpenTimeInstrument
# Автор: OpenTimeInstrument
# Версия: 1.0

echo "=== Удаление OpenTimeInstrument ==="
echo ""

# Проверка прав доступа
if [ "$EUID" -ne 0 ]; then
    echo "Ошибка: Этот скрипт должен быть запущен с правами root"
    echo "Используйте: sudo ./uninstall.sh"
    exit 1
fi

# Функция для вывода статуса
print_status() {
    if [ $1 -eq 0 ]; then
        echo "✓ $2"
    else
        echo "✗ $2"
    fi
}

# Остановка процессов
echo "1. Остановка процессов..."
pkill -f OpenTimeInstrument 2>/dev/null || true
pkill -f cli_interface 2>/dev/null || true
pkill -f gui_interface 2>/dev/null || true
print_status 0 "Процессы остановлены"

# Удаление исполняемых файлов
echo ""
echo "2. Удаление исполняемых файлов..."

EXECUTABLES=("OpenTimeInstrument" "cli_interface" "gui_interface")
for file in "${EXECUTABLES[@]}"; do
    if [ -f "$file" ]; then
        rm -f "$file"
        print_status $? "$file удален"
    else
        echo "✓ $file не найден"
    fi
done

# Удаление исходных файлов интерфейсов
echo ""
echo "3. Удаление исходных файлов интерфейсов..."

SOURCE_FILES=("CLI_INTERFACE.c" "GUI_INTERFACE.c" "Makefile_enhanced")
for file in "${SOURCE_FILES[@]}"; do
    if [ -f "$file" ]; then
        rm -f "$file"
        print_status $? "$file удален"
    else
        echo "✓ $file не найден"
    fi
done

# Удаление скриптов
echo ""
echo "4. Удаление скриптов..."

SCRIPTS=("setup_timecard.sh" "check_system.sh" "install.sh" "uninstall.sh")
for script in "${SCRIPTS[@]}"; do
    if [ -f "$script" ]; then
        rm -f "$script"
        print_status $? "$script удален"
    else
        echo "✓ $script не найден"
    fi
done

# Удаление лог-файлов
echo ""
echo "5. Удаление лог-файлов..."
rm -f OpenTimeInstr-Chan*.log 2>/dev/null || true
print_status 0 "Лог-файлы удалены"

# Удаление документации
echo ""
echo "6. Удаление документации..."

DOCS=("README_RU.md" "ИНСТРУКЦИЯ.md")
for doc in "${DOCS[@]}"; do
    if [ -f "$doc" ]; then
        rm -f "$doc"
        print_status $? "$doc удален"
    else
        echo "✓ $doc не найден"
    fi
done

# Вопрос об удалении зависимостей
echo ""
echo "7. Удаление зависимостей..."
read -p "Удалить установленные зависимости (GTK2, build-essential)? [y/N]: " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Удаление GTK2..."
    apt-get remove -y libgtk2.0-dev
    print_status $? "GTK2 удален"
    
    echo "Удаление build-essential..."
    apt-get remove -y build-essential
    print_status $? "build-essential удален"
    
    echo "Очистка пакетов..."
    apt-get autoremove -y
    print_status $? "Неиспользуемые пакеты удалены"
else
    echo "Зависимости сохранены"
fi

echo ""
echo "=== Удаление завершено! ==="
echo ""
echo "Остались файлы:"
echo "  OpenTimeInstrument.c    # Основной исходный код"
echo "  Makefile.txt           # Оригинальный Makefile"
echo "  README.md              # Оригинальная документация"
echo ""
echo "Для полного удаления удалите эти файлы вручную."