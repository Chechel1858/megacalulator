#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace water {

// Товар из каталога. Цена хранится в целых рублях.
struct Product {
    int id = 0;
    std::string name;
    std::string description;
    int price = 0;
};

void to_json(nlohmann::json& j, const Product& p);

// Каталог товаров, доступных для заказа.
class Catalog {
public:
    explicit Catalog(std::vector<Product> products);

    // Стандартный ассортимент магазина.
    static Catalog default_catalog();

    const std::vector<Product>& products() const { return products_; }

    // Возвращает nullptr, если товара с таким id нет.
    const Product* find(int id) const;

private:
    std::vector<Product> products_;
};

}  // namespace water
