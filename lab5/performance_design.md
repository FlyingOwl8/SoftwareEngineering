# Проектирование производительности: кеширование и rate limiting

### Часто вызываемые операции

`FindUserByLogin`: 
-  Вызывается при каждом POST /auth/login. В системе, где пользователи периодически перелогиниваются (истечение JWT, мобильный клиент и т.д.), одни и те же данные пользователя запрашиваются повторно.
- Данные меняются редко (пользователь не меняет свой профиль в типичном сценарии)

`ListFolders`: 
Вызывается при каждом открытии «главной страницы» — отображение списка папок пользователя

### Медленные операции

Запросы к PostgreSQL (`FindUserByLogin`) и MongoDB (`FindFoldersByOwner`) из-за сетевого round-trip + I/O диска

---

## Стратегия кеширования

### Кешируемые данные

#### 1. `FindUserByLogin` — UserService

**Тип данных:** объект `User` (id, login, first_name, last_name, role, created_at)

**Стратегия:** Cache-Aside (Lazy Loading)

При запросе сначала проверяем кеш; если промах — идём в БД, сохраняем в кеш. При создании пользователя инвалидируем запись по ключу `login`.

**TTL:** 5 минут (`std::chrono::seconds(300)`)

**Ключ:** строка `login`

**Инвалидация:** явная, при `CreateUser` — вызов `user_cache_.Invalidate(login)`

#### 2. `ListFolders` — FileSystemService

**Тип данных:** `std::vector<Folder>` — список папок конкретного пользователя

**Стратегия:** Cache-Aside (Lazy Loading)
При запросе сначала проверяем кеш; если промах — идём в MongoDB, сохраняем.

**TTL:** 30 секунд (`std::chrono::seconds(30)`)

**Ключ:** `owner_id` (UUID пользователя)

**Инвалидация:** при `CreateFolder` и `DeleteFolder` — вызов `folders_cache_.Invalidate(owner_id)`

Папки создаются/удаляются чаще, чем обновляется профиль пользователя. Более короткий TTL, чем для пользователей (5 мин), снизит риск показа устаревшего списка.

### Реализация: `TtlCache<K, V>`

Заголовочный файл `src/shared/cache/ttl_cache.hpp`

Хранилище: `std::unordered_map<K, Entry>` где `Entry = {value, expires_at}`

При `Get` проверяется `steady_clock::now() > expires_at`, если устарело, возвращает `nullopt`

---

## Стратегия rate limiting

### Эндпоинты, требующие rate limiting

- `POST /api/v1/auth/login` - защита от брутфорса паролей
- `POST /api/v1/auth/register` - защита от спама регистраций / bot-флуда

### Алгоритм: Token Bucket

Допускает кратковременные всплески — нормальное поведение пользователя. Прост в реализации без внешних зависимостей (Redis не требуется). Плавное пополнение токенов, в отличие от Fixed Window (нет «reset-spike» в начале каждого окна).

**Параметры:**

| Endpoint | Лимит | Burst |
|----------|-------|-------|
| `POST /auth/login` | 20 req/min | 20 |
| `POST /auth/register` | 10 req/min | 10 |

**Единица ключа:** IP-адрес клиента (из заголовка `X-Real-IP`, проставляемого nginx)

**HTTP-ответ при превышении лимита**

- Статус: `429 Too Many Requests`
- Тело: `{"error": "Too Many Requests", "message": "Rate limit exceeded"}`
- Заголовки:
  - `X-RateLimit-Limit: <burst>` — максимальный размер окна
  - `X-RateLimit-Remaining: <tokens>` — оставшихся запросов в текущем окне
  - `X-RateLimit-Reset: <unix_timestamp>` — когда bucket пополнится до burst

Заголовки возвращаются **при каждом запросе** (не только при 429), что позволяет клиенту управлять частотой запросов.

---

## Как кеширование улучшает производительность

Без кеша каждый запрос `FindUserByLogin` делает сетевой запрос до PostgreSQL, каждый `ListFolders` — до MongoDB. При этом данные читаются намного чаще, чем меняются: пользователь логинится, открывает список папок, переходит между ними — каждое действие генерирует одни и те же запросы к БД.

Cache-Aside позволяет обслуживать повторные запросы из памяти процесса. Время доступа к `std::unordered_map` на несколько порядков меньше сетевого обращения к БД, и снижается количество запросов к PostgreSQL и MongoDB, что даёт им больше ресурсов для операций записи и сложных запросов.

Rate limiting улучшает производительность системы в целом: при флуде на `/auth/login` без ограничений каждый запрос доходит до БД (поиск пользователя + сравнение хеша пароля). Rate limiter отсекает избыточную нагрузку до обращения к хранилищу.

## Метрики для мониторинга

**Кеш:**

**Cache hit rate** — доля запросов, обслуженных из кеша: `hits / (hits + misses)`. Чем выше — тем меньше нагрузка на БД. Измеряется счётчиками внутри метода `Get()`.

**Latency до и после** — время ответа эндпоинта при cache hit vs cache miss. Разница показывает реальную стоимость обращения к БД в данном окружении.

**DB query rate** — количество запросов в секунду к PostgreSQL/MongoDB. При работающем кеше должно снижаться пропорционально hit rate.

**Rate limiting:**

**Количество 429-ответов** — показывает, насколько часто лимит срабатывает. Аномальный рост — признак атаки или неправильно выбранного лимита.

**Распределение по IP** — какие адреса чаще всего получают 429, помогает отличить атаку от легитимного клиента с высокой нагрузкой.

## Что реализовано для измерений

Для получения реальных данных о работе кеша в `TtlCache<K,V>` были добавлены
атомарные счётчики обращений:

```cpp
// src/shared/cache/ttl_cache.hpp
mutable std::atomic<long long> hits_{0};
mutable std::atomic<long long> misses_{0};

std::optional<V> Get(const K& key) const {
    std::shared_lock lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end() || steady_clock::now() > it->second.expires_at) {
        ++misses_;
        return std::nullopt;
    }
    ++hits_;
    return it->second.value;
}

Stats GetStats() const {
    long long h = hits_.load(), m = misses_.load();
    long long total = h + m;
    return {h, m, total > 0 ? static_cast<double>(h) / total : 0.0};
}
```

Использование `std::atomic` гарантирует потокобезопасность без дополнительного
мьютекса: операция `++` на атомарном типе атомарна по определению.

Затем созданы два HTTP-эндпоинта для чтения статистики в реальном времени:

| Эндпоинт | Сервис | Кеш |
|----------|--------|-----|
| `GET /api/v1/users/cache-stats` | user-service | `FindUserByLogin` |
| `GET /api/v1/folders/cache-stats` | folder-file-service | `ListFolders` |

Оба возвращают JSON вида:
```json
{"cache": "user_by_login", "hits": 22, "misses": 2, "hit_rate": 0.917}
```

---

## Методология измерений

Все измерения выполнялись локально: сервисы запущены через `podman compose` в
контейнерах на одном хосте. Время ответа фиксировалось через `curl -w
"%{time_total}"` — это сквозное время от отправки запроса до получения
последнего байта ответа (включает TCP, nginx proxy, обработку в сервисе).

### Измерение кеша пользователей (`FindUserByLogin`)

`FindUserByLogin` вызывается при каждом `POST /auth/login`: сервис сначала
ищет пользователя в БД (или кеше), затем сравнивает хеш пароля.

Шаги:
1. Запустить сервисы (`podman compose up -d`), убедиться в чистом кеше:
   `GET /api/v1/users/cache-stats` → `hits=0, misses=0`.
2. Выполнить `POST /auth/login` для alice — первый вызов, кеш пуст:
   **cache miss**, запрос уходит в PostgreSQL.
3. Проверить статистику: `misses=1, hits=0`.
4. Повторить `POST /auth/login` для alice ещё 20 раз — данные уже в кеше:
   **cache hit**, PostgreSQL не затрагивается.
5. Считать итоговую статистику.

Команды, которые выполнялись:
```bash
# Шаг 1: проверить чистый кеш
curl -s http://localhost:8080/api/v1/users/cache-stats

# Шаг 2-4: последовательные login-запросы
for i in $(seq 1 20); do
  curl -w "%{time_total}\n" -s -X POST http://localhost:8080/api/v1/auth/login \
    -H 'Content-Type: application/json' \
    -d '{"login":"alice","password":"password123"}' -o /dev/null
done

# Шаг 5: итоговая статистика
curl -s http://localhost:8080/api/v1/users/cache-stats
```

### Измерение кеша папок (`ListFolders`)

`ListFolders` вызывается при `GET /api/v1/folders` — главная страница файлового
менеджера, загружает список папок текущего пользователя из MongoDB.

Шаги:
1. Перезапустить folder-file-service и nginx для сброса кеша в памяти.
2. Выполнить первый `GET /api/v1/folders` — **cache miss**, запрос к MongoDB.
3. Выполнить второй и третий `GET /api/v1/folders` — **cache hit**, из памяти.
4. Считать статистику.

Команды:
```bash
TOKEN=$(curl -s -X POST http://localhost:8080/api/v1/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"login":"alice","password":"password123"}' | sed 's/.*"token":"\([^"]*\)".*/\1/')

# miss
curl -w "time_total: %{time_total}s\n" -s http://localhost:8080/api/v1/folders \
  -H "Authorization: Bearer $TOKEN" -o /dev/null

# hit
curl -w "time_total: %{time_total}s\n" -s http://localhost:8080/api/v1/folders \
  -H "Authorization: Bearer $TOKEN" -o /dev/null

curl -s http://localhost:8080/api/v1/folders/cache-stats
```

---

## Полученные результаты

### Кеш пользователей

После 22 обращений к `FindUserByLogin` (1 для alice + 1 для bob = 2 cold miss,
затем 20 повторных для alice):

```
GET /api/v1/users/cache-stats
→ {"cache":"user_by_login","hits":22,"misses":2,"hit_rate":0.9166}
```

**Hit rate: 91.7%** — при 24 запросах 22 были обслужены из памяти без
обращения к PostgreSQL.

Время ответа `POST /auth/login` при последовательных запросах (все локально,
ms округлены):

| № запроса | Время (с) | Тип |
|-----------|-----------|-----|
| 1 (alice) | 0.0044    | miss |
| 2         | 0.0027    | hit  |
| 3         | 0.0034    | hit  |
| 4         | 0.0045    | hit  |
| 5         | 0.0030    | hit  |
| ...       | 0.001–0.005 | hit |
| 20        | 0.0018    | hit  |

### Кеш папок

При трёх последовательных `GET /api/v1/folders` сразу после сброса кеша:

| Запрос | Время (с) | Тип |
|--------|-----------|-----|
| 1      | 0.002162  | miss (MongoDB) |
| 2      | 0.001225  | hit (memory) |
| 3      | 0.001186  | hit (memory) |

```
GET /api/v1/folders/cache-stats
→ {"cache":"folders_by_owner","hits":2,"misses":1,"hit_rate":0.6667}
```

При продолжении работы (больше запросов без создания/удаления папок) hit rate
стремится к 1.0.

В локальной среде разница во времени мала. Основной эффект — уменьшение числа запросов к базе, это позволяет ей обрабатывать больше операций записи и сложных аналитических запросов.
