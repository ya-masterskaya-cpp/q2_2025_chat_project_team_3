# Чат-приложение

## Быстрый старт

### Запуск и отладка

1. **Devcontainer**  
   Рекомендуется открывать проект в devcontainer через VS Code.  
   Встроенный CMake Preset пока поддерживает обычный debug.

2. **Сборка**  
   Пока собирать через CMake-плагин в VS Code.  
   - F1 -> "CMake: Select Configure Preset"  
   - F1 -> "CMake: Build"

3. **Запуск**
   - бинарники лежат по `build/linux-debug-{server,client}/{server,client}/app_{server,client}`
   - Сервер слушает порт `8848`.
   - Подключение к WebSocket:  
     ```
     ws://localhost:8848/ws
     ```
   - Логи сервера можно найти в директории:
     ```
     build/linux-debug-server/server/logs
     ```
   - Логи клиента пока пишутся только в консоль

4. **Дебаг**
   - F1 -> "CMake: Select Configure Preset"  
   - F1 -> "CMake: Debug"

5. **Скрипты**  
   В папке `scripts` есть вспомогательные утилиты:
   - `psql.sh` — быстрое подключение к тестовой БД (PostgreSQL).
   - `generate_models.sh` — автогенерация исходников моделей для ORM (запускать при изменении схемы БД).

---

## Формат сообщений

- Используется protobuf. Сообщения можно глянуть в `common/protobuf/chat.proto`

---

## Используемые технологии и зависимости

- **C++23**
- **CMake** для сборки
- **PostgreSQL**
- **WebSocket сервер\склиент**  
- **ORM для моделей** (автогенерация через скрипт)
- **Devcontainer** для унификации среды разработки
- **WxWidgets**
- **Protocol Buffers**

---

## Дополнительно

_Если у вас есть вопросы по формату сообщений или процессу разработки, обращайтесь к PR [#1](https://github.com/ya-masterskaya-cpp/q2_2025_chat_project_team_3/pull/1) и комментариям к нему._
