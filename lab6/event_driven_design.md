# Event-Driven архитектура: Cloud Storage

## Выбор Apache Kafka

В паттерне CQRS read-модель (MongoDB) является производной от потока событий, а не источником истины. Если коллекция `folders` в MongoDB окажется повреждена или нужен второй consumer (например, поисковый индекс), достаточно сбросить offset и прочитать топик заново. С RabbitMQ это невозможно — сообщения удаляются сразу после обработки. Для синхронизации read-модели Kafka семантически точнее.

---

## Конфигурация Kafka

### Topic

`folder-events`. Партиции - 1. Retention - 7 дней (default)

### Формат сообщений

JSON (UTF-8)

Ключ: `folder.id` (UUID) — все события одной папки попадают в одну партицию, порядок гарантирован

### Гарантии доставки

**at-least-once** — сообщение доставляется минимум один раз, при сбоях возможна повторная доставка. At-least-once безопасен, потому что consumer идемпотентен. Идемпотентность обеспечивается совместно сервисом и MongoDB:

- UUID папки генерируется в `FileSystemService::GenerateUuid()` — на стороне сервиса, до любого обращения к БД — и включается в payload события.
- Consumer записывает этот UUID как `_id` документа: `doc["_id"] = folder.id`.
- MongoDB гарантирует уникальность `_id` на уровне хранилища — это встроенное ограничение, не настраиваемое.
- При повторной доставке того же события `InsertOne` с уже существующим `_id` бросает `DuplicateKey` exception, который перехватывается в `try-catch` в `ProcessBatch` и логируется как ERROR. `AsyncCommit()` вызывается безусловно после цикла — offset продвигается.

Итого папка оказывается в MongoDB ровно один раз независимо от числа доставок события.

### Producer (`FolderFileService`)
`delivery_timeout: 3s` — таймаут подтверждения от брокера

`enable_idempotence: false` — идемпотентность producer (дедупликация ретраев на уровне Kafka) не нужна, идемпотентность обеспечена на уровне consumer + MongoDB

### Consumer (`folder-consumer`)
`AsyncCommit()` вызывается после обработки батча безусловно

При сбое до commit — offset не сдвигается, сообщение будет обработано повторно

---

## CQRS для CreateFolder

`POST /api/v1/folders` → **FolderFileService**:

1. Проверяет дублирование имени: если кеш для `owner_id` есть — ищет в нём; иначе вызывает `FolderNameExists` → MongoDB
2. Генерирует UUID, заполняет `models::Folder`
3. Публикует JSON-событие в Kafka topic `folder-events`
4. Write-through: если кеш есть — берёт список из кеша; иначе подгружает из MongoDB (`FindFoldersByOwner`). Добавляет новую папку и сохраняет обратно в кеш
5. Возвращает `HTTP 201` с данными папки

Прямой записи в MongoDB **не происходит** — это делает consumer асинхронно.


`GET /api/v1/folders` → **FolderFileService**:

1. Проверяет TTL-кеш (`folders_cache_.Get(owner_id)`, TTL = 30 с)
2. При промахе — читает из MongoDB (`FindFoldersByOwner`)
3. Сохраняет результат в кеш

### Синхронизация read- и write-модели

Используется паттерн **write-through cache**: при создании папки сервис немедленно обновляет кеш, не дожидаясь consumer-а.

```
POST /folders → Kafka (folder-events) ─┐
             → folders_cache_.Set()    │ consumer пишет в MongoDB async
                                       │ (~100–500 мс, initial rebalance до ~20 с)
GET /folders ← folders_cache_.Get()   │
              (папка видна мгновенно) ◄┘
```

Клиент видит созданную папку сразу после `POST`. Когда TTL кеша истечёт (30 с), следующий GET прочитает из MongoDB, куда consumer уже успел записать событие.

#### Таблица операций

| Тип | Операция | Путь данных |
|-----|----------|-------------|
| Command | `CreateFolder` | → Kafka → MongoDB (async) |
| Command | `DeleteFolder` | → MongoDB (direct) |
| Command | `CreateFile` | → MongoDB (direct) |
| Command | `DeleteFile` | → MongoDB (direct) |
| Query | `ListFolders` | TTL-кеш → MongoDB |
| Query | `FindFolder` | → MongoDB |
| Query | `GetFile` | → MongoDB |

Полный CQRS реализован только для `CreateFolder`. Остальные операции используют прямую синхронную запись.

---

## Поток событий

```
Клиент
  │
  │  POST /api/v1/folders {"name": "Documents"}
  ▼
nginx (8080)
  │
  ▼
FolderFileService
  │  1. кеш есть? → дубль в кеше; нет → FolderNameExists → MongoDB
  │  2. folder = {id, name, owner_id, created_at}
  │  3. producer_.Send("folder-events", folder.id, json)
  │  4. write-through: base = кеш ?? FindFoldersByOwner(MongoDB)
  │                    base.push_back(folder); cache.Set(owner_id, base)
  │
  ├──► HTTP 201 {"id": "...", "name": "Documents", ...} → Клиент
  │
  │  GET /api/v1/folders → folders_cache_.Get() → папка видна сразу
  │
  ▼
Apache Kafka (topic: folder-events)
  │
  ▼
folder-consumer (FolderConsumerHandler)
  │  ProcessBatch():
  │  1. десериализовать JSON → models::Folder
  │  2. repository_->SaveFolder(folder) → MongoDB
  │  3. consumer_scope_.AsyncCommit()
  │
  ▼
MongoDB (коллекция folders)
  (источник истины, становится актуальным асинхронно)
```
