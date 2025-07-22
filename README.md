# Чат-приложение

[Doxygen](https://ya-masterskaya-cpp.github.io/q2_2025_chat_project_team_3/index.html)

## Быстрый старт

### Запуск и отладка

1. **Devcontainer**  
   Рекомендуется открывать проект в devcontainer через VS Code.
   В обычной VIsual Studio тоже откроется и будет без проблем работать, но документация описывает именено devcontainer

2. **Сборка**  
   Пока собирать через CMake-плагин в VS Code.  
   - F1 -> "CMake: Select Configure Preset"
   - F1 -> "CMake: Select Build Preset"
   - F1 -> "CMake: Build"

3. **Запуск**
   - бинарники лежат по `build/{server,client,aggregator}/{Release,Debug}/app_{server,client,aggregator}`
   - Аггрегатор слушает порт `8848` по умолчанию.
   - Сервер слушает порт `8849` по умолчанию.
   - Подключение к WebSocket:  
     ```
     ws://localhost:8848/ws
     ```
   - Логи сервера можно найти в директории:
     ```
     build/server/{Release,Debug}/logs
     ```
   - Логи клиента пока пишутся только в консоль

4. **Дебаг**
   - F1 -> "CMake: Select Configure Preset"
   - F1 -> "CMake: Select Build Preset"
   - F1 -> "CMake: Set Launch/Debug target"
   - F1 -> "CMake: Debug"

5. **Скрипты**  
   В папке `scripts` есть вспомогательные утилиты:
   - `psql.sh` — быстрое подключение к тестовой БД (PostgreSQL).
   - `generate_models.sh` — автогенерация исходников моделей для ORM (запускать при изменении схемы БД).

---

## Запуск нескольких серверов "в один клик"

Скопируйте себе examples/docker-compose.yml
 - docker compose up -d

В том compose файле пример поднимающий 1 аггрегатор и 6 серверов для него

### Env переменные
 - `AGGREGATOR_ADDR` - адресс агррегатора к которому подключится сервер
 - `SERVER_HOST` - хост по которому будет доступен сервер извне, там может быть йпишник или доменное имя, как с портом тк и без

в examples/docker-compose.yml есть примеры этих переменных

## Формат сообщений

- Используется protobuf. Сообщения можно глянуть в `common/protobuf/chat.proto`

---

## Используемые технологии и зависимости

- **C++23**
- **CMake** для сборки
- **Drogon**
- **PostgreSQL**
- **WebSocket сервер\склиент**  
- **ORM для моделей** (автогенерация через скрипт)
- **Devcontainer** для унификации среды разработки
- **WxWidgets**
- **Protocol Buffers**

---

## Дополнительно

_Если у вас есть вопросы по формату сообщений или процессу разработки, обращайтесь к PR [#1](https://github.com/ya-masterskaya-cpp/q2_2025_chat_project_team_3/pull/1) и комментариям к нему._
