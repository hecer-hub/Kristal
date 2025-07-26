Kristal - Мини-скриптовый язык

Kristal - это легковесный интерпретатор скриптового языка (.krst) для выполнения простых операций: работа с переменными, ввод/вывод, архивация, математические операции и обработка файлов.


Возможности

Создание и использование переменных (stv, ourip, rd)

Условные конструкции (ifi)

Архивация/разархивация файлов и папок (arc, unarc)

Работа с файлами: запись (wf), чтение (rf), поиск по шаблону (grep), вывод содержимого (cat)

Установка уровня доступа (lvl)

Математические операции (math)

Очистка экрана (clear)

Логирование ошибок (флаг -e)


Установка


Установите архив с этим исходным кодом и распакуйте


Скомпилируйте:bash start.sh



Использование
./kristal [-e] script.krst


-e: записывает ошибки в error.txt

Пример скрипта (example.krst)

stv name="Alice"

ourip['Hello, ' @name]

rd age ['Enter your age: ']

ifi age = 18 [ ourip['Adult'] ] j [ ourip['Minor'] ]

arc['archive.artr' 'file1.txt' 'file2.txt']

unarc['archive.artr' 'output_dir']

math result = @age + 5

ourip['Age in 5 years: ' @result]

Команды

stv name="data": создает переменную
ourip['text' @var]: выводит текст и значение переменной
rd var ['prompt']: читает ввод в переменную
ifi var = value [ code ] j [ code ]: условный оператор
arc['archive.artr' 'file1' 'file2']: архивирует файлы/папки
unarc['archive.artr' 'dir']: распаковывает архив
wf['file' 'data']: записывает данные в файл
rf['file']: читает файл
lvl['file' 'p|u|k']: устанавливает уровень доступа
clear: очищает экран
math var = a op b: выполняет арифметическую операцию
grep['pattern' 'file']: ищет строки по шаблону
cat['file']: выводит содержимое файла
help: показывает справку


Версия v0.1.fix.17 rls


Требования

Компилятор GCC
Стандартные библиотеки C

Лицензия
MIT License
