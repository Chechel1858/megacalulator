#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "order.hpp"

namespace water {

// Потокобезопасное хранилище заказов.
// Если указан путь к файлу, заказы сохраняются в него в формате JSON
// и загружаются обратно при следующем запуске сервера.
class OrderStore {
public:
    explicit OrderStore(std::string file_path = {});

    // Присваивает заказу номер и время создания, сохраняет его и возвращает.
    Order add(Order order);

    std::vector<Order> all() const;
    std::optional<Order> find(int id) const;

    // Возвращает false, если заказа с таким номером нет.
    bool set_status(int id, OrderStatus status);

private:
    void load();
    void save() const;

    mutable std::mutex mutex_;
    std::vector<Order> orders_;
    int next_id_ = 1;
    std::string file_path_;
};

// Текущая локальная дата и время: "ГГГГ-ММ-ДД ЧЧ:ММ:СС".
std::string current_datetime();
// Текущая локальная дата: "ГГГГ-ММ-ДД".
std::string current_date();

}  // namespace water
