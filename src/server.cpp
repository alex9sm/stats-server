#include <string>

#include "../vendor/json.hpp"
#include "../vendor/httplib.h"

#include "collector.h"
#include "server.h"

static long long parse_ll(const httplib::Request &req, const char *key) {
    if (!req.has_param(key)) return 0;
    try {
        return std::stoll(req.get_param_value(key));
    } catch (...) {
        return 0;
    }
}

void server_run(RingBuffer &rb) {
    httplib::Server server;

    server.Get("/health", [](const httplib::Request &req, httplib::Response &res) {
        res.set_content("healthy", "text/plain");
    });

    server.Get("/query_range", [&rb](const httplib::Request &req, httplib::Response &res) {
        long long from = parse_ll(req, "from");
        long long to = parse_ll(req, "to");
        long long step = parse_ll(req, "step");

        QueryResult qr = ring_get(rb, from, to, step);

        nlohmann::json out = nlohmann::json::array();
        for (size_t i = 0; i < qr.time_ms.size(); ++i) {
            nlohmann::json row;
            row["time"] = qr.time_ms[i];
            const auto &r = qr.rows[i];
            for (int m = 0; m < metrics_count - 1; ++m) {
                row[metrics[m]] = r[m];
            }
            out.push_back(std::move(row));
        }
        res.set_content(out.dump(), "json");
    }); 

    server.listen("0.0.0.0", 8080);
}