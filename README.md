# quadric-intersections

Приложение с графическим интерфейсом для:

1. Построения **9 канонических поверхностей второго порядка** (эллипсоид,
   однополостный и двуполостный гиперболоиды, эллиптический и гиперболический
   параболоиды, конус, эллиптический, гиперболический и параболический
   цилиндры).
2. Триангуляции каждой поверхности **двумя независимыми методами** —
   Marching Cubes и параметрической сеткой с клиппингом по AABB —
   для сравнения погрешности и скорости.
3. Поиска **попарных линий пересечения** между сетками с замером времени и
   сборкой полилиний.
4. Хранения результатов в SQLite с просмотром таблицей и
   3D-визуализацией.

C++17 + Qt 6 + Eigen 3. Без других внешних зависимостей помимо vcpkg для Eigen
и GoogleTest.

## Скриншот

_(Скриншот появится после демо-запуска: окно с тремя вкладками — Эксперимент,
Результаты, 3D._

## Требования

- CMake ≥ 3.20
- Qt 6.5+ — системная установка. Компоненты: `Widgets`, `Sql`, `OpenGLWidgets`,
  `Concurrent`, `Test`
- Компилятор C++17: GCC 11+ (Linux) или MSVC 2022 (Windows)
- [vcpkg](https://github.com/microsoft/vcpkg) — для Eigen 3 и GoogleTest.
  Переменная окружения `VCPKG_ROOT` должна указывать на каталог vcpkg.

### Установка vcpkg (если ещё нет)

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh              # Linux
# или .\bootstrap-vcpkg.bat             # Windows
export VCPKG_ROOT=~/vcpkg
```

## Сборка

### Linux (GCC + Ninja)

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

Для Release-сборки заменить `debug` на `release`.

### Windows (MSVC 2022)

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

## Запуск

```bash
./build/linux-gcc-debug/quadric_intersections
```

При первом запуске рядом с бинарником создаётся `experiments.db` (SQLite).

## Использование

### Вкладка «Эксперимент»

1. Задайте AABB сцены (6 спинбоксов min/max).
2. Кнопка **`+`** добавляет surface; справа выбирается тип, параметры (`a`,
   `b`, `c`, `p` в зависимости от типа), Transform (translation + Эйлеровы
   углы XYZ), и метод триангуляции (parametric с `uSteps`/`vSteps` или
   marching_cubes с `resolution`).
3. Снизу — комбо метода пересечения (BVH или Naive), notes и кнопка **Run**.
4. **Run** запускает оркестратор в отдельном потоке через `QtConcurrent`,
   прогресс — в нижнем `QProgressBar`. После завершения — `QMessageBox` и
   автообновление двух других вкладок.
5. **Save…** / **Load…** сохраняют/загружают конфиг эксперимента в JSON-файл.

### Вкладка «Результаты»

- Внутренние вкладки **Intersection pairs** и **Experiments**.
- Pairs — таблица из JOIN трёх таблиц БД с сортировкой по любой колонке и
  фильтром по подстроке во всех колонках.
- Experiments — список экспериментов; двойной клик → диалог с подробностями
  (bbox, surfaces, intersections с временами).
- **Delete selected** — каскадно удаляет эксперимент.
- **Export CSV…** — экспорт текущей видимой таблицы (с учётом сортировки и
  фильтра).

### Вкладка «3D»

- Слева — список сохранённых экспериментов.
- Справа — `QOpenGLWidget` с орбитальной камерой:
  - **ЛКМ drag** — вращение,
  - **колесо** — зум,
  - **ПКМ drag** — пан.
- При выборе эксперимента сетки **пересчитываются заново** из параметров
  (в БД меши не хранятся, только числа и времена).

## Тесты

```bash
ctest --preset linux-gcc-debug
```

`--output-on-failure` включён по умолчанию в пресете. На момент финального
этапа: **155+ тестов**, ~60 секунд в Debug.

## Структура

| Путь                | Содержимое                                                                            |
|---------------------|---------------------------------------------------------------------------------------|
| `src/geometry/`     | Квадрики (Quadric + 9 классов), Transform, AABB. Чистый C++ + Eigen, **без Qt**.       |
| `src/mesh/`         | `Mesh`, `Polyline`, `Segment`. Чистый C++ + Eigen, **без Qt**.                         |
| `src/triangulation/`| Marching Cubes (LUT Бурка) и параметрический триангулятор с клиппингом по AABB. Чистый C++. |
| `src/intersection/` | `intersectTriangles` (Devillers-Guigue 2002), Naive и BVH, PolylineBuilder. Чистый C++. |
| `src/experiment/`   | `ExperimentConfig`, `ExperimentResult`, `ExperimentRunner`, `ConfigJson`. Qt6::Core.   |
| `src/storage/`      | `DatabaseManager`, `ExperimentRepository`. SQLite через Qt6::Sql.                     |
| `src/ui/`           | Все Qt-виджеты, `MainWindow`, `Viewport3D`, `OrbitalCamera`.                          |
| `tests/`            | Юнит- и интеграционные тесты (GoogleTest + Qt::Test для signal-spying).               |
| `rubbish/`          | Личные заметки автора: `PLAN.md`, `TESTING.md`, отчёты по каждому этапу.              |

## Алгоритмы

### Триангуляция

- **Marching Cubes** — классические LUT Пола Бурка (`edgeTable[256]` +
  `triTable[256][16]`, public domain). Сэмплируется неявная функция на сетке
  `(N+1)³` точек один раз, потом обход `N³` кубов с интерполяцией пересечений
  рёбер. Без дедупликации вершин на первом проходе. Вершинная погрешность
  ≈ `diagonal/N`.
- **Параметрический** — сетка `(uSteps+1) × (vSteps+1)` точек через
  `Quadric::parametric(u, v)`, каждый квад разбивается на 2 треугольника, потом
  клиппинг каждого треугольника по 6 плоскостям bbox через
  Сазерленд-Ходжмен и фан-триангуляция результата. Точность вершин
  до клиппинга — `kEpsTight = 1e-9`.

### Пересечение треугольников

Devillers & Guigue 2002, "Faster Triangle-Triangle Intersection Tests"
(INRIA RR-4488). Все ветвления — знаки `orient3d` (определитель степени 3).
По сравнению с Möller 1997: степень 3 vs 8, ~20–30 % быстрее в IEEE double,
2–3× меньше ложных срабатываний на почти-касательных конфигурациях.

Дополнительно перед основным алгоритмом — AABB-фильтр (Step 0): отсекает
численно-граничные ложные срабатывания, когда треугольники реально далеко.

### Пересечение мешей

- **Naive** — двойной цикл O(n·m) по треугольникам. Baseline.
- **BVH** — AABB-tree поверх треугольников меша A. Билд через `nth_element`
  по медиане центроидов на самой длинной оси, leaf size = 8. Запрос —
  range-query по AABB треугольника меша B. На сетках ~3000 треугольников
  каждая даёт **~320× ускорение** по сравнению с Naive (97 ms vs 31 s в
  Debug).

### Сборка полилиний

Уникальные точки (epsilon-merging) → граф рёбер → обход компонент связности
через DFS forward-then-backward от seed-ребра. Замкнутая полилиния
содержит повторённую первую точку в конце.

## Литература

- T. Möller. *A Fast Triangle-Triangle Intersection Test*.
  Journal of Graphics Tools, 2(2):25–30, 1997.
- O. Devillers, P. Guigue. *Faster Triangle-Triangle Intersection Tests*.
  INRIA Research Report RR-4488, 2002.
  [hal.inria.fr/inria-00072100](https://hal.inria.fr/inria-00072100)
- W. E. Lorensen, H. E. Cline. *Marching Cubes: A High Resolution 3D Surface
  Construction Algorithm*. SIGGRAPH '87.
- P. Bourke. *Polygonising a scalar field*. 1994.
  [paulbourke.net/geometry/polygonise](http://paulbourke.net/geometry/polygonise/)
- I. E. Sutherland, G. W. Hodgman. *Reentrant Polygon Clipping*.
  Communications of the ACM, 17(1):32–42, 1974.

## Лицензия и происхождение

Учебный проект. Реализация LUT-таблиц Marching Cubes — public domain
(Paul Bourke). Реализация Devillers-Guigue — оригинальная, по статье
RR-4488 (формула определителя в Definition 2.2 § 3.1).
