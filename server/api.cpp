#include "api.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace water {

namespace {

constexpr const char* kJsonType = "application/json; charset=utf-8";

void send_json(httplib::Response& res, const nlohmann::json& body, int status = 200) {
    res.status = status;
    res.set_content(body.dump(), kJsonType);
}

void send_errors(httplib::Response& res, int status, const std::vector<std::string>& errors) {
    send_json(res, nlohmann::json{{"errors", errors}}, status);
}

// Разбирает тело запроса как JSON. При ошибке сам отправляет ответ 400.
std::optional<nlohmann::json> parse_body(const httplib::Request& req, httplib::Response& res) {
    auto body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        send_errors(res, 400, {"Тело запроса должно быть в формате JSON"});
        return std::nullopt;
    }
    return body;
}

}  // namespace

void register_routes(httplib::Server& server, const Catalog& catalog, OrderStore& store) {
    // Разрешаем обращаться к API со страниц, открытых с другого адреса.
    server.set_default_headers({{"Access-Control-Allow-Origin", "*"}});
    server.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        send_json(res, {{"status", "ok"}});
    });

    server.Get("/api/products", [&catalog](const httplib::Request&, httplib::Response& res) {
        send_json(res, catalog.products());
    });

    server.Post("/api/orders", [&catalog, &store](const httplib::Request& req,
                                                  httplib::Response& res) {
        const auto body = parse_body(req, res);
        if (!body) return;

        Order order;
        const auto errors = build_order(*body, catalog, current_date(), order);
        if (!errors.empty()) {
            send_errors(res, 400, errors);
            return;
        }

        const Order saved = store.add(std::move(order));
        std::cout << "Новый заказ №" << saved.id << " на сумму " << saved.total << " руб. ("
                  << saved.customer_name << ", " << saved.address << ")" << std::endl;
        send_json(res, saved, 201);
    });

    server.Get("/api/orders", [&store](const httplib::Request&, httplib::Response& res) {
        send_json(res, store.all());
    });

    server.Get(R"(/api/orders/(\d{1,9}))", [&store](const httplib::Request& req,
                                                     httplib::Response& res) {
        const auto order = store.find(std::stoi(req.matches[1]));
        if (!order) {
            send_errors(res, 404, {"Заказ не найден"});
            return;
        }
        send_json(res, *order);
    });

    server.Patch(R"(/api/orders/(\d{1,9})/status)", [&store](const httplib::Request& req,
                                                              httplib::Response& res) {
        const auto body = parse_body(req, res);
        if (!body) return;

        const auto status_field = body->find("status");
        const auto status = (status_field != body->end() && status_field->is_string())
                                ? status_from_string(status_field->get<std::string>())
                                : std::nullopt;
        if (!status) {
            send_errors(res, 400, {"Неизвестный статус заказа"});
            return;
        }

        const int id = std::stoi(req.matches[1]);
        if (!store.set_status(id, *status)) {
            send_errors(res, 404, {"Заказ не найден"});
            return;
        }
        std::cout << "Заказ №" << id << ": статус изменён на " << to_string(*status) << std::endl;
        send_json(res, *store.find(id));
    });

    server.set_exception_handler(
        [](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
            std::string what = "неизвестная ошибка";
            try {
                std::rethrow_exception(ep);
            } catch (const std::exception& e) {
                what = e.what();
            } catch (...) {
            }
            std::cerr << "Ошибка при обработке " << req.method << " " << req.path << ": " << what
                      << std::endl;
            send_errors(res, 500, {"Внутренняя ошибка сервера"});
        });
}

}  // namespace water
