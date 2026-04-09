-- POST /api/v1/auth/register — регистрация пользователя
-- ──────────────────────────────────────────────────────────────
-- $1 = login, $2 = first_name, $3 = last_name, $4 = password_hash (SHA-256 hex)

INSERT INTO users (login, first_name, last_name, password_hash, role)
VALUES ($1, $2, $3, $4, 'user')
RETURNING id, login, first_name, last_name, role, created_at;


-- POST /api/v1/auth/login — вход (получение JWT)
-- ──────────────────────────────────────────────────────────────
-- $1 = login
-- Сервис сравнивает password_hash самостоятельно

SELECT id, login, first_name, last_name, password_hash, role, created_at
FROM users
WHERE login = $1;


-- GET /api/v1/users/{login} — получить пользователя по логину
-- ──────────────────────────────────────────────────────────────
-- $1 = login

SELECT id, login, first_name, last_name, role, created_at
FROM users
WHERE login = $1;


-- GET /api/v1/users?first_name=&last_name= — поиск (только admin)
-- ──────────────────────────────────────────────────────────────
-- $1 = маска first_name (например '%Ал%'), $2 = маска last_name (например '%ов%')
-- Если маска пустая, передаётся '%'

SELECT id, login, first_name, last_name, role, created_at
FROM users
WHERE first_name ILIKE $1
  AND last_name  ILIKE $2
ORDER BY created_at;


-- Вспомогательные запросы (внутренние проверки сервиса)
-- ============================================================

-- Проверка уникальности логина перед регистрацией
SELECT EXISTS (SELECT 1 FROM users WHERE login = $1) AS login_taken;
