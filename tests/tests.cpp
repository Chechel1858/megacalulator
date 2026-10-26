// Модульные тесты логики заказов (без сетевой части).
// Запуск: ctest --test-dir build   или   build/water_tests

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

#include "catalog.hpp"
#include "order.hpp"
#include "order_store.hpp"

using namespace water;
using nlohmann::json;

namespace {

int g_failed = 0;
int g_passed = 0;

#define CHECK(condition)                                                              \
    do {                                                                              \
        if (condition) {                                                              \
            ++g_passed;                                                               \
        } else {                                                                      \
            ++g_failed;                                                               \
            std::cerr << __FILE__ << ":" << __LINE__ << ": не выполнено: " #condition \
                      << "\n";                                                        \
        }                                                                             \
    } while (false)

const std::string kToday = "2026-10-26";

json valid_request() {
    return {
        {"customer_name", "Иван Петров"},
        {"phone", "+7 (900) 123-45-67"},
        {"address", "г. Москва, ул. Ленина, д. 1, кв. 5"},
        {"delivery_date", "2026-10-27"},
        {"comment", "Позвонить за час"},
        {"items", {{{"product_id", 1}, {"quantity", 2}}, {{"product_id", 5}, {"quantity", 1}}}},
    };
}

bool has_error(const std::vector<std::string>& errors, const std::string& text) {
    for (const auto& error : errors) {
        if (error.find(text) != std::string::npos) return true;
    }
    return false;
}

void test_valid_order() {
    const Catalog catalog = Catalog::default_catalog();
    Order order;
    const auto errors = build_order(valid_request(), catalog, kToday, order);
    CHECK(errors.empty());
    CHECK(order.items.size() == 2);
    CHECK(order.items[0].product_name == "Вода питьевая 19 л");
    CHECK(order.total == 2 * 350 + 450);
    CHECK(order.status == OrderStatus::New);
}

void test_duplicate_items_are_merged() {
    const Catalog catalog = Catalog::default_catalog();
    json request = valid_request();
    request["items"] = {{{"product_id", 2}, {"quantity", 1}}, {{"product_id", 2}, {"quantity", 3}}};
    Order order;
    CHECK(build_order(request, catalog, kToday, order).empty());
    CHECK(order.items.size() == 1);
    CHECK(order.items[0].quantity == 4);
    CHECK(order.total == 4 * 120);
}

void test_invalid_fields() {
    const Catalog catalog = Catalog::default_catalog();
    Order order;

    json request = valid_request();
    request["customer_name"] = "   ";
    CHECK(has_error(build_order(request, catalog, kToday, order), "имя"));

    request = valid_request();
    request["phone"] = "12345";
    CHECK(has_error(build_order(request, catalog, kToday, order), "телефон"));

    request = valid_request();
    request["phone"] = "8900abc4567";
    CHECK(has_error(build_order(request, catalog, kToday, order), "телефон"));

    request = valid_request();
    request.erase("address");
    CHECK(has_error(build_order(request, catalog, kToday, order), "адрес"));

    request = valid_request();
    request["delivery_date"] = "2026-10-25";
    CHECK(has_error(build_order(request, catalog, kToday, order), "в прошлом"));

    request = valid_request();
    request["delivery_date"] = "2026-02-30";
    CHECK(has_error(build_order(request, catalog, kToday, order), "дату"));

    request = valid_request();
    request["items"] = json::array();
    CHECK(has_error(build_order(request, catalog, kToday, order), "хотя бы один"));

    request = valid_request();
    request["items"] = {{{"product_id", 999}, {"quantity", 1}}};
    CHECK(has_error(build_order(request, catalog, kToday, order), "не найден"));

    request = valid_request();
    request["items"] = {{{"product_id", 1}, {"quantity", 0}}};
    CHECK(has_error(build_order(request, catalog, kToday, order), "Количество"));

    request = valid_request();
    request["items"] = {{{"product_id", "1"}, {"quantity", 1}}};
    CHECK(has_error(build_order(request, catalog, kToday, order), "формат"));

    CHECK(has_error(build_order(json::array(), catalog, kToday, order), "формат"));
}

void test_status_strings() {
    for (auto status : {OrderStatus::New, OrderStatus::Confirmed, OrderStatus::Delivering,
                        OrderStatus::Delivered, OrderStatus::Cancelled}) {
        CHECK(status_from_string(to_string(status)) == status);
    }
    CHECK(!status_from_string("lost").has_value());
}

void test_store_in_memory() {
    const Catalog catalog = Catalog::default_catalog();
    Order draft;
    CHECK(build_order(valid_request(), catalog, kToday, draft).empty());

    OrderStore store;
    const Order first = store.add(draft);
    const Order second = store.add(draft);
    CHECK(first.id == 1);
    CHECK(second.id == 2);
    CHECK(!first.created_at.empty());
    CHECK(store.all().size() == 2);

    CHECK(store.set_status(2, OrderStatus::Delivering));
    CHECK(store.find(2)->status == OrderStatus::Delivering);
    CHECK(!store.set_status(42, OrderStatus::Delivered));
    CHECK(!store.find(42).has_value());
}

void test_store_persistence() {
    const auto path = (std::filesystem::temp_directory_path() / "water_orders_test.json").string();
    std::filesystem::remove(path);

    const Catalog catalog = Catalog::default_catalog();
    Order draft;
    CHECK(build_order(valid_request(), catalog, kToday, draft).empty());
    {
        OrderStore store(path);
        store.add(draft);
        store.add(draft);
        store.set_status(1, OrderStatus::Delivered);
    }

    OrderStore reloaded(path);
    const auto orders = reloaded.all();
    CHECK(orders.size() == 2);
    CHECK(reloaded.find(1)->status == OrderStatus::Delivered);
    CHECK(reloaded.find(2)->customer_name == "Иван Петров");
    CHECK(reloaded.find(2)->items.size() == 2);
    // Нумерация продолжается после перезапуска.
    CHECK(reloaded.add(draft).id == 3);

    std::filesystem::remove(path);
}

}  // namespace

int main() {
    test_valid_order();
    test_duplicate_items_are_merged();
    test_invalid_fields();
    test_status_strings();
    test_store_in_memory();
    test_store_persistence();

    std::cout << "Пройдено проверок: " << g_passed << ", провалено: " << g_failed << "\n";
    return g_failed == 0 ? 0 : 1;
}
