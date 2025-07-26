#!/bin/bash

install_packages() {
    echo "Установка необходимых пакетов..."
    case $PACKAGE_MANAGER in
        apt)
            sudo apt update
            sudo apt install -y build-essential
            ;;
        yum)
            sudo yum install -y gcc
            ;;
        dnf)
            sudo dnf install -y gcc
            ;;
        pacman)
            sudo pacman -Syu --noconfirm base-devel
            ;;
        *)
            echo "Неизвестный пакетный менеджер. Установите gcc и вручную."
            exit 1
            ;;
    esac
}

if command -v apt &> /dev/null; then
    PACKAGE_MANAGER="apt"
elif command -v yum &> /dev/null; then
    PACKAGE_MANAGER="yum"
elif command -v dnf &> /dev/null; then
    PACKAGE_MANAGER="dnf"
elif command -v pacman &> /dev/null; then
    PACKAGE_MANAGER="pacman"
else
    echo "Пакетный менеджер не найден. Убедитесь, что вы используете Linux."
    exit 1
fi

if ! command -v gcc &> /dev/null; then
    echo "gcc не установлен."
    install_packages
else
    echo "gcc уже установлен."
fi

echo "Компилирование..."
gcc kristal.c -o kristal
echo "Готово, использование: './kristal help' там больше информации."
echo "Хорошего использования!"