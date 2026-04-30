# quadric-intersections

Приложение с графическим интерфейсом для построения поверхностей второго порядка
(9 канонических квадрик), их триангуляции двумя методами (Marching Cubes и
параметрическим), поиска попарных линий пересечения с замером времени и
хранения результатов в SQLite.

Проект на C++17 + Qt 6 + Eigen 3. Подробный план развития — в
[rubbish/PLAN.md](rubbish/PLAN.md); описание тестовой стратегии — в
[rubbish/TESTING.md](rubbish/TESTING.md).

## Требования

- CMake ≥ 3.20
- Qt 6.5+ (компоненты `Widgets`, `Sql`, `OpenGLWidgets`) — системная установка
- Компилятор C++17: GCC 11+ (Linux) или MSVC 2022 (Windows)
- [vcpkg](https://github.com/microsoft/vcpkg) для Eigen3 и GoogleTest —
  переменная окружения `VCPKG_ROOT` должна указывать на каталог vcpkg

### Установка vcpkg (если ещё нет)

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh              # Linux
# или .\bootstrap-vcpkg.bat             # Windows
export VCPKG_ROOT=~/vcpkg               # добавить в ~/.bashrc / ~/.config/fish/
```

## Сборка

### Linux (GCC + Ninja)

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

Для Release: заменить `debug` на `release`.

### Windows (MSVC 2022)

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

## Запуск

```bash
./build/linux-gcc-debug/quadric_intersections
```

На первом этапе открывается окно с тремя пустыми вкладками: «Эксперимент»,
«Результаты», «3D».

## Тесты

```bash
ctest --preset linux-gcc-debug
```

Флаг `--output-on-failure` включён по умолчанию в пресете.

## Структура

| Путь                | Содержимое                                            |
|---------------------|-------------------------------------------------------|
| `src/geometry/`     | Квадрики, трансформы, AABB (чистый C++)               |
| `src/mesh/`         | Mesh, Polyline, Segment (чистый C++)                  |
| `src/triangulation/`| Marching Cubes и параметрический триангулятор         |
| `src/intersection/` | Пересечение треугольников, сборка полилиний           |
| `src/storage/`      | Репозиторий SQLite (`qi::storage`, Qt6::Sql)          |
| `src/experiment/`   | Оркестрация, конфиг, `ExperimentResult`               |
| `src/ui/`           | Qt-виджеты, главное окно                              |
| `tests/`            | Юнит- и интеграционные тесты (GoogleTest)             |
