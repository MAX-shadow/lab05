# Отчет по лабораторной работе №5

**Студент:** MAX-shadow
**Тема:** Изучение фреймворков модульного тестирования на примере GTest

---

## 1. Цель работы

Освоить фреймворк Google Test (GTest) и инструмент создания mock-объектов
Google Mock (GMock) для модульного тестирования C++ кода. Покрыть тестами
библиотеку `banking` (классы `Account` и `Transaction`), добившись 100%
покрытия по строкам, и настроить непрерывную интеграцию со сбором отчёта
о покрытии.

> Примечание: вместо Travis CI (стал платным, перестал поддерживать
> бесплатную сборку публичных репозиториев) используется GitHub Actions,
> как и в остальных работах курса. Отчёт о покрытии отправляется в
> Coveralls.io.

---

## 2. Ход выполнения работы

### 2.1. Подготовка проекта

За основу взят проект из лабораторной работы №4:

```sh
git clone https://github.com/MAX-shadow/lab04 lab05
cd lab05
git remote set-url origin https://github.com/MAX-shadow/lab05
```

### 2.2. Подключение Google Test

GTest добавлен как подмодуль Git. В инструкции предлагалась версия
`release-1.8.1`, но она не собирается современными компиляторами и не
содержит нового синтаксиса `MOCK_METHOD`, поэтому подключён актуальный
googletest (в нём GTest и GMock лежат вместе):

```sh
mkdir third-party
git submodule add https://github.com/google/googletest third-party/gtest
```

### 2.3. Библиотека banking

Библиотека состоит из двух классов. Методы `Account`, от которых зависит
`Transaction`, объявлены `virtual` — это позволяет подменить их
mock-объектом в тестах. Метод `Transaction::SaveToDataBase` также
виртуальный, чтобы в тестах не обращаться к «базе данных».

`banking/CMakeLists.txt`:

```cmake
add_library(banking STATIC Account.cpp Transaction.cpp)
target_include_directories(banking PUBLIC .)

install(TARGETS banking ARCHIVE DESTINATION lib)
install(FILES Account.h Transaction.h DESTINATION include)
```

### 2.4. Модульные тесты с mock-объектами

Тесты разделены на два файла в каталоге `tests/`.

`tests/test_account.cpp` проверяет сам класс `Account` на настоящих
объектах: конструктор, запрет изменения баланса без блокировки, изменение
после блокировки, запрет повторной блокировки и повторную блокировку
после разблокировки.

`tests/test_transaction.cpp` проверяет `Transaction`. Зависимость от
`Account` подменяется mock-объектом, а сам метод `SaveToDataBase`
подменяется через наследника `MockTransaction`:

```cpp
class MockAccount : public Account {
 public:
  MockAccount(int id, int balance) : Account(id, balance) {}

  MOCK_METHOD(int, GetBalance, (), (const, override));
  MOCK_METHOD(void, ChangeBalance, (int diff), (override));
  MOCK_METHOD(void, Lock, (), (override));
  MOCK_METHOD(void, Unlock, (), (override));
};

class MockTransaction : public Transaction {
 public:
  MOCK_METHOD(void, SaveToDataBase, (Account& from, Account& to, int sum),
              (override));
};
```

С помощью `EXPECT_CALL` проверяется, что перевод блокирует оба счёта
(RAII-`Guard`), зачисляет сумму получателю, списывает сумму с комиссией и
при нехватке средств откатывает зачисление. Покрыты все ветки метода
`Make`: одинаковые счета, отрицательная сумма, слишком малая сумма,
слишком большая комиссия, успешный перевод и откат при недостатке средств.

### 2.5. Корневой CMakeLists.txt

Тесты собираются только при включённой опции `BUILD_TESTS`, инструментация
покрытия — при `COVERAGE`:

```cmake
option(BUILD_TESTS "Build tests" OFF)
option(COVERAGE "Enable coverage instrumentation" OFF)

if(COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

add_subdirectory(banking)

if(BUILD_TESTS)
    enable_testing()
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(third-party/gtest)

    add_executable(check tests/test_account.cpp tests/test_transaction.cpp)
    target_link_libraries(check banking gtest_main gmock)
    target_include_directories(check PRIVATE banking)

    add_test(NAME check COMMAND check)
endif()
```

Сборка и запуск тестов локально:

```sh
cmake -H. -B_build -DBUILD_TESTS=ON
cmake --build _build
ctest --test-dir _build --output-on-failure
```

---

## 3. Домашнее задание

Задание: создать `CMakeLists.txt` для библиотеки `banking`, написать
модульные тесты на `Transaction` и `Account` с использованием
mock-объектов (покрытие 100%), настроить сборочную процедуру и Coveralls.io.

### 3.1. Непрерывная интеграция

Файл `.github/workflows/ci.yml`. Job `build` собирает проект и прогоняет
тесты на матрице компиляторов gcc/clang. Job `coverage` собирает проект с
инструментацией, прогоняет тесты, формирует отчёт `lcov` только по
библиотеке `banking` и отправляет его в Coveralls:

```yaml
  - name: Collect coverage
    run: |
      lcov --capture --directory _build --output-file coverage.info \
           --rc lcov_branch_coverage=0 --gcov-tool gcov
      lcov --extract coverage.info '*/banking/*' --output-file coverage.info
      lcov --list coverage.info

  - name: Upload to Coveralls
    uses: coverallsapp/github-action@v2
    with:
      file: coverage.info
      format: lcov
```

### 3.2. Coveralls.io

Репозиторий `MAX-shadow/lab05` подключён к Coveralls.io через GitHub.
После каждой сборки в `master` отчёт о покрытии загружается автоматически,
а в `README.md` выведен бейдж с актуальным процентом.

---

## 4. Результаты

- Библиотека `banking` собрана статической библиотекой через CMake.
- Написаны 13 модульных тестов (5 на `Account`, 8 на `Transaction`),
  все проходят на gcc и clang.
- Использованы mock-объекты (`MockAccount`, `MockTransaction`).
- Покрытие по строкам — 100% (`Account.cpp`, `Account.h`,
  `Transaction.cpp`, `Transaction.h`), подтверждено отчётом `lcov`:

```
                      |Lines       |Functions  |Branches
Filename              |Rate     Num|Rate    Num|Rate     Num
============================================================
Account.cpp           | 100%     13|85.7%     7|    -      0
Account.h             | 100%      1| 100%     1|    -      0
Transaction.cpp       | 100%     33|88.9%     9|    -      0
Transaction.h         | 100%      2| 100%     2|    -      0
============================================================
                Total:| 100%     49|89.5%    19|    -      0
```

- Настроены GitHub Actions (сборка + тесты + покрытие) и Coveralls.io.
