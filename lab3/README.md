# Лабораторная работа 3: PostgreSQL

## Стек сервиса

```
Клиент
  │
  ▼
nginx:8080
  │
  ├──→ user-service:8081  ──→ PostgreSQL (users)
  │
  └──→ folder-file-service:8082
```

### Как применяются docker-compose файлы разных лабораторных

| Файл | Что запускает | Когда использовать |
|------|--------------|-------------------|
| `lab2/docker-compose.yml` | оба сервиса in-memory + nginx | тестирование без БД |
| `lab3/docker-compose.yml` | PostgreSQL + user-service с PostgreSQL + folder-file-service in-memory | тестирование только PostgreSQL |

Новый `docker-compose.yml` расширяет предыдущий, добавляя новую БД.
Оба разделяют одну кодовую базу из `lab2/cloud-storage/src/` — меняется только
монтируемый `config_vars.yaml`, который включает нужный репозиторий.


## Схема базы данных

Одна таблица — `users`.

| Колонка | Тип | Ограничения |
|---------|-----|-------------|
| id | UUID | PK, DEFAULT gen_random_uuid() |
| login | VARCHAR(255) | NOT NULL, UNIQUE |
| first_name | VARCHAR(255) | NOT NULL |
| last_name | VARCHAR(255) | NOT NULL |
| password_hash | CHAR(64) | NOT NULL (SHA-256 hex) |
| role | VARCHAR(10) | NOT NULL, CHECK IN ('user','admin') |
| created_at | TIMESTAMPTZ | NOT NULL, DEFAULT NOW() |

---

## Индексы

| Индекс | Тип | Назначение |
|--------|-----|------------|
| `users_login_unique` | B-tree (auto) | Поиск по логину при входе и регистрации |
| `users_first_name_trgm_idx` | GIN trgm | `ILIKE '%...%'` поиск по имени |
| `users_last_name_trgm_idx` | GIN trgm | `ILIKE '%...%'` поиск по фамилии |

GIN-индексы используют расширение `pg_trgm`. Без них `ILIKE '%маска%'` делает Seq Scan
по всей таблице. Подробнее — в `optimization.md`.

---

## Запуск только lab3 (PostgreSQL + user-service)

```bash
cd lab3
podman compose build && podman compose up
# или
docker compose build && docker compose up
```

Запускает: PostgreSQL с таблицей `users` + user-service, подключённый к ней.
folder-file-service при этом работает с in-memory хранилищем (без MongoDB).

---

## Структура файлов

```
lab3/
├── schema.sql        — CREATE TABLE users + индексы
├── data.sql          — 15 пользователей (1 admin + 14 user)
├── queries.sql       — SQL для операций с пользователями
├── optimization.md   — EXPLAIN ANALYZE планы выполнения
├── docker-compose.yml
├── configs/
│   └── user-service/config_vars.yaml  — postgres_enabled: true
└── README.md
```

---

## Тестовые данные

15 пользователей: `admin` (роль admin) и 14 обычных пользователей.

## Подключение API

user-service из lab2 подключается к PostgreSQL через `postgres_enabled` в `config_vars.yaml`:
- `postgres_enabled: true` (монтируется из `lab3/configs/`) → `PostgresUserRepository`
- `postgres_enabled: false` (дефолт образа, режим lab2) → `InMemoryUserRepository`

folder-file-service работает с in-memory хранилищем (`mongo_enabled: false` по умолчанию).
