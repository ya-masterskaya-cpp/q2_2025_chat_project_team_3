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

### Рекомендации по тестированию

- Для тестирования WebSocket API удобнее всего использовать Postman (curl не подойдет).
- Примеры запросов и скриншоты доступны в комментариях к PR [#1](https://github.com/ya-masterskaya-cpp/q2_2025_chat_project_team_3/pull/1).

---

## Формат сообщений

Примеры JSON-сообщений для взаимодействия с сервером:

```json
{
  "channel": "client2server",
  "type": "register",
  "data": {
    "username": "alice",
    "password": "password123"
  }
}

{
  "channel": "client2server",
  "type": "auth",
  "data": {
    "username": "alice",
    "password": "password123"
  }
}

{
  "channel": "client2server",
  "type": "getUsers"
}

{
  "channel": "client2server",
  "type": "createRoom",
  "data": {
    "roomName": "lobby1"
  }
}

{
  "channel": "client2server",
  "type": "getRooms"
}

{
  "channel": "client2server",
  "type": "joinRoom",
  "data": {
    "room": "lobby1"
  }
}

{
  "channel": "client2server",
  "type": "leaveRoom"
}

{
  "channel": "client2server",
  "type": "sendMessage",
  "data": {
    "message": "hello world!"
  }
}
```

---

## Используемые технологии и зависимости

- **C++23**
- **CMake** для сборки
- **PostgreSQL**
- **WebSocket сервер\склиент**  
- **ORM для моделей** (автогенерация через скрипт)
- **Devcontainer** для унификации среды разработки
- **WxWidgets**

---

## Дополнительно

_Если у вас есть вопросы по формату сообщений или процессу разработки, обращайтесь к PR [#1](https://github.com/ya-masterskaya-cpp/q2_2025_chat_project_team_3/pull/1) и комментариям к нему._
