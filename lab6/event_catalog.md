# Каталог событий: Cloud Storage

## Команды и порождаемые события

| Команда | Событие | Инициатор | Статус |
|---------|---------|-----------|--------|
| `POST /api/v1/folders` | `FolderCreated` | Авторизованный пользователь | **реализовано** |
| `DELETE /api/v1/folders/{id}` | `FolderDeleted` | Владелец папки | не реализовано |
| `POST /api/v1/folders/{id}/files` | `FileUploaded` | Владелец папки | не реализовано |
| `DELETE /api/v1/folders/{id}/files/{fid}/delete` | `FileDeleted` | Владелец файла | не реализовано |

---

## FolderCreated

| Поле | Значение |
|------|----------|
| **Производитель** | `FolderFileService` |
| **Потребитель** | `folder-consumer` → MongoDB |
| **Гарантия доставки** | at-least-once |
| **Формат** | JSON, topic `folder-events` |

### Payload

```json
{
  "id":              "3f2090d9-57a1-4d14-ae14-008137cf603e",
  "name":            "Documents",
  "owner_id":        "a1b2c3d4-0000-4000-a000-000000000001",
  "created_at_unix": 1746057600
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| `id` | string (UUID v4) | Идентификатор папки, генерируется сервисом до публикации |
| `name` | string | Имя папки |
| `owner_id` | string (UUID v4) | Идентификатор владельца (из JWT) |
| `created_at_unix` | int64 | Время создания, секунды Unix epoch |

---

## FolderDeleted *(не реализовано)*

| Поле | Значение |
|------|----------|
| **Производитель** | `FolderFileService` |
| **Потребитель** | `folder-consumer` → MongoDB |
| **Гарантия доставки** | at-least-once |
| **Формат** | JSON, topic `folder-events` |

Сейчас `DeleteFolder` пишет в MongoDB напрямую. При переводе на CQRS consumer должен удалять документ по `folder_id`; идемпотентность обеспечивается тем, что `DeleteOne` по несуществующему `_id` не является ошибкой.

### Payload

```json
{
  "folder_id":       "35edf86e-83e6-4b4e-8038-57e588e41274",
  "owner_id":        "a1b2c3d4-0000-4000-a000-000000000001",
  "deleted_at_unix": 1746057600
}
```

---

## FileUploaded *(не реализовано)*

| Поле | Значение |
|------|----------|
| **Производитель** | `FolderFileService` |
| **Потребитель** | `folder-consumer` → MongoDB |
| **Гарантия доставки** | at-least-once |
| **Формат** | JSON, topic `folder-events` |

Аналог `FolderCreated` для файлов. Идемпотентность — тот же механизм: `file_id` генерируется на стороне сервиса, записывается как `_id`, повторная вставка даёт `DuplicateKey`.

### Payload

```json
{
  "file_id":         "ffea33f5-46b4-4944-9804-c3552a0deb15",
  "folder_id":       "35edf86e-83e6-4b4e-8038-57e588e41274",
  "owner_id":        "a1b2c3d4-0000-4000-a000-000000000001",
  "name":            "report.pdf",
  "size":            204800,
  "created_at_unix": 1746057600
}
```

---

## FileDeleted *(не реализовано)*

| Поле | Значение |
|------|----------|
| **Производитель** | `FolderFileService` |
| **Потребитель** | `folder-consumer` → MongoDB |
| **Гарантия доставки** | at-least-once |
| **Формат** | JSON, topic `folder-events` |

Аналог `FolderDeleted` для файлов. Идемпотентность — `DeleteOne` по несуществующему `_id` не является ошибкой.

### Payload

```json
{
  "file_id":         "ffea33f5-46b4-4944-9804-c3552a0deb15",
  "folder_id":       "35edf86e-83e6-4b4e-8038-57e588e41274",
  "owner_id":        "a1b2c3d4-0000-4000-a000-000000000001",
  "deleted_at_unix": 1746057600
}
```
