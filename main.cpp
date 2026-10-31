#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const float PI = 3.14159265f;

// Настройки
const float BOND_THICKNESS = 0.09f;     // радиус цилиндра связи
const float BOND_TOLERANCE = 1.25f;     // связь есть, если расстояние < tolerance * (r1 + r2)
const float MOUSE_SENSITIVITY = 0.5f;   // скорость вращения при перетаскивании мышью
const float MIN_ZOOM = 0.3f;            // минимальный масштаб
const float MAX_ZOOM = 4.0f;            // максимальный масштаб
const int SPHERE_STEPS = 20;            // гладкость шара
const int BOND_STEPS = 12;              // гладкость цилиндра связи

// Таблица элементов
struct ElementInfo {
    const char* symbol;
    float bondRadius;    // нужен для поиска связей
    float sphereRadius;  // размер шара на экране
    float r, g, b;       // цвет
};

const ElementInfo elements[] = {
    // символ  связь  шар     R      G      B
    { "H",     0.31f, 0.25f,  0.95f, 0.95f, 0.95f },  // белый
    { "C",     0.76f, 0.42f,  0.25f, 0.28f, 0.33f },  // тёмно-серый
    { "N",     0.71f, 0.40f,  0.20f, 0.35f, 0.95f },  // синий
    { "O",     0.66f, 0.38f,  0.95f, 0.12f, 0.12f },  // красный
    { "S",     1.05f, 0.50f,  1.00f, 0.85f, 0.10f },  // жёлтый
    { "P",     1.07f, 0.50f,  1.00f, 0.55f, 0.10f },  // оранжевый
    { "F",     0.57f, 0.36f,  0.56f, 0.88f, 0.31f },  // светло-зелёный
    { "Cl",    1.02f, 0.48f,  0.12f, 0.94f, 0.12f },  // зелёный
    { "Br",    1.20f, 0.52f,  0.65f, 0.16f, 0.16f },  // тёмно-красный
};

// Значения для всех элементов, которых нет в таблице.
const ElementInfo defaultElement = { "?", 0.75f, 0.42f, 0.65f, 0.55f, 0.75f };

// Ищет элемент в таблице по символу; если не нашли, возвращает defaultElement.
const ElementInfo& getElement(const string& symbol) {
    for (const ElementInfo& element : elements)
        if (symbol == element.symbol) return element;
    return defaultElement;
}

// Файлы молекул:
const char* moleculeFiles[] = {
    "water.xyz", "methane.xyz", "benzene.xyz",
    "caffeine.xyz", "ammonia.xyz", "bromomethane.xyz"
};
// Количество файлов в списке и номер файла
const int MOLECULE_COUNT = 6;

// Данные молекулы
struct Atom {
    string element;   // символ элемента
    float x, y, z;         // координаты
};

vector<Atom> atoms;                   // все атомы
vector<pair<int, int>> bonds;    // связи: пары номеров атомов

// Глобальные переменные для управления мышью
bool isDragging = false;       // зажата ли левая кнопка мыши
double lastMouseX = 0.0;       // прошлая позиция мыши
double lastMouseY = 0.0;
float rotX = 20.0f;            // углы поворота (в градусах)
float rotY = 25.0f;
float zoomLevel = 1.0f;        // масштаб (колесо мыши)
float cameraDistance = 8.0f;   // расстояние от камеры до молекулы

// Чтение XYZ: первая строка - число атомов, вторая - комментарий, затем элемент и x y z.
bool loadXYZ(const string& filename, GLFWwindow* window) {
    ifstream file(filename);

    // Если файл не нашёлся рядом с программой, ищем в папке проекта.
    if (!file.is_open()) {
        file.clear();
        file.open("C:/first_project/" + filename);
    }
    if (!file.is_open()) {
        cout << "File not found: " << filename << endl;
        return false;
    }

    int count;
    if (!(file >> count) || count < 1 || count > 1000) {
        cout << "Cannot read " << filename << endl;
        return false;
    }
    string comment;
    getline(file, comment); // Дочитываем первую строку.
    getline(file, comment); // Пропускаем строку-комментарий XYZ.

    // Сначала читаем во временный список, чтобы при ошибке не испортить текущую молекулу.
    vector<Atom> loaded;
    for (int i = 0; i < count; ++i) {
        Atom a;
        if (!(file >> a.element >> a.x >> a.y >> a.z)) {
            cout << "Invalid XYZ coordinates in " << filename << endl;
            return false;
        }
        loaded.push_back(a);
    }

    atoms = loaded;

    // Переносим центр молекулы в точку (0, 0, 0).
    float cx = 0, cy = 0, cz = 0;
    for (const Atom& a : atoms) { cx += a.x; cy += a.y; cz += a.z; }
    cx /= count; cy /= count; cz /= count;

    // Заодно находим радиус молекулы (самый далёкий атом от центра).
    float size = 0;
    for (Atom& a : atoms) {
        a.x -= cx; a.y -= cy; a.z -= cz;
        float d = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
        if (d > size) size = d;
    }
    // Чем больше молекула, тем дальше камера.
    cameraDistance = (size + 1.0f) * 3.0f;
    zoomLevel = 1.0f;

    // Автоматически находим связи по расстоянию и ковалентным радиусам.
    bonds.clear();
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            float dx = atoms[i].x - atoms[j].x;
            float dy = atoms[i].y - atoms[j].y;
            float dz = atoms[i].z - atoms[j].z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);
            float limit = BOND_TOLERANCE * (getElement(atoms[i].element).bondRadius +
                getElement(atoms[j].element).bondRadius);
            if (distance > 0.4f && distance < limit) bonds.push_back({ i, j });
        }
    }

    // Заголовок окна: имя файла, число атомов и связей.
    string title = filename + " - " + to_string(count) +
        " atoms, " + to_string(bonds.size()) + " bonds";
    glfwSetWindowTitle(window, title.c_str());
    return true;
}

// Шар, нарисованный несколькими полосками по широте.
void drawSphere(float radius) {
    for (int i = 0; i < SPHERE_STEPS; ++i) {
        float lat1 = -PI / 2 + PI * i / SPHERE_STEPS;        // нижняя граница полоски
        float lat2 = -PI / 2 + PI * (i + 1) / SPHERE_STEPS;  // верхняя граница полоски
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= SPHERE_STEPS; ++j) {
            float lon = 2 * PI * j / SPHERE_STEPS;           // долгота
            float x = cos(lon), z = sin(lon);
            // Нормаль направлена от центра шара наружу (нужна для освещения).
            glNormal3f(cos(lat1) * x, sin(lat1), cos(lat1) * z);
            glVertex3f(radius * cos(lat1) * x, radius * sin(lat1), radius * cos(lat1) * z);
            glNormal3f(cos(lat2) * x, sin(lat2), cos(lat2) * z);
            glVertex3f(radius * cos(lat2) * x, radius * sin(lat2), radius * cos(lat2) * z);
        }
        glEnd();
    }
}

// Тонкий цилиндр между двумя атомами (химическая связь).
void drawBond(const Atom& a, const Atom& b) {
    // Вектор от атома a к атому b и его длина.
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    float length = sqrt(dx * dx + dy * dy + dz * dz);
    if (length < 0.001f) return;

    glPushMatrix();                    // запоминаем систему координат
    glTranslatef(a.x, a.y, a.z);       // переносим начало координат в атом a
    // Цилиндр строится вдоль оси Z, поэтому поворачиваем его в сторону атома b.
    float angle = acos(dz / length) * 180.0f / PI;
    if (fabs(dx) + fabs(dy) > 0.0001f)
        glRotatef(angle, -dy, dx, 0);
    else if (dz < 0) glRotatef(180, 1, 0, 0);   // связь строго вдоль Z, но вниз

    glColor3f(0.70f, 0.70f, 0.73f);    // светло-серый цвет связи
    glBegin(GL_QUAD_STRIP);
    for (int k = 0; k <= BOND_STEPS; ++k) {
        float t = 2 * PI * k / BOND_STEPS, x = cos(t), y = sin(t);
        glNormal3f(x, y, 0);
        glVertex3f(BOND_THICKNESS * x, BOND_THICKNESS * y, 0);        // нижнее кольцо
        glVertex3f(BOND_THICKNESS * x, BOND_THICKNESS * y, length);   // верхнее кольцо
    }
    glEnd();
    glPopMatrix();                     // возвращаем систему координат
}

// Обработчики ввода

// Нажатие и отпускание левой кнопки мыши.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            // Запоминаем, где началось перетаскивание.
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        }
        else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

// Движение мыши: если кнопка зажата, поворачиваем молекулу.
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging) {
        // Насколько сдвинулась мышь с прошлого раза.
        double deltaX = xpos - lastMouseX;
        double deltaY = ypos - lastMouseY;

        rotY += (float)deltaX * MOUSE_SENSITIVITY;   // влево-вправо: вокруг оси Y
        rotX += (float)deltaY * MOUSE_SENSITIVITY;   // вверх-вниз: вокруг оси X

        lastMouseX = xpos;
        lastMouseY = ypos;
    }
}

// Колесо мыши: приближение и отдаление.
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    zoomLevel *= yoffset > 0 ? 1.1f : 0.9f;
    if (zoomLevel < MIN_ZOOM) zoomLevel = MIN_ZOOM;
    if (zoomLevel > MAX_ZOOM) zoomLevel = MAX_ZOOM;
}

// Клавиатура: ESC - выход, цифры - загрузка молекулы из списка.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);

    // Клавиши 1, 2, 3, ... загружают файлы из списка moleculeFiles.
    int index = key - GLFW_KEY_1;
    if (index >= 0 && index < MOLECULE_COUNT)
        loadXYZ(moleculeFiles[index], window);
}

// Перетаскивание файла в окно: загружаем его.
void dropCallback(GLFWwindow* window, int count, const char** paths) {
    if (count > 0) loadXYZ(paths[0], window);
}

// Изменение размера окна: обновляем область рисования и перспективу.
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (height < 1) height = 1;    // защита от деления на ноль
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float top = 0.1f * tan(45.0f * PI / 360.0f);   // угол обзора 45 градусов
    float right = top * width / height;                 // учитываем пропорции окна
    glFrustum(-right, right, -top, top, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    // 1. Инициализация GLFW
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // 2. Создание окна
    GLFWwindow* window = glfwCreateWindow(850, 650, "Molecules from XYZ", NULL, NULL);
    if (window == NULL) {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 3. Инициализация GLAD (загрузка функций OpenGL)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        glfwTerminate();
        return -1;
    }

    // 4. Регистрация обработчиков событий
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetDropCallback(window, dropCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Один раз настраиваем перспективу вручную (колбэк срабатывает только при изменении размера).
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebufferSizeCallback(window, width, height);

    // 5. Настройки OpenGL: тест глубины и простое освещение
    glEnable(GL_DEPTH_TEST);        // ближние объекты закрывают дальние
    glEnable(GL_LIGHTING);          // включаем освещение
    glEnable(GL_LIGHT0);            // включаем источник света 0
    glEnable(GL_COLOR_MATERIAL);    // glColor задаёт цвет материала
    glEnable(GL_NORMALIZE);         // нормали приводятся к единичной длине
    glClearColor(0.10f, 0.13f, 0.18f, 1.0f);

    // 6. Загрузка молекулы (имя файла из командной строки или water.xyz по умолчанию)
    loadXYZ(argc > 1 ? argv[1] : "water.xyz", window);

    // 7. Цикл отрисовки
    while (!glfwWindowShouldClose(window)) {
        // Очищаем экран и буфер глубины
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Свет задаём до поворотов, поэтому он неподвижен относительно камеры
        GLfloat light[] = { 5.0f, 8.0f, 10.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light);

        // Отодвигаем камеру (масштаб) и применяем повороты мышью
        glTranslatef(0.0f, 0.0f, -cameraDistance / zoomLevel);
        glRotatef(rotX, 1.0f, 0.0f, 0.0f);
        glRotatef(rotY, 0.0f, 1.0f, 0.0f);

        // Сначала рисуем связи, потом атомы
        for (const auto& bond : bonds)
            drawBond(atoms[bond.first], atoms[bond.second]);

        for (const Atom& a : atoms) {
            const ElementInfo& element = getElement(a.element);
            glPushMatrix();
            glTranslatef(a.x, a.y, a.z);                  // переносим в позицию атома
            glColor3f(element.r, element.g, element.b);   // цвет по таблице
            drawSphere(element.sphereRadius);             // размер по таблице
            glPopMatrix();
        }

        // Показываем нарисованный кадр и обрабатываем события
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 8. Завершение работы
    glfwTerminate();
    return 0;
}