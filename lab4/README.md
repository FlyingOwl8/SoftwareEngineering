# Лабораторная работа 4: MongoDB

## Стек сервиса

```
Клиент
  │
  ▼
nginx:8080
  │
  ├──→ user-service:8081  ──→ PostgreSQL (users)
  │
  └──→ folder-file-service:8082  ──→ MongoDB (folders, files)
```

### Как применяются docker-compose файлы разных лабораторных

| Файл | Что запускает | Когда использовать |
|------|--------------|-------------------|
| `lab2/docker-compose.yml` | оба сервиса in-memory + nginx | тестирование без БД |
| `lab3/docker-compose.yml` | PostgreSQL + user-service с PostgreSQL + folder-file-service in-memory | тестирование только PostgreSQL |
| `lab4/docker-compose.yml` | PostgreSQL + MongoDB + оба сервиса с БД + nginx | **полный стек** |

Каждый следующий `docker-compose.yml` расширяет предыдущий, добавляя новую БД.
Все три разделяют одну кодовую базу из `lab2/cloud-storage/src/` — меняется только
монтируемый `config_vars.yaml`, который включает нужный репозиторий.

---

## Запуск

```bash
cd lab4
podman compose build && podman compose up
# или
docker compose build && docker compose up
```

Порядок старта:
1. `postgres-db` — PostgreSQL, загружает `lab3/schema.sql` и `lab3/data.sql`
2. `mongodb` — MongoDB, загружает `data.js` (папки + файлы)
3. `user-service` — подключается к PostgreSQL (`postgres_enabled: true` в config_vars)
4. `folder-file-service` — подключается к MongoDB (`mongo_enabled: true` в config_vars)
5. `nginx` — роутинг, порт 8080

API: `http://localhost:8080/api/v1/`

Swagger UI: `http://localhost:8081`

---

## Коллекции MongoDB

### `folders`

```js
{
  _id: "10000000-0000-4000-a000-000000000001",  // UUID (string)
  name: "Documents",
  owner_id: "00000000-0000-4000-a000-000000000002",  // ref → PostgreSQL users.id
  created_at: ISODate("2025-01-10T08:00:00Z")
}
```

Индексы: `{ owner_id: 1 }`, `{ owner_id: 1, name: 1 }` unique

### `files`

```js
{
  _id: "20000000-0000-4000-a000-000000000001",
  name: "readme.txt",
  folder_id: "10000000-0000-4000-a000-000000000001",  // ref → folders._id
  owner_id: "00000000-0000-4000-a000-000000000002",
  content_type: "text/plain",
  size: 42,           // int64, >= 0
  content: "SGVsbG8=",  // Base64
  created_at: ISODate("2025-01-10T08:10:00Z")
}
```

Индексы: `{ folder_id: 1, name: 1 }` unique, `{ owner_id: 1 }`

---

## Структура файлов

```
lab4/
├── schema_design.md   — документная модель, embedded vs references
├── data.js            — тестовые данные (12 папок, 13 файлов + индексы)
├── queries.js         — CRUD с $in, $gt, $lt, $ne, $and, $regex, $set
├── validation.js      — $jsonSchema + 5 тестов валидации
├── docker-compose.yml — полный стек: PostgreSQL + MongoDB + оба сервиса
├── configs/
│   └── folder-file-service/config_vars.yaml  — mongo_enabled: true
└── README.md
```

---

## Переменные окружения и конфигурация

Выбор хранилища задаётся через монтируемые `config_vars.yaml`:

| Сервис | Файл | Ключ |
|--------|------|------|
| user-service | `lab3/configs/user-service/config_vars.yaml` | `postgres_enabled: true` |
| folder-file-service | `lab4/configs/folder-file-service/config_vars.yaml` | `mongo_enabled: true` |

Если файл не монтируется — используется дефолт образа (`*_enabled: false`, in-memory).
