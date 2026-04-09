# Оптимизация запросов — таблица users (вариант 11)

Анализ выполнен на PostgreSQL 16.13 с тестовыми данными из `data.sql` (15 пользователей).

---

## 1. Поиск пользователя по логину (POST /auth/login, GET /users/{login})

```sql
SELECT id, login, first_name, last_name, password_hash, role, created_at
FROM users WHERE login = 'alice';
```

### План выполнения

```
Index Scan using users_login_unique on users
  (cost=0.14..8.16 rows=1 width=1870)
  (actual time=0.015..0.020 rows=1 loops=1)
  Index Cond: ((login)::text = 'alice'::text)
Planning Time: 0.948 ms
Execution Time: 0.074 ms
```

**Вывод:** `UNIQUE` constraint на `login` автоматически создаёт B-tree индекс.
Запрос всегда использует Index Scan, O(log n). Дополнительной оптимизации не требует.

---

## 2. Поиск пользователей по маске имени (GET /users?first_name=&last_name=)

```sql
SELECT id, login, first_name, last_name, role, created_at
FROM users
WHERE first_name ILIKE '%ал%' AND last_name ILIKE '%ов%';
```

### До оптимизации (без триграммных индексов)

```
Seq Scan on users
  (cost=0.00..10.60 rows=1 width=1610)
  (actual time=0.010..0.018 rows=1 loops=1)
  Filter: (first_name ~~* '%ал%' AND last_name ~~* '%ов%')
  Rows Removed by Filter: 14
Planning Time: 0.286 ms
Execution Time: 0.036 ms
```

**Проблема:** `ILIKE '%маска%'` с ведущим `%` не может использовать обычный B-tree индекс.
При росте числа пользователей это будет Seq Scan по всей таблице — O(n).

### После оптимизации (GIN-индексы на триграммах, `enable_seqscan=off` для демонстрации)

```sql
CREATE EXTENSION pg_trgm;
CREATE INDEX users_first_name_trgm_idx ON users USING GIN (first_name gin_trgm_ops);
CREATE INDEX users_last_name_trgm_idx  ON users USING GIN (last_name  gin_trgm_ops);
```

```
Bitmap Heap Scan on users
  (cost=46.79..50.80 rows=1 width=68)
  (actual time=0.062..0.064 rows=1 loops=1)
  Recheck Cond: (last_name ~~* '%Сем%')
  Filter: (first_name ~~* '%Ири%')
  Heap Blocks: exact=1
  -> Bitmap Index Scan on users_last_name_trgm_idx
       (cost=0.00..46.79 rows=1 width=0)
       (actual time=0.053..0.053 rows=1 loops=1)
       Index Cond: (last_name ~~* '%Сем%')
Planning Time: 0.690 ms
Execution Time: 0.191 ms
```

**Вывод:** GIN-индекс по триграммам позволяет PostgreSQL использовать Bitmap Index Scan
вместо Seq Scan при селективных масках. При небольших таблицах планировщик всё равно
выбирает Seq Scan (дешевле), но при тысячах записей индекс даёт значимый прирост.

---

## Итоговая таблица индексов

| Индекс | Колонки | Тип | Покрывает |
|--------|---------|-----|-----------|
| `users_login_unique` | `users(login)` | B-tree (auto) | логин при входе, проверка дубликата при регистрации |
| `users_first_name_trgm_idx` | `users(first_name)` | GIN trgm | ILIKE-поиск по имени |
| `users_last_name_trgm_idx` | `users(last_name)` | GIN trgm | ILIKE-поиск по фамилии |
