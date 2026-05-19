workspace  {
    !identifiers hierarchical

    model {
        user = person "Пользователь" "Клиент, работающий с API"
        admin = person "Администратор"

        cloudStorage = softwareSystem "Cloud Storage" "Система хранения файлов" {

            main = container "Main Service" {
                description  "Вызывает внутренние сервисы"
                technology  "C++ userver"
                tags  "main"
            }

            userService = container "User Service" {
                description  "Регистрация, аутентификация, управление пользовательскими данными"
                technology  "C++ userver"
                tags  "Service"
            }

            folderFileService = container "FolderFile Service" {
                description  "Управляет папками и файлами; публикует FolderCreated в Kafka"
                technology  "C++ userver"
                tags  "Service"
            }

            folderConsumer = container "Folder Consumer" {
                description  "Читает FolderCreated из Kafka и записывает папки в MongoDB"
                technology  "C++ userver"
                tags  "Service"
            }

            kafka = container "Event Topic" {
                description  "Журнал событий для CQRS"
                technology  "Apache Kafka"
                tags  "EventTopic"
            }

            userDb = container "User Database" {
                description  "Хранит учётные данные пользователей"
                technology  "PostgreSQL"
                tags  "Database"
            }

            folderFileDb = container "FolderFile Database" {
                description  "Хранит папки, метаданные и содержимое файлов"
                technology  "MongoDB"
                tags  "Database"
            }

            user -> cloudStorage.main "Вызывает API (как обычный пользователь)" "HTTPS/REST"
            admin -> cloudStorage.main "Вызывает API (как администратор)" "HTTPS/REST"

            cloudStorage.main -> cloudStorage.userService "Регистрация, поиск, проверка и управление данными пользователей" "HTTP/REST"
            cloudStorage.main -> cloudStorage.folderFileService "Управление папками и файлами" "HTTP/REST"

            cloudStorage.userService -> cloudStorage.userDb "CRUD запросы по пользовательским данным" "SQL/TCP"
            cloudStorage.folderFileService -> cloudStorage.folderFileDb "Чтение папок/файлов" "MQL/TCP"

            cloudStorage.folderFileService -> cloudStorage.kafka "Публикация FolderCreated" "Kafka"
            cloudStorage.kafka -> cloudStorage.folderConsumer "Доставка FolderCreated" "Kafka"
            cloudStorage.folderConsumer -> cloudStorage.folderFileDb "Запись папки в MongoDB" "MQL/TCP"
        }
    }

    views {
        systemContext cloudStorage  {
            include *
            autolayout lr
        }

        container cloudStorage  {
            include *
            autolayout lr
        }

        dynamic cloudStorage  {
            description "Сценарий: поиск пользователя"
            autolayout lr

            admin -> cloudStorage.main "Запрос на поиск пользователя (с JWT администратора)" "HTTPS/REST"
            cloudStorage.main -> cloudStorage.userService "Проверка JWT и роли" "HTTP/REST"
            cloudStorage.userService -> cloudStorage.main "Роль admin подтверждена" "HTTP/REST"
            cloudStorage.main -> cloudStorage.userService "Запрос на поиск пользователей" "HTTP/REST"
            cloudStorage.userService -> cloudStorage.userDb "SELECT данных пользователя" "SQL/TCP"
            cloudStorage.userDb -> cloudStorage.userService "Результаты поиска" "SQL/TCP"
            cloudStorage.userService -> cloudStorage.main "Список пользователей" "HTTP/REST"
            cloudStorage.main -> admin "Ответ OK с данными пользователей" "HTTPS/REST"
        }

        dynamic cloudStorage "CreateFolder" {
            description "Сценарий: создание папки (CQRS + Kafka)"
            autolayout lr

            user -> cloudStorage.main "POST /api/v1/folders {name}" "HTTPS/REST"
            cloudStorage.main -> cloudStorage.folderFileService "Создать папку" "HTTP/REST"
            cloudStorage.folderFileService -> cloudStorage.folderFileDb "Проверка дублирования имени" "MQL/TCP"
            cloudStorage.folderFileService -> cloudStorage.kafka "FolderCreated (JSON)" "Kafka"
            cloudStorage.folderFileService -> cloudStorage.main "HTTP 201" "HTTP/REST"
            cloudStorage.main -> user "HTTP 201 с данными папки" "HTTPS/REST"
            cloudStorage.kafka -> cloudStorage.folderConsumer "FolderCreated" "Kafka"
            cloudStorage.folderConsumer -> cloudStorage.folderFileDb "SaveFolder → MongoDB" "MQL/TCP"
        }

        styles {
            element "Person" {
                shape person
                background #08427b
                color #ffffff
            }
            element "main" {
                background #4363d8
                color #ffffff
                shape roundedBox
            }
            element "Service" {
                background #3cb44b
                color #ffffff
                shape roundedBox
            }
            element "Database" {
                shape cylinder
                background #ffe119
                color #000000
            }
            element "EventTopic" {
                shape pipe
                background #f5da81
                color #000000
            }
            element "ExternalSystem" {
                background #dddddd
                color #000000
            }
        }
        theme default
    }
}
