#pragma once

#include "httplib.h"

#include "catalog.hpp"
#include "order_store.hpp"

namespace water {

// Регистрирует обработчики REST API:
//   GET   /api/health              — проверка, что сервер работает
//   GET   /api/products            — каталог товаров
//   POST  /api/orders              — оформить заказ
//   GET   /api/orders              — список всех заказов (для оператора)
//   GET   /api/orders/{id}         — информация о заказе
//   PATCH /api/orders/{id}/status  — изменить статус заказа
void register_routes(httplib::Server& server, const Catalog& catalog, OrderStore& store);

}  // namespace water
