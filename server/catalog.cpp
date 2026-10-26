#include "catalog.hpp"

#include <utility>

namespace water {

void to_json(nlohmann::json& j, const Product& p) {
    j = nlohmann::json{
        {"id", p.id},
        {"name", p.name},
        {"description", p.description},
        {"price", p.price},
    };
}

Catalog::Catalog(std::vector<Product> products) : products_(std::move(products)) {}

Catalog Catalog::default_catalog() {
    return Catalog({
        {1, "Вода питьевая 19 л", "Артезианская вода в многоразовой бутыли для кулера", 350},
        {2, "Вода питьевая 5 л", "Артезианская вода в удобной канистре", 120},
        {3, "Вода детская 5 л", "Подходит для приготовления детского питания", 160},
        {4, "Минеральная вода 1,5 л (6 шт.)", "Упаковка газированной минеральной воды", 390},
        {5, "Помпа для бутыли", "Механическая помпа для бутылей 19 л", 450},
    });
}

const Product* Catalog::find(int id) const {
    for (const auto& product : products_) {
        if (product.id == id) {
            return &product;
        }
    }
    return nullptr;
}

}  // namespace water
