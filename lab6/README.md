# Лабораторная работа 6: Event-Driven архитектура (CQRS + Kafka)

```
Клиент
  │
  ▼
nginx:8080
  │
  ├──→ user-service:8081        ──→ PostgreSQL (users)
  │
  └──→ folder-file-service:8082 ──→ MongoDB (folders, files)
            │  POST /folders: publishes FolderCreated
            ▼
       Apache Kafka (topic: folder-events)
            │
            ▼
       folder-consumer ──→ MongoDB (записывает папку асинхронно)
```

## Введение CQRS и Kafka

**Apache Kafka** в режиме KRaft (без ZooKeeper), топик `folder-events`

**`folder-consumer`** — отдельный сервис, читает топик и записывает папки в MongoDB

**CQRS для `CreateFolder`**: write-сторона публикует событие в Kafka, read-сторона читает MongoDB

**Write-through cache**: `folder-file-service` немедленно обновляет TTL-кеш при создании папки — клиент видит результат без задержки, не дожидаясь consumer-а

Подробнее: `event_driven_design.md`, каталог событий: `event_catalog.md`

---

## Запуск

```bash
cd lab6
podman compose build && podman compose up
# или
docker compose build && docker compose up
```

Порядок старта:
1. `postgres-db` — PostgreSQL, инициализируется схемой и данными из lab3
2. `mongodb` — MongoDB, инициализируется тестовыми данными из lab4
3. `kafka` — Apache Kafka (KRaft), ждёт healthcheck
4. `kafka-init` — создаёт топик `folder-events` (1 партиция)
5. `user-service` — подключается к PostgreSQL
6. `folder-file-service` — подключается к MongoDB и Kafka (producer)
7. `folder-consumer` — подключается к MongoDB и Kafka (consumer)
8. `nginx` — роутинг, порт 8080

API: `http://localhost:8080/api/v1/`

Swagger UI: `http://localhost:8081`

---

## Тесты

```bash
bash lab6/tests/test_api.sh
```

52 теста: регистрация, логин, пользователи, папки (включая CQRS), файлы, каскадное удаление.

---

## Структура файлов

```
lab6/
├── docker-compose.yml          — полный стек: PostgreSQL + MongoDB + Kafka + 3 сервиса
├── Dockerfile                  — общий образ для всех трёх C++ сервисов
├── cloud-storage/
│   └── configs/
│       ├── folder-file-service/config.yaml   — конфиг сервиса + Kafka producer
│       └── folder-consumer/config.yaml       — конфиг consumer-а
├── configs/
│   ├── folder-file-service/config_vars.yaml
│   ├── folder-consumer/config_vars.yaml
│   └── secdist/secdist.json    — брокеры Kafka (credentials для producer и consumer)
├── tests/
│   └── test_api.sh
├── event_driven_design.md      — обоснование Kafka, конфигурация, CQRS-схема
└── event_catalog.md            — каталог всех событий системы
```
