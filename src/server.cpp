#include "../vendor/httplib.h"

void server_init() {
    httplib::Server server;

    server.Get("/health", [](const httplib::Request &req, httplib::Response &res) {
        res.set_content("healthy", "text/plain");
    });

    server.listen("0.0.0.0", 8080);
}