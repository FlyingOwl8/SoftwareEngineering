const alice_id    = "00000000-0000-4000-a000-000000000002";
const bob_id      = "00000000-0000-4000-a000-000000000003";
const folder1_id  = "10000000-0000-4000-a000-000000000001";
const folder4_id  = "10000000-0000-4000-a000-000000000004";
const file1_id    = "20000000-0000-4000-a000-000000000001";




// POST /api/v1/folders — создать папку
// Проверка уникальности имени у владельца
print("\n--- Создание папки ---");
var folderExists = db.folders.findOne({ owner_id: alice_id, name: "NewFolder" });
print("Папка уже существует:", folderExists !== null);

db.folders.insertOne({
  _id: "aaaaaaaa-0000-4000-a000-000000000001",
  name: "NewFolder",
  owner_id: alice_id,
  created_at: new Date()
});
print("Папка создана:", db.folders.findOne({ _id: "aaaaaaaa-0000-4000-a000-000000000001" }).name);

// POST /api/v1/folders/{id}/files — загрузить файл в папку
// Проверка уникальности имени файла в папке
print("\n--- Создание файла ---");
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




// GET /api/v1/folders — список папок пользователя
print("\n--- Список папок пользователя ---");
var aliceFolders = db.folders.find(
  { owner_id: alice_id },
  { name: 1, created_at: 1 }
).sort({ created_at: 1 }).toArray();
printjson(aliceFolders.map(f => f.name));

// GET /api/v1/folders/{id}/files/{name} — получить файл по имени
print("\n--- Получить файл по имени ---");
var file = db.files.findOne({ folder_id: folder1_id, name: "readme.txt" });
printjson({ id: file._id, name: file.name, size: file.size, content_type: file.content_type });

// Получить папку по id (для проверки владельца)
print("\n--- Получить папку по id ---");
var folder = db.folders.findOne({ _id: folder1_id }, { name: 1, owner_id: 1 });
printjson(folder);

// Список файлов в папке
print("\n--- Список файлов в папке ---");
var folderFiles = db.files.find(
  { folder_id: folder1_id },
  { name: 1, content_type: 1, size: 1, created_at: 1 }
).sort({ created_at: 1 }).toArray();
printjson(folderFiles.map(f => ({ name: f.name, size: f.size })));

// Поиск папок по нескольким пользователям ($in)
print("\n--- Папки нескольких пользователей ($in) ---");
var multiFolders = db.folders.find(
  { owner_id: { $in: [alice_id, bob_id] } }
).sort({ owner_id: 1, created_at: 1 }).toArray();
print("Найдено папок:", multiFolders.length);

// Файлы определённого типа у владельца ($and + $eq)
print("\n--- Файлы определённого типа ($and + $eq) ---");
var imageFiles = db.files.find({
  $and: [
    { owner_id: alice_id },
    { content_type: { $in: ["image/jpeg", "image/png"] } }
  ]
}).toArray();
print("Изображений у alice:", imageFiles.length);

// Большие файлы ($gt)
print("\n--- Большие файлы > 1MB ($gt) ---");
var bigFiles = db.files.find(
  { size: { $gt: 1048576 } },
  { name: 1, size: 1, content_type: 1 }
).sort({ size: -1 }).toArray();
printjson(bigFiles.map(f => ({ name: f.name, size: f.size })));




// Переименование папки (нет в API, но демонстрирует updateOne)
print("\n--- Переименование папки ---");
db.folders.updateOne(
  { _id: "aaaaaaaa-0000-4000-a000-000000000001" },
  { $set: { name: "RenamedFolder" } }
);
print("Новое имя:", db.folders.findOne({ _id: "aaaaaaaa-0000-4000-a000-000000000001" }).name);

// Обновление размера файла ($set)
print("\n--- Обновление размера файла ---");
db.files.updateOne(
  { _id: "bbbbbbbb-0000-4000-a000-000000000001" },
  { $set: { size: 200, content: "dXBkYXRlZA==" } }
);
print("Новый размер:", db.files.findOne({ _id: "bbbbbbbb-0000-4000-a000-000000000001" }).size);




// DELETE /api/v1/folders/{id}/files/{fid}/delete — удалить файл
print("\n--- Удаление файла ---");
var deleteResult = db.files.deleteOne({ _id: "bbbbbbbb-0000-4000-a000-000000000001" });
print("Удалено файлов:", deleteResult.deletedCount);

// DELETE /api/v1/folders/{id} — удалить папку (каскадно)
// Сначала удаляем файлы, потом папку
print("\n--- Каскадное удаление папки ---");
var folder_to_delete = "aaaaaaaa-0000-4000-a000-000000000001";
var filesDeleted = db.files.deleteMany({ folder_id: folder_to_delete });
print("Файлов удалено:", filesDeleted.deletedCount);
var folderDeleted = db.folders.deleteOne({ _id: folder_to_delete });
print("Папок удалено:", folderDeleted.deletedCount);




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
