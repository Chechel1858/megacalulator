#include "order.hpp"

#include <cctype>
#include <map>

namespace water {

namespace {

constexpr std::size_t kMaxNameLength = 100;
constexpr std::size_t kMaxAddressLength = 200;
constexpr std::size_t kMaxCommentLength = 500;
constexpr int kMaxQuantity = 100;

std::string trim(const std::string& text) {
    const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && is_space(text[begin])) ++begin;
    while (end > begin && is_space(text[end - 1])) --end;
    return text.substr(begin, end - begin);
}

// Длина строки в символах, а не в байтах (русские буквы в UTF-8 занимают 2 байта).
std::size_t utf8_length(const std::string& text) {
    std::size_t length = 0;
    for (unsigned char c : text) {
        if ((c & 0xC0) != 0x80) ++length;
    }
    return length;
}

// Возвращает строковое поле или пустую строку, если поля нет или оно не строка.
std::string get_string(const nlohmann::json& body, const char* key) {
    const auto it = body.find(key);
    if (it == body.end() || !it->is_string()) return {};
    return trim(it->get<std::string>());
}

bool is_valid_phone(const std::string& phone) {
    int digits = 0;
    for (std::size_t i = 0; i < phone.size(); ++i) {
        const char c = phone[i];
        if (std::isdigit(static_cast<unsigned char>(c))) {
            ++digits;
        } else if (c == '+' && i == 0) {
            continue;
        } else if (c != ' ' && c != '-' && c != '(' && c != ')') {
            return false;
        }
    }
    return digits >= 10 && digits <= 15;
}

bool is_valid_date(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (std::size_t i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }
    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (month < 1 || month > 12 || day < 1) return false;

    static const int kDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    const int max_day = (month == 2 && leap) ? 29 : kDaysInMonth[month - 1];
    return day <= max_day;
}

}  // namespace

std::string to_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::New: return "new";
        case OrderStatus::Confirmed: return "confirmed";
        case OrderStatus::Delivering: return "delivering";
        case OrderStatus::Delivered: return "delivered";
        case OrderStatus::Cancelled: return "cancelled";
    }
    return "new";
}

std::optional<OrderStatus> status_from_string(const std::string& text) {
    for (auto status : {OrderStatus::New, OrderStatus::Confirmed, OrderStatus::Delivering,
                        OrderStatus::Delivered, OrderStatus::Cancelled}) {
        if (to_string(status) == text) return status;
    }
    return std::nullopt;
}

void to_json(nlohmann::json& j, const OrderItem& item) {
    j = nlohmann::json{
        {"product_id", item.product_id},
        {"product_name", item.product_name},
        {"quantity", item.quantity},
        {"price", item.price},
    };
}

void from_json(const nlohmann::json& j, OrderItem& item) {
    j.at("product_id").get_to(item.product_id);
    j.at("product_name").get_to(item.product_name);
    j.at("quantity").get_to(item.quantity);
    j.at("price").get_to(item.price);
}

void to_json(nlohmann::json& j, const Order& order) {
    j = nlohmann::json{
        {"id", order.id},
        {"customer_name", order.customer_name},
        {"phone", order.phone},
        {"address", order.address},
        {"delivery_date", order.delivery_date},
        {"comment", order.comment},
        {"items", order.items},
        {"total", order.total},
        {"status", to_string(order.status)},
        {"created_at", order.created_at},
    };
}

void from_json(const nlohmann::json& j, Order& order) {
    j.at("id").get_to(order.id);
    j.at("customer_name").get_to(order.customer_name);
    j.at("phone").get_to(order.phone);
    j.at("address").get_to(order.address);
    j.at("delivery_date").get_to(order.delivery_date);
    order.comment = j.value("comment", "");
    j.at("items").get_to(order.items);
    j.at("total").get_to(order.total);
    order.status = status_from_string(j.value("status", "new")).value_or(OrderStatus::New);
    order.created_at = j.value("created_at", "");
}

std::vector<std::string> build_order(const nlohmann::json& body, const Catalog& catalog,
                                     const std::string& today, Order& out) {
    std::vector<std::string> errors;
    if (!body.is_object()) {
        return {"Неверный формат заказа"};
    }

    Order order;
    order.customer_name = get_string(body, "customer_name");
    order.phone = get_string(body, "phone");
    order.address = get_string(body, "address");
    order.delivery_date = get_string(body, "delivery_date");
    order.comment = get_string(body, "comment");

    if (order.customer_name.empty()) {
        errors.push_back("Укажите имя");
    } else if (utf8_length(order.customer_name) > kMaxNameLength) {
        errors.push_back("Имя слишком длинное");
    }

    if (!is_valid_phone(order.phone)) {
        errors.push_back("Укажите корректный номер телефона");
    }

    if (order.address.empty()) {
        errors.push_back("Укажите адрес доставки");
    } else if (utf8_length(order.address) > kMaxAddressLength) {
        errors.push_back("Адрес слишком длинный");
    }

    if (!is_valid_date(order.delivery_date)) {
        errors.push_back("Укажите дату доставки");
    } else if (order.delivery_date < today) {
        // Даты в формате ГГГГ-ММ-ДД можно сравнивать как строки.
        errors.push_back("Дата доставки не может быть в прошлом");
    }

    if (utf8_length(order.comment) > kMaxCommentLength) {
        errors.push_back("Комментарий слишком длинный");
    }

    const auto items = body.find("items");
    if (items == body.end() || !items->is_array() || items->empty()) {
        errors.push_back("Добавьте в заказ хотя бы один товар");
    } else {
        // Одинаковые товары объединяются в одну позицию, порядок сохраняется.
        std::map<int, std::size_t> position_by_product;
        for (const auto& item : *items) {
            const auto id = item.find("product_id");
            const auto quantity = item.find("quantity");
            if (!item.is_object() || id == item.end() || !id->is_number_integer() ||
                quantity == item.end() || !quantity->is_number_integer()) {
                errors.push_back("Неверный формат позиции заказа");
                continue;
            }

            const Product* product = catalog.find(id->get<int>());
            if (product == nullptr) {
                errors.push_back("Товар не найден: " + std::to_string(id->get<int>()));
                continue;
            }

            const auto count = quantity->get<long long>();
            if (count < 1 || count > kMaxQuantity) {
                errors.push_back("Количество товара «" + product->name + "» должно быть от 1 до " +
                                 std::to_string(kMaxQuantity));
                continue;
            }

            const auto existing = position_by_product.find(product->id);
            if (existing != position_by_product.end()) {
                order.items[existing->second].quantity += static_cast<int>(count);
            } else {
                position_by_product[product->id] = order.items.size();
                order.items.push_back({product->id, product->name, static_cast<int>(count),
                                       product->price});
            }
        }
    }

    if (!errors.empty()) {
        return errors;
    }

    for (const auto& item : order.items) {
        order.total += item.price * item.quantity;
    }
    out = std::move(order);
    return errors;
}

}  // namespace water
