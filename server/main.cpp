// Сервер системы заказа воды.
// Отдаёт сайт клиента (папка client) и обрабатывает REST API заказов.
//
// Запуск: water_server [--port 8080] [--client путь/к/client] [--data orders.json]

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "httplib.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "api.hpp"
#include "catalog.hpp"
#include "order_store.hpp"

namespace fs = std::filesystem;

namespace {

struct Options {
    int port = 8080;
    std::string client_dir;
    std::string data_file = "orders.json";
};

void print_usage() {
    std::cout << "Использование: water_server [--port 8080] [--client папка_сайта] "
                 "[--data orders.json]\n";
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        }
        if (i + 1 >= argc) {
            std::cerr << "Не указано значение для " << arg << "\n";
            return false;
        }
        const std::string value = argv[++i];
        if (arg == "--port") {
            options.port = std::atoi(value.c_str());
            if (options.port <= 0 || options.port > 65535) {
                std::cerr << "Неверный порт: " << value << "\n";
                return false;
            }
        } else if (arg == "--client") {
            options.client_dir = value;
        } else if (arg == "--data") {
            options.data_file = value;
        } else {
            std::cerr << "Неизвестный параметр: " << arg << "\n";
            return false;
        }
    }
    return true;
}

// Ищет папку с сайтом: рядом с текущей папкой, выше по дереву (если сервер
// запущен из build/ или build/Debug) и рядом с исполняемым файлом.
std::string find_client_dir(const char* argv0) {
    std::error_code ec;
    const fs::path exe_dir = fs::absolute(argv0, ec).parent_path();
    for (const fs::path& candidate :
         {fs::path("client"), fs::path("../client"), fs::path("../../client"), exe_dir / "client"}) {
        if (fs::exists(candidate / "index.html", ec)) {
            return candidate.string();
        }
    }
    return {};
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    // Чтобы русский текст нормально отображался в cmd.
    SetConsoleOutputCP(CP_UTF8);
#endif

    Options options;
    if (!parse_options(argc, argv, options)) {
        print_usage();
        return 1;
    }
    if (options.client_dir.empty()) {
        options.client_dir = find_client_dir(argv[0]);
    }
    if (options.client_dir.empty()) {
        std::cerr << "Не найдена папка client с сайтом. Укажите её через --client\n";
        return 1;
    }

    try {
        const water::Catalog catalog = water::Catalog::default_catalog();
        water::OrderStore store(options.data_file);

        httplib::Server server;
#ifndef _WIN32
        // По умолчанию cpp-httplib включает SO_REUSEPORT, и второй запущенный сервер
        // молча занимает тот же порт. Оставляем только SO_REUSEADDR, чтобы второй
        // запуск получал ошибку «порт занят».
        server.set_socket_options([](socket_t sock) {
            int yes = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        });
#endif
        if (!server.set_mount_point("/", options.client_dir)) {
            std::cerr << "Не удалось открыть папку сайта: " << options.client_dir << "\n";
            return 1;
        }
        water::register_routes(server, catalog, store);

        if (!server.bind_to_port("0.0.0.0", options.port)) {
            std::cerr << "Не удалось запустить сервер на порту " << options.port
                      << " (возможно, порт уже занят)\n";
            return 1;
        }

        std::cout << "Сервер заказа воды запущен\n"
                  << "  Сайт клиента:     http://localhost:" << options.port << "/\n"
                  << "  Панель оператора: http://localhost:" << options.port << "/admin.html\n"
                  << "  Заказы хранятся в файле: " << options.data_file << "\n"
                  << "Для остановки нажмите Ctrl+C" << std::endl;

        if (!server.listen_after_bind()) {
            std::cerr << "Сервер остановлен из-за ошибки\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
