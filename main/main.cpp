#define CROW_DISABLE_STATIC_DIR
#include <crow_all.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <thread>
#include <condition_variable>
#include "WebService.h"

using hospital_web::Json;
using hospital_web::WebService;
using hospital_web::ApiError;

struct AccessControl {
    struct context {};
    std::string expected;
    void before_handle(crow::request& req, crow::response& res, context&) {
        if (expected.empty() || req.url == "/api/health") return;
        const auto supplied = req.get_header_value("Authorization");
        unsigned difference = supplied.size() == expected.size() ? 0 : 1;
        for (std::size_t i = 0; i < expected.size(); ++i)
            difference |= static_cast<unsigned char>(expected[i]) ^
                (i < supplied.size() ? static_cast<unsigned char>(supplied[i]) : 0);
        if (difference == 0) return;
        res.code = 401;
        res.set_header("WWW-Authenticate", "Basic realm=\"MediFlow\", charset=\"UTF-8\"");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Cache-Control", "no-store");
        res.body = R"({"ok":false,"error":"Can dang nhap de truy cap MediFlow"})";
        res.end();
    }
    void after_handle(crow::request&, crow::response&, context&) {}
};
using HospitalApp = crow::App<AccessControl>;

namespace {
crow::response asset(const char* file, const char* type) {
    std::ifstream stream(std::string("main/web/") + file, std::ios::binary);
    if (!stream) return crow::response(404, "Khong tim thay giao dien web. Kiem tra thu muc main/web.");
    std::ostringstream content;
    content << stream.rdbuf();
    crow::response response(content.str());
    response.set_header("Content-Type", type);
    response.set_header("Cache-Control", "no-cache");
    response.set_header("X-Content-Type-Options", "nosniff");
    return response;
}
crow::response jsonResponse(int status, const Json& body) {
    crow::response response(status, body.dump());
    response.set_header("Content-Type", "application/json; charset=utf-8");
    response.set_header("Cache-Control", "no-store");
    return response;
}
Json body(const crow::request& request) {
    if (request.body.size() > 1024 * 1024) throw ApiError(413, "Body qua lon");
    auto value = Json::parse(request.body, nullptr, false);
    if (value.is_discarded() || !value.is_object()) throw ApiError(400, "Body phai la JSON object hop le");
    return value;
}
template<class Action>
crow::response handle(WebService& service, Action action, int status = 200) {
    std::lock_guard<std::mutex> lock(service.mutex);
    try { return jsonResponse(status, {{"ok",true},{"data",action()}}); }
    catch (const ApiError& error) { return jsonResponse(error.status, {{"ok",false},{"error",error.what()}}); }
    catch (const Json::exception&) { return jsonResponse(400, {{"ok",false},{"error","Du lieu JSON khong hop le"}}); }
    catch (const std::exception& error) {
        std::cerr << "API error: " << error.what() << '\n';
        return jsonResponse(500, {{"ok",false},{"error","Loi xu ly tren server"}});
    }
}
}

// Đăng ký routes tách khỏi main để có thể thêm frontend hoặc middleware sau này.
void registerRoutes(HospitalApp& app, WebService& service) {
    CROW_ROUTE(app, "/")([] {
        return asset("index.html", "text/html; charset=utf-8");
    });
    // Danh sách file cố định; không nhận đường dẫn file từ request.
    CROW_ROUTE(app, "/assets/app.css")([] { return asset("app.css", "text/css; charset=utf-8"); });
    CROW_ROUTE(app, "/assets/app.js")([] { return asset("app.js", "text/javascript; charset=utf-8"); });
    CROW_ROUTE(app, "/api/health")([] {
        return jsonResponse(200, {{"ok",true},{"data",{{"status","running"}}}});
    });
    CROW_ROUTE(app, "/api/departments")([&] {
        return handle(service, [&] { return service.departments(); });
    });
    CROW_ROUTE(app, "/api/patients").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
    ([&](const crow::request& req) {
        return handle(service, [&] {
            if (req.method == crow::HTTPMethod::Post) return service.savePatient(body(req));
            const auto* search = req.url_params.get("q");
            return service.listPatients(search ? search : "");
        }, req.method == crow::HTTPMethod::Post ? 201 : 200);
    });
    CROW_ROUTE(app, "/api/patients/<int>").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Patch, crow::HTTPMethod::Delete)
    ([&](const crow::request& req, int id) {
        return handle(service, [&] {
            if (id <= 0) throw ApiError(400, "ID phai lon hon 0");
            if (req.method == crow::HTTPMethod::Patch) return service.savePatient(body(req), id);
            if (req.method == crow::HTTPMethod::Delete) return service.deletePatient(id);
            return service.getPatient(id);
        });
    });
    CROW_ROUTE(app, "/api/checkins").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
    ([&](const crow::request& req) {
        return handle(service, [&] {
            return req.method == crow::HTTPMethod::Post ? service.checkIn(body(req)) : service.listCheckIns();
        }, req.method == crow::HTTPMethod::Post ? 201 : 200);
    });
    CROW_ROUTE(app, "/api/queue")([&] { return handle(service, [&] { return service.queue(); }); });
    CROW_ROUTE(app, "/api/queue/sync").methods(crow::HTTPMethod::Post)([&] {
        return handle(service, [&] { return service.syncExams(); });
    });
    CROW_ROUTE(app, "/api/checkins/<int>/priority").methods(crow::HTTPMethod::Patch)
    ([&](const crow::request& req, int id) {
        return handle(service, [&] { return service.changePriority(id, body(req)); });
    });
    CROW_ROUTE(app, "/api/doctors")([&] { return handle(service, [&] { return service.listDoctors(); }); });
    CROW_ROUTE(app, "/api/doctors/<string>/status").methods(crow::HTTPMethod::Patch)
    ([&](const crow::request& req, std::string id) {
        return handle(service, [&] { return service.updateDoctor(id, body(req)); });
    });
    CROW_ROUTE(app, "/api/assignments").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
    ([&](const crow::request& req) {
        return handle(service, [&] {
            return req.method == crow::HTTPMethod::Post ? service.schedule(body(req)) : service.listAssignments();
        });
    });
    CROW_ROUTE(app, "/api/exams")([&](const crow::request& req) {
        return handle(service, [&] {
            const auto* active = req.url_params.get("active");
            if (active && std::string(active) != "true" && std::string(active) != "false")
                throw ApiError(400, "active phai la true hoac false");
            return service.listExams(!active || std::string(active) == "true");
        });
    });
    CROW_ROUTE(app, "/api/exams/sync").methods(crow::HTTPMethod::Post)([&] {
        return handle(service, [&] { return service.syncExams(); });
    });
    CROW_ROUTE(app, "/api/exams/<int>/diagnosis").methods(crow::HTTPMethod::Patch)
    ([&](const crow::request& req, int id) { return handle(service, [&] { return service.diagnosis(id, body(req)); }); });
    CROW_ROUTE(app, "/api/exams/<int>/finish").methods(crow::HTTPMethod::Post)
    ([&](int id) { return handle(service, [&] { return service.finishExam(id); }); });
}

int main(int argc, char** argv) {
    try {
        if (argc > 3) throw std::invalid_argument("Cach dung: hospital_web.exe [repo-root] [port]");
        if (argc >= 2) std::filesystem::current_path(std::filesystem::absolute(argv[1]));
        if (!std::filesystem::exists("SAP_XEP_BAC_SI/src"))
            throw std::runtime_error("Hay chay tu root repo hoac truyen repo-root");
        unsigned port = 18080;
        const char* configuredPort = std::getenv("PORT");
        if (argc == 3 || configuredPort) {
            std::string value = argc == 3 ? argv[2] : configuredPort;
            if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
                throw std::invalid_argument("Port khong hop le");
            const auto parsedPort = std::stoull(value);
            if (!parsedPort || parsedPort > 65535) throw std::invalid_argument("Port phai tu 1 den 65535");
            port = static_cast<unsigned>(parsedPort);
        }
        const char* configuredBind = std::getenv("HOSPITAL_BIND");
        const std::string bindAddress = configuredBind ? configuredBind : "127.0.0.1";
        const char* password = std::getenv("HOSPITAL_PASSWORD");
        const std::string accessPassword = password ? password : "";
        if (!accessPassword.empty() && accessPassword.size() < 12)
            throw std::invalid_argument("HOSPITAL_PASSWORD can it nhat 12 ky tu");
        if (bindAddress != "127.0.0.1" && bindAddress != "::1" && accessPassword.empty())
            throw std::invalid_argument("Can dat HOSPITAL_PASSWORD truoc khi cho phep truy cap ngoai may");
        WebService service;
        HospitalApp app;
        if (!accessPassword.empty()) {
            const std::string credentials = "admin:" + accessPassword;
            app.get_middleware<AccessControl>().expected = "Basic " + crow::utility::base64encode(credentials, credentials.size());
        }
        registerRoutes(app, service);
        service.syncExams();
        std::mutex timerMutex;
        std::condition_variable timerWake;
        bool stopped = false;
        std::thread synchronizer([&] {
            std::unique_lock<std::mutex> timerLock(timerMutex);
            while (!timerWake.wait_for(timerLock, std::chrono::seconds(5), [&] { return stopped; })) {
                timerLock.unlock();
                try {
                    std::lock_guard<std::mutex> lock(service.mutex);
                    service.syncExams();
                } catch (const std::exception& error) {
                    std::cerr << "Automatic sync: " << error.what() << '\n';
                }
                timerLock.lock();
            }
        });
        const auto stopSync = [&] {
            { std::lock_guard<std::mutex> lock(timerMutex); stopped = true; }
            timerWake.notify_one();
            synchronizer.join();
        };
        std::cout << "Hospital API: http://" << bindAddress << ':' << port << '\n';
        try { app.bindaddr(bindAddress).port(static_cast<uint16_t>(port)).concurrency(2).run(); }
        catch (...) { stopSync(); throw; }
        stopSync();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
