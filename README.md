# HOMEWORK 04

Написать программу, скачивающую с помощью libcurl и разбирающую
с помощью произвольной сторонней библиотеки для JSON в C текущие
погодные данные с https://www.metaweather.com/api/ для заданной
аргументом командной строки локации.

## Цель задания

Получить навык работы со сторонними прикладными библиотеками.

## Критерии успеха

1. Выбрана библиотека для работы с JSON в C.
2. Создано консольное приложение, принимающее аргументом
командной строки локацию (например, moscow).
3. Приложение выводит на экран прогноз погоды на текущий день:
текстовое описание погоды, направление и скорость ветра, диапазон
температуры.
4. Приложение корректно обрабатывает ошибочно заданную локацию
и сетевые ошибки.
5. Код компилируется без warning’ов с ключами компилятора -Wall
-Wextra -Wpedantic -std=c11.
6. Далее успешность определяется code review.

## Build project
To build project run:
```sh
make
```

## Start project
To start project run:
```sh
cd dist && ./main
```