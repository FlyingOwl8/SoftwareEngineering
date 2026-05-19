#!/usr/bin/env bash
# Тесты REST API cloud-storage (lab6 — CQRS + Kafka)
# Запуск: ./tests/test_api.sh
# Требует: curl, jq
# Сервис должен быть запущен: podman compose up (порт 8080)

set -euo pipefail

BASE_URL="${BASE_URL:-http://localhost:8080}"
PASS=0
FAIL=0

# Уникальный суффикс — тесты не конфликтуют с данными от предыдущих прогонов
TS=$(date +%s)
ALICE="alice_${TS}"
BOB="bob_${TS}"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass()    { echo -e "  ${GREEN}✓${NC} $1"; PASS=$((PASS + 1)); }
fail()    { echo -e "  ${RED}✗${NC} $1"; FAIL=$((FAIL + 1)); }
section() { echo -e "\n${YELLOW}▶ $1${NC}"; }

http_status() {
    curl -s -o /dev/null -w "%{http_code}" "$@"
}

http_full() {
    curl -s -w "\n%{http_code}" "$@"
}

assert_status() {
    local label="$1" expected="$2" actual="$3"
    if [[ "$actual" == "$expected" ]]; then
        pass "$label (HTTP $actual)"
    else
        fail "$label (ожидался HTTP $expected, получен $actual)"
    fi
}

assert_field() {
    local label="$1" field="$2" value="$3"
    if [[ -n "$value" && "$value" != "null" ]]; then
        pass "$label (.$field = \"$value\")"
    else
        fail "$label (поле .$field отсутствует или пустое)"
    fi
}

# ─── 1. Регистрация ────────────────────────────────────────────────────────────

section "1. Регистрация пользователей"

BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"first_name\":\"Alice\",\"last_name\":\"Smith\",\"password\":\"secret123\"}")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /auth/register — новый пользователь" 201 "$STATUS"
assert_field  "  ответ содержит id" "id" "$(echo "$JSON" | jq -r '.id // empty')"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"first_name\":\"A\",\"last_name\":\"B\",\"password\":\"secret123\"}")
assert_status "POST /auth/register — дубль логина → 409" 409 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d '{"login":"ab","first_name":"A","last_name":"B","password":"secret123"}')
assert_status "POST /auth/register — логин < 3 символов → 400" 400 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d '{"login":"tmpuser","first_name":"B","last_name":"B","password":"123"}')
assert_status "POST /auth/register — пароль < 6 символов → 400" 400 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d '{"login":"charlie","password":"secret123"}')
assert_status "POST /auth/register — нет first_name/last_name → 400" 400 "$STATUS"

curl -s -o /dev/null \
    -X POST "$BASE_URL/api/v1/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$BOB\",\"first_name\":\"Bob\",\"last_name\":\"Jones\",\"password\":\"bobpass1\"}"

# ─── 2. Вход ──────────────────────────────────────────────────────────────────

section "2. Вход (логин)"

BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"password\":\"secret123\"}")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /auth/login — верные данные" 200 "$STATUS"
USER_TOKEN=$(echo "$JSON" | jq -r '.token // empty')
assert_field  "  ответ содержит token" "token" "$USER_TOKEN"

BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"admin","password":"admin123"}')
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "POST /auth/login — admin" 200 "$STATUS"
ADMIN_TOKEN=$(echo "$JSON" | jq -r '.token // empty')
assert_field  "  ответ содержит token" "token" "$ADMIN_TOKEN"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$ALICE\",\"password\":\"wrongpass\"}")
assert_status "POST /auth/login — неверный пароль → 401" 401 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"nobody","password":"secret123"}')
assert_status "POST /auth/login — несуществующий логин → 401" 401 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d 'not-json')
assert_status "POST /auth/login — невалидный JSON → 400" 400 "$STATUS"

# ─── 3. Пользователи (только admin) ───────────────────────────────────────────

section "3. Пользователи (только admin)"

BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/users/$ALICE" \
    -H "Authorization: Bearer $ADMIN_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /users/{login} — admin, пользователь существует" 200 "$STATUS"
assert_field  "  ответ содержит login" "login" "$(echo "$JSON" | jq -r '.login // empty')"

STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users/$ALICE" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /users/{login} — не-admin → 403" 403 "$STATUS"

STATUS=$(http_status -X GET "$BASE_URL/api/v1/users/$ALICE")
assert_status "GET /users/{login} — без авторизации → 401" 401 "$STATUS"

STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users/nobody_${TS}" \
    -H "Authorization: Bearer $ADMIN_TOKEN")
assert_status "GET /users/{login} — пользователь не найден → 404" 404 "$STATUS"

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

STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users?first_name=Alice" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /users?first_name= — не-admin → 403" 403 "$STATUS"

STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/users" \
    -H "Authorization: Bearer $ADMIN_TOKEN")
assert_status "GET /users — без параметров поиска → 400" 400 "$STATUS"

# ─── 4. Папки (CQRS + write-through cache) ────────────────────────────────────

section "4. Папки (CQRS + write-through cache)"

# 4.1 Создать папку — должна быть видна немедленно (write-through)
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

# 4.2 Папка видна в списке немедленно (write-through cache, без polling)
BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /folders — сразу после POST" 200 "$STATUS"
FOUND=$(echo "$JSON" | jq -r '[.[] | select(.name=="documents")] | length')
if [[ "$FOUND" -ge 1 ]]; then
    pass "  папка 'documents' видна немедленно (write-through cache)"
else
    fail "  папка 'documents' не найдена сразу после создания"
fi

# 4.3 Дубль имени → 409 (проверка через cache: FolderNameExists читает MongoDB)
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"documents"}')
assert_status "POST /folders — дубль имени → 409" 409 "$STATUS"

# 4.4 Создать без имени → 400
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{}')
assert_status "POST /folders — нет поля name → 400" 400 "$STATUS"

# 4.5 Создать без токена → 401
STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders" \
    -H "Content-Type: application/json" \
    -d '{"name":"new"}')
assert_status "POST /folders — без авторизации → 401" 401 "$STATUS"

# 4.6 Список папок — без токена → 401
STATUS=$(http_status -X GET "$BASE_URL/api/v1/folders")
assert_status "GET /folders — без авторизации → 401" 401 "$STATUS"

# 4.7 Создать несколько папок, все видны сразу
for i in 1 2 3; do
    NAME="batch-${i}-${TS}"
    STATUS=$(http_status \
        -X POST "$BASE_URL/api/v1/folders" \
        -H "Authorization: Bearer $USER_TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"name\":\"${NAME}\"}")
    assert_status "POST /folders '${NAME}' → 201" 201 "$STATUS"
done

BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN")
COUNT=$(echo "$BODY" | head -1 | jq 'length // 0')
if [[ "$COUNT" -ge 4 ]]; then
    pass "GET /folders — найдено ${COUNT} папок (все batch видны немедленно)"
else
    fail "GET /folders — найдено только ${COUNT} папок, ожидалось ≥ 4"
fi

# 4.8 Удалить чужую папку → 403
BOB_BODY=$(http_full \
    -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$BOB\",\"password\":\"bobpass1\"}")
BOB_TOKEN=$(echo "$BOB_BODY" | head -1 | jq -r '.token // empty')
STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID" \
    -H "Authorization: Bearer $BOB_TOKEN")
assert_status "DELETE /folders/{id} — чужая папка → 403" 403 "$STATUS"

# 4.9 Удалить несуществующую папку → 404
STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/00000000-0000-0000-0000-000000000000" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "DELETE /folders/{id} — не найдена → 404" 404 "$STATUS"

# ─── 5. Файлы ─────────────────────────────────────────────────────────────────

section "5. Файлы"

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

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"readme.txt"}')
assert_status "POST /folders/{id}/files — дубль имени → 409" 409 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{}')
assert_status "POST /folders/{id}/files — нет поля name → 400" 400 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/00000000-0000-0000-0000-000000000000/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"x.txt"}')
assert_status "POST /folders/{id}/files — папка не найдена → 404" 404 "$STATUS"

STATUS=$(http_status \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $BOB_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"hack.txt"}')
assert_status "POST /folders/{id}/files — чужая папка → 403" 403 "$STATUS"

BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/readme.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /folders/{id}/files/{name} — существующий" 200 "$STATUS"
assert_field  "  ответ содержит name" "name" "$(echo "$JSON" | jq -r '.name // empty')"

STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/ghost.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /folders/{id}/files/{name} — не найден → 404" 404 "$STATUS"

STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID/files/$FILE_ID/delete" \
    -H "Authorization: Bearer $BOB_TOKEN")
assert_status "DELETE /folders/{id}/files/{fid}/delete — чужой файл → 403" 403 "$STATUS"

STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID/files/$FILE_ID/delete" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "DELETE /folders/{id}/files/{fid}/delete — успешно → 204" 204 "$STATUS"

STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/readme.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET /folders/{id}/files/{name} — после удаления → 404" 404 "$STATUS"

# ─── 6. Удаление папки (каскадное) ───────────────────────────────────────────

section "6. Удаление папки (каскадное)"

curl -s -o /dev/null \
    -X POST "$BASE_URL/api/v1/folders/$FOLDER_ID/files" \
    -H "Authorization: Bearer $USER_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"temp.txt"}'

STATUS=$(http_status \
    -X DELETE "$BASE_URL/api/v1/folders/$FOLDER_ID" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "DELETE /folders/{id} — успешно → 204" 204 "$STATUS"

STATUS=$(http_status \
    -X GET "$BASE_URL/api/v1/folders/$FOLDER_ID/files/temp.txt" \
    -H "Authorization: Bearer $USER_TOKEN")
assert_status "GET файла из удалённой папки → 404" 404 "$STATUS"

BODY=$(http_full \
    -X GET "$BASE_URL/api/v1/folders" \
    -H "Authorization: Bearer $USER_TOKEN")
STATUS=$(echo "$BODY" | tail -1)
JSON=$(echo "$BODY" | head -1)
assert_status "GET /folders — после удаления" 200 "$STATUS"
# batch-папки остаются, основная удалена
COUNT=$(echo "$JSON" | jq '[.[] | select(.name=="documents")] | length')
if [[ "$COUNT" -eq 0 ]]; then
    pass "  папка 'documents' исчезла из списка"
else
    fail "  папка 'documents' всё ещё в списке после удаления"
fi

# ─── Итог ─────────────────────────────────────────────────────────────────────

echo ""
echo "─────────────────────────────────────────"
TOTAL=$((PASS + FAIL))
echo -e "Результат: ${GREEN}$PASS${NC}/$TOTAL прошло, ${RED}$FAIL${NC}/$TOTAL упало"
if [[ $FAIL -eq 0 ]]; then
    echo -e "${GREEN}Все тесты пройдены!${NC}"
    exit 0
else
    echo -e "${RED}Есть упавшие тесты.${NC}"
    exit 1
fi
