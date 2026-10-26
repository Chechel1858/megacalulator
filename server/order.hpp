#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "catalog.hpp"

namespace water {

enum class OrderStatus { New, Confirmed, Delivering, Delivered, Cancelled };

std::string to_string(OrderStatus status);
std::optional<OrderStatus> status_from_string(const std::string& text);

struct OrderItem {
    int product_id = 0;
    std::string product_name;
    int quantity = 0;
    int price = 0;  // цена за единицу на момент заказа
};

struct Order {
    int id = 0;
    std::string customer_name;
    std::string phone;
    std::string address;
    std::string delivery_date;  // ГГГГ-ММ-ДД
    std::string comment;
    std::vector<OrderItem> items;
    int total = 0;
    OrderStatus status = OrderStatus::New;
    std::string created_at;
};

void to_json(nlohmann::json& j, const OrderItem& item);
void from_json(const nlohmann::json& j, OrderItem& item);
void to_json(nlohmann::json& j, const Order& order);
void from_json(const nlohmann::json& j, Order& order);

// Собирает заказ из JSON, присланного сайтом клиента.
// today — текущая дата в формате ГГГГ-ММ-ДД (дата доставки не может быть раньше).
// Возвращает список ошибок; если он пуст, заказ записан в out.
std::vector<std::string> build_order(const nlohmann::json& body, const Catalog& catalog,
                                     const std::string& today, Order& out);

}  // namespace water
