db.runCommand({
  collMod: "folders",
  validator: {
    $jsonSchema: {
      bsonType: "object",
      title: "Folder",
      required: ["_id", "name", "owner_id", "created_at"],
      additionalProperties: false,
      properties: {
        _id: {
          bsonType: "string",
          description: "UUID папки (обязательное)",
          pattern: "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"
        },
        name: {
          bsonType: "string",
          description: "Имя папки (обязательное, не пустое)",
          minLength: 1,
          maxLength: 255
        },
        owner_id: {
          bsonType: "string",
          description: "UUID владельца (обязательное)",
          pattern: "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"
        },
        created_at: {
          bsonType: "date",
          description: "Дата создания (обязательная)"
        }
      }
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});

print("Валидация folders настроена");




db.runCommand({
  collMod: "files",
  validator: {
    $jsonSchema: {
      bsonType: "object",
      title: "File",
      required: ["_id", "name", "folder_id", "owner_id", "content_type", "size", "content", "created_at"],
      additionalProperties: false,
      properties: {
        _id: {
          bsonType: "string",
          description: "UUID файла",
          pattern: "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"
        },
        name: {
          bsonType: "string",
          description: "Имя файла",
          minLength: 1,
          maxLength: 255
        },
        folder_id: {
          bsonType: "string",
          description: "UUID папки",
          pattern: "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"
        },
        owner_id: {
          bsonType: "string",
          description: "UUID владельца",
          pattern: "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"
        },
        content_type: {
          bsonType: "string",
          description: "MIME-тип",
          minLength: 1,
          maxLength: 255
        },
        size: {
          bsonType: "long",
          description: "Размер файла в байтах (>=0)",
          minimum: 0
        },
        content: {
          bsonType: "string",
          description: "Содержимое в Base64"
        },
        created_at: {
          bsonType: "date",
          description: "Дата создания"
        }
      }
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});

print("Валидация files настроена");




print("\n папка без обязательного поля name");
try {
  db.folders.insertOne({
    _id: "cccccccc-0000-4000-a000-000000000001",
    owner_id: "00000000-0000-4000-a000-000000000002",
    created_at: new Date()
  });
  print("ОШИБКА: документ вставлен, но не должен был");
} catch (e) {
  print("OK: вставка отклонена —", e.message.substring(0, 80));
}

print("\n папка с пустым именем (minLength: 1)");
try {
  db.folders.insertOne({
    _id: "cccccccc-0000-4000-a000-000000000002",
    name: "",
    owner_id: "00000000-0000-4000-a000-000000000002",
    created_at: new Date()
  });
  print("ОШИБКА: документ вставлен, но не должен был");
} catch (e) {
  print("OK: вставка отклонена —", e.message.substring(0, 80));
}

print("\n папка с некорректным UUID владельца");
try {
  db.folders.insertOne({
    _id: "cccccccc-0000-4000-a000-000000000003",
    name: "TestFolder",
    owner_id: "not-a-valid-uuid",
    created_at: new Date()
  });
  print("ОШИБКА: документ вставлен, но не должен был");
} catch (e) {
  print("OK: вставка отклонена —", e.message.substring(0, 80));
}

print("\n файл с отрицательным размером (minimum: 0)");
try {
  db.files.insertOne({
    _id: "dddddddd-0000-4000-a000-000000000001",
    name: "test.txt",
    folder_id: "10000000-0000-4000-a000-000000000001",
    owner_id: "00000000-0000-4000-a000-000000000002",
    content_type: "text/plain",
    size: NumberLong(-1),
    content: "dGVzdA==",
    created_at: new Date()
  });
  print("ОШИБКА: документ вставлен, но не должен был");
} catch (e) {
  print("OK: вставка отклонена —", e.message.substring(0, 80));
}

print("\n валидный документ (должен вставиться)");
try {
  db.folders.insertOne({
    _id: "eeeeeeee-0000-4000-a000-000000000001",
    name: "ValidFolder",
    owner_id: "00000000-0000-4000-a000-000000000002",
    created_at: new Date()
  });
  print("OK: валидный документ успешно вставлен");
  db.folders.deleteOne({ _id: "eeeeeeee-0000-4000-a000-000000000001" });
} catch (e) {
  print("ОШИБКА: валидный документ отклонён —", e.message);
}
