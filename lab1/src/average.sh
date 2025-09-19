#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $0 number1 number2 ..."
    exit 1
fi

sum=0
count=$#

for num in "$@"; do
    sum=$((sum + num))
done

average=$((sum / count))

echo "Количество чисел: $count"
echo "Среднее арифметическое: $average"