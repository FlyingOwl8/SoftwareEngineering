# Лабораторная работа 2: Разработка REST API сервиса

## Задание

Выберите нужный вариант из файла `homework_variants.pdf` (варианты 1–24) и выполните следующие задачи:

### 1. Проектирование REST API

- Изучите выбранный вариант задания
- Спроектируйте REST API endpoints для всех операций из вашего варианта
- Используйте правильные HTTP методы (GET, POST, PUT, DELETE, PATCH)
- Используйте правильные HTTP статус-коды
- Спроектируйте структуру URL (ресурсы, вложенные ресурсы)
- Определите структуру Request/Response для каждого endpoint

### 2. Реализация REST API сервиса

- Реализуйте REST API сервис на выбранном языке и фреймворке (Python FastAPI, C++ Poco, Yandex Userver)
- Реализуйте минимум 5 API endpoints из вашего варианта задания
- Используйте in-memory хранилище (списки, словари) или простую БД (SQLite)
- Реализуйте обработку ошибок с правильными HTTP статус-кодами
- Используйте DTO (Data Transfer Objects) для передачи данных

### 3. Реализация аутентификации

- Реализуйте простую аутентификацию (можно использовать JWT токены или session-based)
- Защитите минимум 2 endpoint с помощью аутентификации
- Реализуйте endpoint для регистрации/логина пользователя
- Добавьте middleware для проверки аутентификации

### 4. Документирование API

- Создайте OpenAPI/Swagger спецификацию для вашего API
- Опишите все endpoints с параметрами, request/response схемами
- Добавьте примеры запросов и ответов
- Если возможно, добавьте Swagger UI для интерактивного тестирования API

### 5. Тестирование

- Создайте простые тесты для основных endpoints (можно использовать curl, Postman или unit-тесты)
- Протестируйте успешные сценарии
- Протестируйте обработку ошибок (невалидные данные, отсутствующие ресурсы и т.д.)

---

## Реализация

### Эндпоинты

| Метод | Путь | Авторизация | Описание |
|-------|------|-------------|----------|
| POST | `/api/v1/auth/register` | — | Регистрация пользователя |
| POST | `/api/v1/auth/login` | — | Вход, получение JWT-токена |
| GET | `/api/v1/users/{login}` | JWT (admin) | Получить пользователя по логину |
| GET | `/api/v1/users?first_name=&last_name=` | JWT (admin) | Поиск пользователей по маске имени |
| POST | `/api/v1/folders` | JWT | Создать папку |
| GET | `/api/v1/folders` | JWT | Список папок текущего пользователя |
| DELETE | `/api/v1/folders/{id}` | JWT | Удалить папку каскадно |
| POST | `/api/v1/folders/{id}/files` | JWT | Загрузить файл в папку |
| GET | `/api/v1/folders/{id}/files/{name}` | JWT | Получить файл по имени |
| DELETE | `/api/v1/folders/{id}/files/{fid}/delete` | JWT | Удалить файл |

### Структура кода

```
cloud-storage/src/
├── shared/             ← модели, JWT, исключения (общий код обоих сервисов)
├── user-service/
│   ├── handlers/       ← HTTP-хендлеры
│   ├── service/        ← бизнес-логика
│   └── repository/     ← IUserRepository + InMemoryUserRepository
└── folder-file-service/
    ├── handlers/
    ├── service/
    └── repository/     ← IFileSystemRepository + InMemoryFileSystemRepository
```

---

## Архитектура

```
                        ┌──▶  user-service:8081         /api/v1/auth/*, /api/v1/users/*
nginx:8080  ────────────┤
                        └──▶  folder-file-service:8082  /api/v1/folders/*
```

## Запуск

```bash
podman compose build && podman compose up
# или
docker compose build && docker compose up
```

Swagger UI: http://localhost:8081

## Тесты

```bash
bash tests/test_api.sh
```