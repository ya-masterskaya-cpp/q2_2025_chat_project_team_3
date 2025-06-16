# 🧾 Протокол обмена между клиентом и сервером

## 📡 Общие сведения
Протокол основан на WebSocket-соединении. Передача сообщений осуществляется в формате JSON.

---

## 🔐 Аутентификация

### 1. auth

**Описание:** Авторизация пользователя

**Запрос:**
```json
{
  "channel": "client2server",
  "type": "auth",
  "data": {
    "username": "some_username",
    "password": "some_password"
  }
}
```

**Ответ:**
```json
{
  "channel": "server2client",
  "type": "auth",
  "data": {
    "success": true
  }
}
```

---

### 2. register

**Описание:** Регистрация нового пользователя

**Запрос:**
```json
{
  "channel": "client2server",
  "type": "register",
  "data": {
    "username": "some_username",
    "password": "some_password"
  }
}
```

**Ответ:**
```json
{
  "channel": "server2client",
  "type": "register",
  "data": {
    "success": true
  }
}
```

---

## 💬 Обмен сообщениями

### 3. sendMessage

**Описание:** Отправка текстового сообщения в текущую комнату

**Запрос:**
```json
{
  "channel": "client2server",
  "type": "sendMessage",
  "data": {
    "message": "Hello, world!"
  }
}
```

**Ответ:**
```json
{
  "channel": "server2client",
  "type": "sendMessage",
  "data": {
    "success": true
  }
}
```

---

## 🏠 Работа с комнатами

### 4. getRooms

**Описание:** Запрос списка доступных комнат

**Запрос:**
```json
{
  "channel": "client2server",
  "type": "getRooms"
}
```

**Ответ:**
```json
{
  "channel": "server2client",
  "type": "getRooms",
  "data": {
    "rooms": ["room1", "room2", "room3"],
    "success": true
  }
}
```

---

### 5. createRoom

**Описание:** Создание новой комнаты

**Запрос:**
```json
{
  "channel": "client2server",
  "type": "createRoom",
  "data": {
    "roomName": "new_room"
  }
}
```

**Ответ:**
```json
{
  "channel": "server2client",
  "type": "createRoom",
  "data": {
    "success": true
  }
}
```

---

### 6. joinRoom

**Описание:** Присоединение к существующей комнате

**Запрос:**
```json
{
  "channel": "client2server",
  "type": "joinRoom",
  "data": {
    "room": "exist_room"
  }
}
```

**Ответ:**
```json
{
  "channel": "server2client",
  "type": "joinRoom",
  "data": {
    "success": true
  }
}
```

---

### 7. leaveRoom

**Описание:** Покинуть текущую комнату

**Запрос:**
```json
{
  "channel": "client2server",
  "type": "leaveRoom"
}
```

**Ответ:**
```json
{
  "channel": "server2client",
  "type": "leaveRoom",
  "data": {
    "success": true
  }
}
```

---
