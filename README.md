# Molecules from XYZ

Программа для просмотра молекул из .xyz файлов.
Модель «шары и стержни»: цвет и размер атома зависят от элемента,
связи определяются автоматически по расстоянию между атомами.

Написана на C++ с использованием OpenGL 2.1, GLFW и GLAD.

## Управление

- Левая кнопка мыши (зажать и двигать): вращение молекулы
- Колесо мыши: приближение и отдаление
- Клавиши 1-6: вода, метан, бензол, кофеин, аммиак, бромметан
- Перетаскивание .xyz файла в окно программы: загрузка этого файла
- ESC: выход

## Формат файла .xyz

3
Water
O 0.000000 0.000000 0.000000
H 0.758602 0.000000 0.504284
H -0.758602 0.000000 0.504284

Первая строка: число атомов. Вторая: комментарий.
Далее: символ элемента и три координаты в ангстремах.

## Поддерживаемые элементы

H, C, N, O, S, P, F, Cl, Br. Остальные элементы рисуются серо-фиолетовым шаром.
Новый элемент добавляется одной строкой в таблицу `elements` в начале main.cpp.

## Сборка

1. Установите vcpkg и зависимости (тройка x64-windows-static-md):
   glfw3, glad, glm.
2. Откройте проект как CMake-проект (например, в Visual Studio) и соберите.
3. Файлы .xyz ищутся рядом с программой или в папке C:/first_project/.
__________________________________________________________________________________________

# Molecules from XYZ

A program for viewing molecules from .xyz files.
Ball-and-stick model: atom colors and sizes depend on the element,
and bonds are automatically determined by the distances between atoms.

Written in C++ using OpenGL 2.1, GLFW, and GLAD.

## Controls

- Left mouse button (click and drag): rotate the molecule
- Mouse wheel: zoom in and out
- Keys 1-6: water, methane, benzene, caffeine, ammonia, bromomethane
- Drag and drop an .xyz file into the program window: load the file
- ESC: exit

## .xyz File Format

3
Water
O 0.000000 0.000000 0.000000
H 0.758602 0.000000 0.504284
H -0.758602 0.000000 0.504284

First line: number of atoms. Second line: comment.
Next lines: element symbol and three coordinates in angstroms.

## Supported Elements

H, C, N, O, S, P, F, Cl, Br. All other elements are displayed as grayish-purple spheres.
A new element can be added with a single line in the `elements` table at the beginning of main.cpp.

## Build Instructions

1. Install vcpkg and dependencies (triplet x64-windows-static-md):
   glfw3, glad, glm.
2. Open the project as a CMake project (for example, in Visual Studio) and build it.
3. .xyz files are searched for next to the program or in the C:/first_project/ directory.

