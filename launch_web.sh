#!/bin/bash

# Скрипт запуска веб-интерфейса OpenTimeInstrument

echo "==================================="
echo "OpenTimeInstrument Web Interface"
echo "==================================="

# Проверка зависимостей
echo "Проверка зависимостей..."

# Проверка Python
if ! command -v python3 &> /dev/null; then
    echo "Ошибка: Python3 не установлен"
    echo "Установите Python3: sudo apt-get install python3 python3-pip"
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

# Проверка и установка Python зависимостей
echo "Проверка Python зависимостей..."
python3 -c "import flask, flask_socketio, plotly" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Python зависимости не найдены. Попытка установки..."
    
    # Try pip3 install first
    pip3 install -r requirements.txt 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Попытка установки с --break-system-packages..."
        pip3 install -r requirements.txt --break-system-packages 2>/dev/null
        if [ $? -ne 0 ]; then
            echo "❌ Не удалось установить зависимости автоматически"
            echo ""
            echo "Попробуйте один из следующих способов:"
            echo "1. sudo pip3 install -r requirements.txt --break-system-packages"
            echo "2. Создать виртуальное окружение:"
            echo "   python3 -m venv venv"
            echo "   source venv/bin/activate"
            echo "   pip install -r requirements.txt"
            echo "3. Установить системные пакеты:"
            echo "   sudo apt install python3-flask python3-flask-socketio python3-plotly"
            echo ""
            echo "После установки зависимостей запустите скрипт снова."
            exit 1
        fi
    fi
    
    # Verify installation
    python3 -c "import flask, flask_socketio, plotly" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "❌ Зависимости по-прежнему недоступны после установки"
        exit 1
    fi
    echo "✅ Зависимости успешно установлены"
fi

# Создание директорий для шаблонов
mkdir -p templates static

echo "Запуск веб-интерфейса..."
echo ""
echo "Откройте браузер и перейдите по адресу:"
echo "http://localhost:5000"
echo ""
echo "Для остановки нажмите Ctrl+C"
echo ""

# Запуск веб-интерфейса
python3 web_interface.py