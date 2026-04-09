// ── Вспомогательные переменные ────────────────────────────────────────────────

const alice_id    = "00000000-0000-4000-a000-000000000002";
const bob_id      = "00000000-0000-4000-a000-000000000003";
const folder1_id  = "10000000-0000-4000-a000-000000000001";  // alice/Documents
const folder4_id  = "10000000-0000-4000-a000-000000000004";  // bob/Work
const file1_id    = "20000000-0000-4000-a000-000000000001";  // readme.txt


// CREATE операции
// ============================================================

// 1. POST /api/v1/folders — создать папку
// Проверка уникальности имени у владельца
print("\n--- 1. Создание папки ---");
var folderExists = db.folders.findOne({ owner_id: alice_id, name: "NewFolder" });
print("Папка уже существует:", folderExists !== null);

db.folders.insertOne({
  _id: "aaaaaaaa-0000-4000-a000-000000000001",
  name: "NewFolder",
  owner_id: alice_id,
  created_at: new Date()
});
print("Папка создана:", db.folders.findOne({ _id: "aaaaaaaa-0000-4000-a000-000000000001" }).name);

// 2. POST /api/v1/folders/{id}/files — загрузить файл в папку
// Проверка уникальности имени файла в папке
print("\n--- 2. Создание файла ---");
var fileExists = db.files.findOne({ folder_id: folder1_id, name: "new_file.txt" });
print("Файл уже существует:", fileExists !== null);

db.files.insertOne({
  _id: "bbbbbbbb-0000-4000-a000-000000000001",
  name: "new_file.txt",
  folder_id: folder1_id,
  owner_id: alice_id,
  content_type: "text/plain",
  size: 100,
  content: "aGVsbG8=",
  created_at: new Date()
});
print("Файл создан:", db.files.findOne({ _id: "bbbbbbbb-0000-4000-a000-000000000001" }).name);

// READ операции
// ============================================================

// 3. GET /api/v1/folders — список папок пользователя
print("\n--- 3. Список папок пользователя ---");
var aliceFolders = db.folders.find(
  { owner_id: alice_id },
  { name: 1, created_at: 1 }
).sort({ created_at: 1 }).toArray();
printjson(aliceFolders.map(f => f.name));

// 4. GET /api/v1/folders/{id}/files/{name} — получить файл по имени
print("\n--- 4. Получить файл по имени ---");
var file = db.files.findOne({ folder_id: folder1_id, name: "readme.txt" });
printjson({ id: file._id, name: file.name, size: file.size, content_type: file.content_type });

// 5. Получить папку по id (для проверки владельца)
print("\n--- 5. Получить папку по id ---");
var folder = db.folders.findOne({ _id: folder1_id }, { name: 1, owner_id: 1 });
printjson(folder);

// 6. Список файлов в папке
print("\n--- 6. Список файлов в папке ---");
var folderFiles = db.files.find(
  { folder_id: folder1_id },
  { name: 1, content_type: 1, size: 1, created_at: 1 }
).sort({ created_at: 1 }).toArray();
printjson(folderFiles.map(f => ({ name: f.name, size: f.size })));

// 7. Поиск папок по нескольким пользователям ($in)
print("\n--- 7. Папки нескольких пользователей ($in) ---");
var multiFolders = db.folders.find(
  { owner_id: { $in: [alice_id, bob_id] } }
).sort({ owner_id: 1, created_at: 1 }).toArray();
print("Найдено папок:", multiFolders.length);

// 8. Файлы определённого типа у владельца ($and + $eq)
print("\n--- 8. Файлы определённого типа ($and + $eq) ---");
var imageFiles = db.files.find({
  $and: [
    { owner_id: alice_id },
    { content_type: { $in: ["image/jpeg", "image/png"] } }
  ]
}).toArray();
print("Изображений у alice:", imageFiles.length);

// 9. Большие файлы ($gt)
print("\n--- 9. Большие файлы > 1MB ($gt) ---");
var bigFiles = db.files.find(
  { size: { $gt: 1048576 } },
  { name: 1, size: 1, content_type: 1 }
).sort({ size: -1 }).toArray();
printjson(bigFiles.map(f => ({ name: f.name, size: f.size })));

// UPDATE операции
// ============================================================

// 10. Переименование папки (нет в API, но демонстрирует updateOne)
print("\n--- 10. Переименование папки ---");
db.folders.updateOne(
  { _id: "aaaaaaaa-0000-4000-a000-000000000001" },
  { $set: { name: "RenamedFolder" } }
);
print("Новое имя:", db.folders.findOne({ _id: "aaaaaaaa-0000-4000-a000-000000000001" }).name);

// 11. Обновление размера файла ($set)
print("\n--- 11. Обновление размера файла ---");
db.files.updateOne(
  { _id: "bbbbbbbb-0000-4000-a000-000000000001" },
  { $set: { size: 200, content: "dXBkYXRlZA==" } }
);
print("Новый размер:", db.files.findOne({ _id: "bbbbbbbb-0000-4000-a000-000000000001" }).size);

// DELETE операции
// ============================================================

// 12. DELETE /api/v1/folders/{id}/files/{fid}/delete — удалить файл
print("\n--- 12. Удаление файла ---");
var deleteResult = db.files.deleteOne({ _id: "bbbbbbbb-0000-4000-a000-000000000001" });
print("Удалено файлов:", deleteResult.deletedCount);

// 13. DELETE /api/v1/folders/{id} — удалить папку (каскадно)
// Сначала удаляем файлы, потом папку
print("\n--- 13. Каскадное удаление папки ---");
var folder_to_delete = "aaaaaaaa-0000-4000-a000-000000000001";
var filesDeleted = db.files.deleteMany({ folder_id: folder_to_delete });
print("Файлов удалено:", filesDeleted.deletedCount);
var folderDeleted = db.folders.deleteOne({ _id: folder_to_delete });
print("Папок удалено:", folderDeleted.deletedCount);

// Дополнительные запросы с операторами
// ============================================================

// Файлы НЕ являющиеся текстовыми ($ne)
print("\n--- Не текстовые файлы ($ne) ---");
var nonText = db.files.find(
  { content_type: { $ne: "text/plain" } },
  { name: 1, content_type: 1 }
).toArray();
print("Нетекстовых файлов:", nonText.length);

// Файлы размером между 100KB и 1MB ($gt + $lt)
print("\n--- Файлы 100KB–1MB ---");
var mediumFiles = db.files.find(
  { size: { $gt: 102400, $lt: 1048576 } },
  { name: 1, size: 1 }
).toArray();
printjson(mediumFiles.map(f => ({ name: f.name, size: f.size })));

// Папки с именем начинающимся на "D" ($regex)
print("\n--- Папки на букву D ($regex) ---");
var dFolders = db.folders.find(
  { name: { $regex: /^D/i } },
  { name: 1, owner_id: 1 }
).toArray();
printjson(dFolders.map(f => f.name));

// Файлы загруженные после определённой даты ($gt на Date)
print("\n--- Файлы после 2025-01-13 ---");
var recentFiles = db.files.find(
  { created_at: { $gt: new Date("2025-01-13T00:00:00Z") } },
  { name: 1, created_at: 1 }
).sort({ created_at: 1 }).toArray();
printjson(recentFiles.map(f => f.name));

print("\n=== Все запросы выполнены ===");
