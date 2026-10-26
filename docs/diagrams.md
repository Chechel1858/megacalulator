# Диаграммы системы заказа воды

Все диаграммы есть в формате **draw.io**: их можно открыть и редактировать на [app.diagrams.net](https://app.diagrams.net/)
через «Файл → Открыть» или в приложении draw.io.

| Диаграмма | draw.io | Картинка |
|---|---|---|
| Варианты использования (Use Case) | [`use-case.drawio`](use-case.drawio) | [`use-case.png`](use-case.png) |
| Последовательность | [`sequence.drawio`](sequence.drawio) | [`sequence.png`](sequence.png) |
| Классы | [`classes.drawio`](classes.drawio) | [`classes.png`](classes.png) |

Ниже те же диаграммы последовательности и классов на [Mermaid](https://mermaid.js.org/), GitHub рисует их прямо на этой странице.

## 1. Диаграмма последовательности: оформление заказа

Показывает, что происходит, когда клиент оформляет заказ на сайте и когда оператор меняет его статус.

```mermaid
sequenceDiagram
    autonumber
    actor Client as Клиент
    participant Site as Сайт клиента<br/>(index.html, app.js)
    participant Server as C++ сервер<br/>(cpp-httplib)
    participant Logic as build_order()<br/>проверка заказа
    participant Store as OrderStore
    participant File as orders.json
    actor Operator as Оператор

    Client->>Site: Открывает сайт
    Site->>Server: GET /api/products
    Server-->>Site: 200 OK, каталог товаров (JSON)
    Client->>Site: Выбирает воду, заполняет адрес и дату
    Site->>Server: POST /api/orders (JSON заказа)
    Server->>Logic: Проверить данные и посчитать сумму
    alt Данные с ошибками
        Logic-->>Server: Список ошибок
        Server-->>Site: 400 Bad Request {errors}
        Site-->>Client: Показывает, что исправить
    else Данные корректны
        Logic-->>Server: Готовый заказ
        Server->>Store: add(order)
        Store->>Store: Присвоить номер и время создания
        Store->>File: Сохранить все заказы
        Store-->>Server: Заказ №N
        Server-->>Site: 201 Created {заказ}
        Site-->>Client: «Заказ №N принят»
    end

    Operator->>Server: GET /api/orders (admin.html)
    Server-->>Operator: Список заказов
    Operator->>Server: PATCH /api/orders/N/status {"status": "delivering"}
    Server->>Store: set_status(N, Delivering)
    Store->>File: Сохранить все заказы
    Server-->>Operator: 200 OK, обновлённый заказ

    Client->>Site: Вводит номер заказа
    Site->>Server: GET /api/orders/N
    Server-->>Site: Заказ со статусом «В пути»
```

## 2. Диаграмма классов: устройство сервера

Показывает, из каких классов состоит C++ сервер и как они связаны.

```mermaid
classDiagram
    direction LR

    class Product {
        +int id
        +string name
        +string description
        +int price
    }

    class Catalog {
        -vector~Product~ products_
        +default_catalog()$ Catalog
        +products() vector~Product~
        +find(int id) Product*
    }

    class OrderStatus {
        <<enumeration>>
        New
        Confirmed
        Delivering
        Delivered
        Cancelled
    }

    class OrderItem {
        +int product_id
        +string product_name
        +int quantity
        +int price
    }

    class Order {
        +int id
        +string customer_name
        +string phone
        +string address
        +string delivery_date
        +string comment
        +vector~OrderItem~ items
        +int total
        +OrderStatus status
        +string created_at
    }

    class OrderStore {
        -mutex mutex_
        -vector~Order~ orders_
        -int next_id_
        -string file_path_
        +add(Order) Order
        +all() vector~Order~
        +find(int id) optional~Order~
        +set_status(int id, OrderStatus) bool
        -load()
        -save()
    }

    class Api {
        <<api.cpp>>
        +register_routes(Server, Catalog, OrderStore)
    }

    class HttpServer {
        <<httplib::Server>>
        +Get()
        +Post()
        +Patch()
        +set_mount_point()
        +listen()
    }

    Catalog "1" *-- "*" Product : содержит
    Order "1" *-- "1..*" OrderItem : состоит из
    Order --> OrderStatus : статус
    OrderItem ..> Product : копия цены и названия
    OrderStore "1" o-- "*" Order : хранит
    Api ..> Catalog : читает каталог
    Api ..> OrderStore : создаёт и меняет заказы
    Api ..> HttpServer : регистрирует маршруты
```
