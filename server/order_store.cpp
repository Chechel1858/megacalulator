#include "order_store.hpp"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace water {

namespace {

std::string format_now(const char* format) {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, format);
    return out.str();
}

}  // namespace

std::string current_datetime() { return format_now("%Y-%m-%d %H:%M:%S"); }

std::string current_date() { return format_now("%Y-%m-%d"); }

OrderStore::OrderStore(std::string file_path) : file_path_(std::move(file_path)) {
    load();
}

Order OrderStore::add(Order order) {
    std::lock_guard<std::mutex> lock(mutex_);
    order.id = next_id_++;
    order.status = OrderStatus::New;
    order.created_at = current_datetime();
    orders_.push_back(order);
    try {
        save();
    } catch (...) {
        // Заказ, который не удалось сохранить, не должен остаться в памяти.
        orders_.pop_back();
        --next_id_;
        throw;
    }
    return order;
}

std::vector<Order> OrderStore::all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return orders_;
}

std::optional<Order> OrderStore::find(int id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(orders_.begin(), orders_.end(),
                                 [id](const Order& order) { return order.id == id; });
    if (it == orders_.end()) return std::nullopt;
    return *it;
}

bool OrderStore::set_status(int id, OrderStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(orders_.begin(), orders_.end(),
                                 [id](const Order& order) { return order.id == id; });
    if (it == orders_.end()) return false;
    const OrderStatus previous = it->status;
    it->status = status;
    try {
        save();
    } catch (...) {
        it->status = previous;
        throw;
    }
    return true;
}

void OrderStore::load() {
    if (file_path_.empty() || !std::filesystem::exists(file_path_)) return;

    std::ifstream in(file_path_);
    if (!in) {
        throw std::runtime_error("Не удалось открыть файл заказов: " + file_path_);
    }
    const auto data = nlohmann::json::parse(in);
    orders_ = data.at("orders").get<std::vector<Order>>();
    for (const auto& order : orders_) {
        next_id_ = std::max(next_id_, order.id + 1);
    }
}

void OrderStore::save() const {
    if (file_path_.empty()) return;

    // Сначала пишем во временный файл, затем подменяем основной,
    // чтобы при сбое не остаться с наполовину записанным файлом.
    const std::string temp_path = file_path_ + ".tmp";
    {
        std::ofstream out(temp_path, std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Не удалось записать файл заказов: " + temp_path);
        }
        out << nlohmann::json{{"orders", orders_}}.dump(2);
    }
    std::filesystem::rename(temp_path, file_path_);
}

}  // namespace water
