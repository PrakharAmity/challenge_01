#include "httplib.h"
#include "cric.hpp"
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

using json = nlohmann::json;

namespace {

struct Session {
    std::string userId;
    std::string role;
    std::string name;
};

std::mutex g_stateMutex;
std::map<std::string, Session> g_sessions;
std::string g_playerNote = "Play straight early; accelerate after the powerplay.";

long long currentTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string extractToken(const httplib::Request& req) {
    auto auth = req.get_header_value("Authorization");
    std::string prefix = "Bearer ";
    if (auth.rfind(prefix, 0) == 0) {
        return auth.substr(prefix.length());
    }
    return "";
}

bool getSession(const httplib::Request& req, Session& outSession) {
    std::string token = extractToken(req);
    if (token.empty()) return false;
    std::lock_guard<std::mutex> lock(g_stateMutex);
    auto it = g_sessions.find(token);
    if (it != g_sessions.end()) {
        outSession = it->second;
        return true;
    }
    return false;
}

void parseRequestParams(const httplib::Request& req, std::string& user, std::string& pass, std::string& note, std::string& choice) {
    if (req.has_param("user")) user = req.get_param_value("user");
    if (req.has_param("password")) pass = req.get_param_value("password");
    if (req.has_param("note")) note = req.get_param_value("note");
    if (req.has_param("choice")) choice = req.get_param_value("choice");

    if (!req.body.empty() && req.body.front() == '{') {
        try {
            auto bodyJson = json::parse(req.body);
            if (bodyJson.contains("user") && bodyJson["user"].is_string()) user = bodyJson["user"];
            if (bodyJson.contains("password") && bodyJson["password"].is_string()) pass = bodyJson["password"];
            if (bodyJson.contains("note") && bodyJson["note"].is_string()) note = bodyJson["note"];
            if (bodyJson.contains("choice") && bodyJson["choice"].is_string()) choice = bodyJson["choice"];
        } catch (...) {
            // Not valid JSON, ignore
        }
    }
}

void serveStaticFile(const std::string& filePath, const std::string& defaultType, httplib::Response& res) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        res.status = 404;
        res.set_content("File not found", "text/plain");
        return;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    res.set_content(content, defaultType);
}

} // namespace

int main(int argc, char* argv[]) {
    int port = 5000;
    if (const char* envPort = std::getenv("PORT")) {
        port = std::atoi(envPort);
    } else if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    httplib::Server svr;

    cricpulse::MatchState match = cricpulse::sampleMatch();

    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
    });

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // 1. GET /api/state
    svr.Get("/api/state", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(cricpulse::matchJson(match), "application/json");
    });

    // 2. GET /api/analytics
    svr.Get("/api/analytics", [&](const httplib::Request&, httplib::Response& res) {
        auto chain = cricpulse::strongestPartnershipChain(match, 0, 5);
        int strength = cricpulse::chainStrength(match, chain);
        int bestSix = cricpulse::bestSixOverRuns(match);
        double rollingRate = cricpulse::rollingRunRate(match);

        json j;
        j["bestSixOverRuns"] = bestSix;
        j["rollingRunRate"] = rollingRate;
        j["chainStrength"] = strength;
        j["chain"] = chain;

        res.set_content(j.dump(), "application/json");
    });

    // 3. GET /api/reachable/:id
    svr.Get(R"(/api/reachable/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int playerId = std::stoi(req.matches[1]);
        auto reachable = cricpulse::partnershipReachable(match, playerId);

        json j;
        j["player_id"] = playerId;
        j["reachable"] = reachable;

        res.set_content(j.dump(), "application/json");
    });

    // 4. POST /api/login
    svr.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        std::string user, pass, note, choice;
        parseRequestParams(req, user, pass, note, choice);

        Session session;
        bool valid = false;

        if (user == "rohit" && pass == "coverdrive") {
            session = {"player-rohit", "player", "Rohit Sharma"};
            valid = true;
        } else if (user == "fan" && pass == "fanpass") {
            session = {"fan-101", "fan", "Aarav Mehta"};
            valid = true;
        } else if (user == "fan2" && pass == "fanpass") {
            session = {"fan-102", "fan", "Riya Sen"};
            valid = true;
        }

        if (valid) {
            std::string token = "cp-" + session.userId + "-" + std::to_string(currentTimestampMs());
            {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                g_sessions[token] = session;
            }
            json j;
            j["ok"] = true;
            j["token"] = token;
            j["user"] = session.name;
            j["role"] = session.role;
            res.set_content(j.dump(), "application/json");
        } else {
            res.status = 401;
            res.set_content(R"({"ok":false,"error":"Invalid sign-in"})", "application/json");
        }
    });

    // 5. POST /api/poll
    svr.Post("/api/poll", [&](const httplib::Request& req, httplib::Response& res) {
        Session session;
        if (!getSession(req, session)) {
            res.status = 401;
            res.set_content(R"({"ok":false,"error":"Sign in required"})", "application/json");
            return;
        }

        long long nowMs = currentTimestampMs();
        if (!cricpulse::allowFanPoll(session.userId, nowMs)) {
            res.status = 429;
            res.set_content(R"({"ok":false,"error":"Poll limit reached"})", "application/json");
            return;
        }

        res.set_content(R"({"ok":true,"message":"Vote counted"})", "application/json");
    });

    // 6. GET /api/player-note
    svr.Get("/api/player-note", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        json j;
        j["ok"] = true;
        j["note"] = g_playerNote;
        res.set_content(j.dump(), "application/json");
    });

    // 7. POST /api/player-note
    svr.Post("/api/player-note", [&](const httplib::Request& req, httplib::Response& res) {
        Session session;
        if (!getSession(req, session)) {
            res.status = 401;
            res.set_content(R"({"ok":false,"error":"Sign in required"})", "application/json");
            return;
        }

        std::string user, pass, note, choice;
        parseRequestParams(req, user, pass, note, choice);

        bool saved = false;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            saved = cricpulse::savePlayerNote(g_playerNote, note, session.role);
        }

        if (saved) {
            json j;
            j["ok"] = true;
            j["note"] = g_playerNote;
            res.set_content(j.dump(), "application/json");
        } else {
            res.status = 403;
            res.set_content(R"({"ok":false,"error":"Player access required"})", "application/json");
        }
    });

    // Static assets
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        serveStaticFile("web/index.html", "text/html; charset=utf-8", res);
    });

    svr.Get(R"(/([^?]+))", [](const httplib::Request& req, httplib::Response& res) {
        std::string path = req.matches[1];
        if (path.empty() || path == "/") path = "index.html";
        std::string filePath = "web/" + path;

        std::string ctype = "text/plain";
        if (filePath.rfind(".html") != std::string::npos) ctype = "text/html; charset=utf-8";
        else if (filePath.rfind(".css") != std::string::npos) ctype = "text/css; charset=utf-8";
        else if (filePath.rfind(".js") != std::string::npos) ctype = "application/javascript; charset=utf-8";
        else if (filePath.rfind(".json") != std::string::npos) ctype = "application/json; charset=utf-8";
        else if (filePath.rfind(".svg") != std::string::npos) ctype = "image/svg+xml";

        serveStaticFile(filePath, ctype, res);
    });

    std::cout << "CricPulse listening on 0.0.0.0:" << port << std::endl;
    svr.listen("0.0.0.0", port);

    return 0;
}
