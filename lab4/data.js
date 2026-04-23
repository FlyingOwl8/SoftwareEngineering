db.folders.deleteMany({});
db.files.deleteMany({});

const users = {
  admin:  "00000000-0000-4000-a000-000000000001",
  alice:  "00000000-0000-4000-a000-000000000002",
  bob:    "00000000-0000-4000-a000-000000000003",
  carol:  "00000000-0000-4000-a000-000000000004",
  dave:   "00000000-0000-4000-a000-000000000005",
  eve:    "00000000-0000-4000-a000-000000000006",
  frank:  "00000000-0000-4000-a000-000000000007",
  grace:  "00000000-0000-4000-a000-000000000008",
  henry:  "00000000-0000-4000-a000-000000000009",
  irene:  "00000000-0000-4000-a000-000000000010",
};


db.folders.insertMany([
  { _id: "10000000-0000-4000-a000-000000000001", name: "Documents",  owner_id: users.alice,  created_at: new Date("2025-01-10T08:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000002", name: "Photos",     owner_id: users.alice,  created_at: new Date("2025-01-10T08:01:00Z") },
  { _id: "10000000-0000-4000-a000-000000000003", name: "Music",      owner_id: users.alice,  created_at: new Date("2025-01-10T08:02:00Z") },
  { _id: "10000000-0000-4000-a000-000000000004", name: "Work",       owner_id: users.bob,    created_at: new Date("2025-01-11T09:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000005", name: "Personal",   owner_id: users.bob,    created_at: new Date("2025-01-11T09:01:00Z") },
  { _id: "10000000-0000-4000-a000-000000000006", name: "Projects",   owner_id: users.carol,  created_at: new Date("2025-01-12T10:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000007", name: "Backup",     owner_id: users.dave,   created_at: new Date("2025-01-13T11:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000008", name: "Videos",     owner_id: users.eve,    created_at: new Date("2025-01-14T12:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000009", name: "Archives",   owner_id: users.frank,  created_at: new Date("2025-01-15T13:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000010", name: "Downloads",  owner_id: users.grace,  created_at: new Date("2025-01-16T14:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000011", name: "Reports",    owner_id: users.henry,  created_at: new Date("2025-01-17T15:00:00Z") },
  { _id: "10000000-0000-4000-a000-000000000012", name: "Shared",     owner_id: users.irene,  created_at: new Date("2025-01-18T16:00:00Z") },
]);


db.files.insertMany([
  {
    _id: "20000000-0000-4000-a000-000000000001",
    name: "readme.txt",
    folder_id: "10000000-0000-4000-a000-000000000001",
    owner_id: users.alice,
    content_type: "text/plain",
    size: 42,
    content: "SGVsbG8sIHdvcmxkIQ==",
    created_at: new Date("2025-01-10T08:10:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000002",
    name: "notes.txt",
    folder_id: "10000000-0000-4000-a000-000000000001",
    owner_id: users.alice,
    content_type: "text/plain",
    size: 128,
    content: "TXkgbm90ZXM=",
    created_at: new Date("2025-01-10T08:11:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000003",
    name: "photo1.jpg",
    folder_id: "10000000-0000-4000-a000-000000000002",
    owner_id: users.alice,
    content_type: "image/jpeg",
    size: 204800,
    content: "/9j/4AAQSkZJRgAB",
    created_at: new Date("2025-01-10T08:20:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000004",
    name: "photo2.jpg",
    folder_id: "10000000-0000-4000-a000-000000000002",
    owner_id: users.alice,
    content_type: "image/jpeg",
    size: 184320,
    content: "/9j/4AAQSkZJRgAC",
    created_at: new Date("2025-01-10T08:21:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000005",
    name: "song.mp3",
    folder_id: "10000000-0000-4000-a000-000000000003",
    owner_id: users.alice,
    content_type: "audio/mpeg",
    size: 3145728,
    content: "SUQzBAAAAAAAI1RTU0UAAAAPAAADTGF2",
    created_at: new Date("2025-01-10T08:30:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000006",
    name: "report.pdf",
    folder_id: "10000000-0000-4000-a000-000000000004",
    owner_id: users.bob,
    content_type: "application/pdf",
    size: 512000,
    content: "JVBERi0xLjQKJcOk",
    created_at: new Date("2025-01-11T09:10:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000007",
    name: "budget.xlsx",
    folder_id: "10000000-0000-4000-a000-000000000004",
    owner_id: users.bob,
    content_type: "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    size: 98304,
    content: "UEsDBBQABgAIAAAAIQ==",
    created_at: new Date("2025-01-11T09:11:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000008",
    name: "diary.txt",
    folder_id: "10000000-0000-4000-a000-000000000005",
    owner_id: users.bob,
    content_type: "text/plain",
    size: 256,
    content: "VG9kYXkgd2FzIGEgZ29vZCBkYXku",
    created_at: new Date("2025-01-11T09:20:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000009",
    name: "design.png",
    folder_id: "10000000-0000-4000-a000-000000000006",
    owner_id: users.carol,
    content_type: "image/png",
    size: 102400,
    content: "iVBORw0KGgoAAAANSUhEUgAA",
    created_at: new Date("2025-01-12T10:10:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000010",
    name: "backup.tar.gz",
    folder_id: "10000000-0000-4000-a000-000000000007",
    owner_id: users.dave,
    content_type: "application/gzip",
    size: 1048576,
    content: "H4sIAAAAAAAAA+",
    created_at: new Date("2025-01-13T11:10:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000011",
    name: "movie.mp4",
    folder_id: "10000000-0000-4000-a000-000000000008",
    owner_id: users.eve,
    content_type: "video/mp4",
    size: 10485760,
    content: "AAAAIGZ0eXBpc29t",
    created_at: new Date("2025-01-14T12:10:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000012",
    name: "archive.zip",
    folder_id: "10000000-0000-4000-a000-000000000009",
    owner_id: users.frank,
    content_type: "application/zip",
    size: 2097152,
    content: "UEsDBAoAAAAAAA==",
    created_at: new Date("2025-01-15T13:10:00Z")
  },
  {
    _id: "20000000-0000-4000-a000-000000000013",
    name: "q4_report.pdf",
    folder_id: "10000000-0000-4000-a000-000000000011",
    owner_id: users.henry,
    content_type: "application/pdf",
    size: 307200,
    content: "JVBERi0xLjUKJcOk",
    created_at: new Date("2025-01-17T15:10:00Z")
  },
]);


// Индексы

db.folders.createIndex({ owner_id: 1 });
db.folders.createIndex({ owner_id: 1, name: 1 }, { unique: true });

db.files.createIndex({ folder_id: 1, name: 1 }, { unique: true });
db.files.createIndex({ owner_id: 1 });

print("Данные загружены: " + db.folders.countDocuments() + " папок, " + db.files.countDocuments() + " файлов");
