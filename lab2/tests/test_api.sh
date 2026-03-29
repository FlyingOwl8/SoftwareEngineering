#!/usr/bin/env bash
# Тесты REST API cloud-storage
# Запуск: ./tests/test_api.sh
# Требует: curl, jq
# Сервис должен быть запущен: podman-compose up (порт 8080)

BASE_URL="${BASE_URL:-http://localhost:8080}"
PASS=0
FAIL=0

# Уникальный суффикс — тесты не конфликтуют с данными от предыдущих прогонов
TS=$(date +%s)
ALICE="alice_${TS}"
BOB="bob_${TS}"

# ─── Вспомогательные функции ──────────────────────────────────────────────────

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass() { echo -e "  ${GREEN}✓${NC} $1"; PASS=$((PASS + 1)); }
fail() { echo -e "  ${RED}✗${NC} $1"; FAIL=$((FAIL + 1)); }
section() { echo -e "\n${YELLOW}▶ $1${NC}"; }

# Выполнить запрос, вернуть HTTP-статус
http_status() {
    curl -s -o /dev/null -w "%{http_code}" "$@"
}

# Выполнить запрос, вернуть тело + статус через \n
http_full() {
    curl -s -w "\n%{http_code}" "$@"
}

# Проверить: ожидаемый статус == реальный
assert_status() {
    local label="$1" expected="$2" actual="$3"
    if [[ "$actual" == "$expected" ]]; then
        pass "$label (HTTP $actual)"
    else
        fail "$label (ожидался HTTP $expected, получен $actual)"
    fi
}

# Проверить: JSON-поле существует и не пустое
assert_field() {
    local label="$1" field="$2" value="$3"
    if [[ -n "$value" && "$value" != "null" ]]; then
        pass "$label (.$field = \"$value\")"
    else
        fail "$label (поле .$field отсутствует или пустое)"
    fi
}

# ─── Сценарии ─────────────────────────────────────────────────────────────────

section "1. Регистрация пользователей"

# 1.1 Успешная регистрация
BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"first_name\":\"Alice\",\"last_name\":\"Smith\",\"password\":\"secret123\"}")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /auth/register — новый пользователь" 201 "$STATUS"
assert_field  "  ответ содержит id" "id" "$(echo "$JSON" | jq -r '.id // empty')"

# 1.2 Дублирующийся логин
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"first_name\":\"A\",\"last_name\":\"B\",\"password\":\"secret123\"}")
assert_status "POST /auth/register — дубль логина → 409" 409 "$STATUS"

# 1.3 Короткий логин
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d '{"login":"ab","first_name":"A","last_name":"B","password":"secret123"}')
assert_status "POST /auth/register — логин < 3 символов → 400" 400 "$STATUS"

# 1.4 Короткий пароль
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d '{"login":"tmpuser","first_name":"B","last_name":"B","password":"123"}')
assert_status "POST /auth/register — пароль < 6 символов → 400" 400 "$STATUS"

# 1.5 Отсутствующее поле
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d '{"login":"charlie","password":"secret123"}')
assert_status "POST /auth/register — нет first_name/last_name → 400" 400 "$STATUS"

# Зарегистрируем второго пользователя для тестов прав доступа
curl -s -o /dev/null \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$BOB\",\"first_name\":\"Bob\",\"last_name\":\"Jones\",\"password\":\"bobpass1\"}"

# ─────────────────────────────────────────────────────────────────────────────

section "2. Вход (логин)"

# 2.1 Успешный вход обычного пользователя
BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"password\":\"secret123\"}")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /auth/login — верные данные" 200 "$STATUS"
USER_TOKEN=$(echo "$JSON" | jq -r '.token // empty')
assert_field  "  ответ содержит token" "token" "$USER_TOKEN"

# 2.2 Успешный вход администратора
BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"admin","password":"admin123"}')
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /auth/login — admin" 200 "$STATUS"
ADMIN_TOKEN=$(echo "$JSON" | jq -r '.token // empty')
assert_field  "  ответ содержит token" "token" "$ADMIN_TOKEN"

# 2.3 Неверный пароль
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"password\":\"wrongpass\"}")
assert_status "POST /auth/login — неверный пароль → 401" 401 "$STATUS"

# 2.4 Несуществующий пользователь
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"nobody","password":"secret123"}')
assert_status "POST /auth/login — несуществующий логин → 401" 401 "$STATUS"

# 2.5 Невалидный JSON
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d 'not-json')
assert_status "POST /auth/login — невалидный JSON → 400" 400 "$STATUS"

# ─────────────────────────────────────────────────────────────────────────────

section "3. Пользователи (только admin)"

# 3.1 GET /users/{login} — admin успешно
BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/users/$ALICE" \
    -H "Authorization: Bearer $ADMIN_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /users/{login} — admin, пользователь существует" 200 "$STATUS"
assert_field  "  ответ содержит login" "login" "$(echo "$JSON" | jq -r '.login // empty')"

# 3.2 GET /users/{login} — обычный пользователь → 403
STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users/$ALICE" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /users/{login} — не-admin → 403" 403 "$STATUS"

# 3.3 GET /users/{login} — без токена → 401
STATUS=$(http_status -X GET "$BASE_URL/api/v1/users/$ALICE")
assert_status "GET /users/{login} — без авторизации → 401" 401 "$STATUS"

# 3.4 GET /users/{login} — несуществующий → 404
STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users/nobody_${TS}" \
    -H "Authorization: Bearer $ADMIN_TOKEN")
assert_status "GET /users/{login} — пользователь не найден → 404" 404 "$STATUS"

# 3.5 GET /users?first_name= — поиск администратором
BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/users?first_name=Alice" \
    -H "Authorization: Bearer $ADMIN_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /users?first_name=Alice — admin" 200 "$STATUS"
COUNT=$(echo "$JSON" | jq 'length // 0')
if [[ "$COUNT" -ge 1 ]]; then
    pass "  найдено $COUNT пользователь(ей)"
else
    fail "  ожидался хотя бы 1 результат, получено $COUNT"
fi

# 3.6 GET /users?first_name= — обычный пользователь → 403
STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users?first_name=Alice" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /users?first_name= — не-admin → 403" 403 "$STATUS"

# 3.7 GET /users без параметров → 400
STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users" \
    -H "Authorization: Bearer $ADMIN_TOKEN")
assert_status "GET /users — без параметров поиска → 400" 400 "$STATUS"

# ─────────────────────────────────────────────────────────────────────────────

section "4. Папки"

# 4.1 Создать папку
BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"documents"}')
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /folders — создание" 201 "$STATUS"
FOLDER_ID=$(echo "$JSON" | jq -r '.id // empty')
assert_field  "  ответ содержит id папки" "id" "$FOLDER_ID"

# 4.2 Создать папку с тем же именем → 409
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"documents"}')
assert_status "POST /folders — дубль имени → 409" 409 "$STATUS"

# 4.3 Создать без имени → 400
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{}')
assert_status "POST /folders — нет поля name → 400" 400 "$STATUS"

# 4.4 Создать без токена → 401
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders" \
    -H "Content-Type: application/json" \
    -d '{"name":"new"}')
assert_status "POST /folders — без авторизации → 401" 401 "$STATUS"

# 4.5 Список папок
BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /folders — список" 200 "$STATUS"
COUNT=$(echo "$JSON" | jq 'length // 0')
if [[ "$COUNT" -ge 1 ]]; then
    pass "  в списке $COUNT папка(ок)"
else
    fail "  ожидалась хотя бы 1 папка, получено $COUNT"
fi

# 4.6 Список папок — без токена → 401
STATUS=$(http_status -X GET "$BASE_URL/api/v1/folders")
assert_status "GET /folders — без авторизации → 401" 401 "$STATUS"

# 4.7 Удалить чужую папку — bob пытается удалить папку alice → 403
BOB_BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$BOB\",\"password\":\"bobpass1\"}")
BOB_TOKEN=$(echo "$BOB_BODY" | head -1 | jq -r '.token // empty')
STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID" \
    -H "Authorization: Bearer $BOB_TOKEN")
assert_status "DELETE /folders/{id} — чужая папка → 403" 403 "$STATUS"

# 4.8 Удалить несуществующую папку → 404
STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/00000000-0000-0000-0000-000000000000" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "DELETE /folders/{id} — не найдена → 404" 404 "$STATUS"

# ─────────────────────────────────────────────────────────────────────────────

section "5. Файлы"

# 5.1 Создать файл
BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"readme.txt","content_type":"text/plain","size":11,"content":"aGVsbG8gd29ybGQ="}')
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /folders/{id}/files — создание" 201 "$STATUS"
FILE_ID=$(echo "$JSON" | jq -r '.id // empty')
assert_field  "  ответ содержит id файла" "id" "$FILE_ID"

# 5.2 Создать файл с тем же именем → 409
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"readme.txt"}')
assert_status "POST /folders/{id}/files — дубль имени → 409" 409 "$STATUS"

# 5.3 Создать файл без имени → 400
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{}')
assert_status "POST /folders/{id}/files — нет поля name → 400" 400 "$STATUS"

# 5.4 Создать файл в несуществующей папке → 404
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/00000000-0000-0000-0000-000000000000/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"x.txt"}')
assert_status "POST /folders/{id}/files — папка не найдена → 404" 404 "$STATUS"

# 5.5 Создать файл в чужой папке → 403
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $BOB_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"hack.txt"}')
assert_status "POST /folders/{id}/files — чужая папка → 403" 403 "$STATUS"

# 5.6 Получить файл по имени
BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/readme.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /folders/{id}/files/{name} — существующий" 200 "$STATUS"
assert_field  "  ответ содержит name" "name" "$(echo "$JSON" | jq -r '.name // empty')"

# 5.7 Получить несуществующий файл → 404
STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/ghost.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /folders/{id}/files/{name} — не найден → 404" 404 "$STATUS"

# 5.8 Удалить файл из чужой папки → 403
STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID/files/$FILE_ID/delete" \
    -H "Authorization: Bearer $BOB_TOKEN")
assert_status "DELETE /folders/{id}/files/{fid}/delete — чужой файл → 403" 403 "$STATUS"

# 5.9 Удалить файл
STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID/files/$FILE_ID/delete" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "DELETE /folders/{id}/files/{fid}/delete — успешно → 204" 204 "$STATUS"

# 5.10 Получить удалённый файл → 404
STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/readme.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /folders/{id}/files/{name} — после удаления → 404" 404 "$STATUS"

# ─────────────────────────────────────────────────────────────────────────────

section "6. Удаление папки (каскадное)"

# Создать файл в папке, затем удалить папку
curl -s -o /dev/null \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"temp.txt"}'

STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "DELETE /folders/{id} — успешно → 204" 204 "$STATUS"

# Файл в удалённой папке → 404
STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/temp.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET файла из удалённой папки → 404" 404 "$STATUS"

# Список папок стал пустым
BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /folders — после удаления" 200 "$STATUS"
COUNT=$(echo "$JSON" | jq 'length // 0')
if [[ "$COUNT" -eq 0 ]]; then
    pass "  список папок пуст"
else
    fail "  ожидался пустой список, получено $COUNT"
fi

# ─────────────────────────────────────────────────────────────────────────────

echo ""
echo "─────────────────────────────────────────"
TOTAL=$((PASS + FAIL))
echo -e "Результат: ${GREEN}$PASS${NC}/$TOTAL прошло, ${RED}$FAIL${NC}/$TOTAL упало"
if [[ $FAIL -eq 0 ]]; then
    echo -e "${GREEN}Все тесты пройдены!${NC}"
    exit 0
else
    exit 1
fi
