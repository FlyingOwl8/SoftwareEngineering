CREATE EXTENSION IF NOT EXISTS "pgcrypto";
CREATE EXTENSION IF NOT EXISTS pg_trgm;

-- Таблица пользователей
CREATE TABLE users (
    id            UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    login         VARCHAR(255) NOT NULL,
    first_name    VARCHAR(255) NOT NULL,
    last_name     VARCHAR(255) NOT NULL,
    password_hash CHAR(64)     NOT NULL,
    role          VARCHAR(10)  NOT NULL DEFAULT 'user'
                               CHECK (role IN ('user', 'admin')),
    created_at    TIMESTAMPTZ  NOT NULL DEFAULT NOW(),

    CONSTRAINT users_login_unique UNIQUE (login)
);

COMMENT ON TABLE  users               IS 'Зарегистрированные пользователи системы';
COMMENT ON COLUMN users.password_hash IS 'SHA-256 от пароля в hex-представлении';
COMMENT ON COLUMN users.role          IS 'Роль: user — обычный, admin — администратор';


-- Индексы

-- UNIQUE constraint на login автоматически создаёт B-tree индекс —
-- используется при каждом входе в систему (WHERE login = $1).

-- GIN-индексы на триграммах для поиска по маске имени/фамилии (ILIKE '%...%').
-- Без них при большом числе пользователей будет Seq Scan по всей таблице.
CREATE INDEX users_first_name_trgm_idx ON users USING GIN (first_name gin_trgm_ops);
CREATE INDEX users_last_name_trgm_idx  ON users USING GIN (last_name  gin_trgm_ops);

COMMENT ON INDEX users_first_name_trgm_idx IS
    'Ускоряет поиск по маске first_name ILIKE ''%...%'' через триграммы';
COMMENT ON INDEX users_last_name_trgm_idx IS
    'Ускоряет поиск по маске last_name ILIKE ''%...%'' через триграммы';
