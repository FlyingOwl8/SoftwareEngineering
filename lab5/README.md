# Лабораторная работа 5: Кеширование и Rate Limiting

```
Клиент
  │
  ▼
nginx:8080  (X-Real-IP → upstream)
  │
  ├──→ user-service:8081       ──→ PostgreSQL (users)
  │         ├── TtlCache<login, User>   (TTL 5 мин)
  │         ├── RateLimiter /auth/login   (20 req/min)
  │         └── RateLimiter /auth/register (10 req/min)
  │
  └──→ folder-file-service:8082 ──→ MongoDB (folders, files)
            └── TtlCache<owner_id, []Folder> (TTL 30 сек)
```

## Реализованные оптимизации

### Кеширование (Cache-Aside, header-only `TtlCache<K,V>`)

| Endpoint | Кеш | TTL | Инвалидация |
|----------|-----|-----|-------------|
| `GET /users/{login}` и логин | `user_cache_` по ключу `login` | 5 мин | при `POST /auth/register` |
| `GET /folders` | `folders_cache_` по ключу `owner_id` | 30 сек | при `POST /folders`, `DELETE /folders/{id}` |

Реализация: `lab2/cloud-storage/src/shared/cache/ttl_cache.hpp`
- Thread-safe через `std::shared_mutex`
- Устаревшие записи вытесняются при следующем `Get()`

### Rate Limiting (Token Bucket, header-only `RateLimiter`)

| Endpoint | Лимит | Burst | Ключ |
|----------|-------|-------|------|
| `POST /api/v1/auth/login` | 20 req/min | 20 | IP (`X-Real-IP`) |
| `POST /api/v1/auth/register` | 10 req/min | 10 | IP (`X-Real-IP`) |

Реализация: `lab2/cloud-storage/src/shared/ratelimit/token_bucket.hpp`

При превышении лимита возвращается: `429 Too Many Requests`

## Запуск

```bash
cd lab5
podman compose build && podman compose up
# или
docker compose build && docker compose up
```

API: `http://localhost:8080/api/v1/`

Swagger UI: `http://localhost:8081`

### Тесты

```bash
bash lab2/tests/test_api.sh
```

### Проверка rate limiting

```bash
# Отправить 25 запросов на /auth/login — первые 20 должны вернуть 401, последние 5 — 429
for i in $(seq 1 25); do
  curl -s -o /dev/null -w "%{http_code}\n" \
    -X POST http://localhost:8080/api/v1/auth/login \
    -H 'Content-Type: application/json' \
    -d '{"login":"nonexistent","password":"wrongpass"}'
done
```

### Проверка заголовков rate limiting

```bash
curl -v -X POST http://localhost:8080/api/v1/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"login":"admin","password":"wrong"}'
# Response headers:
# X-RateLimit-Limit: 20
# X-RateLimit-Remaining: 19
# X-RateLimit-Reset: <unix_timestamp>
```
