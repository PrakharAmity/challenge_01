#include "analytics.hpp"
#include "auth.hpp"
#include "match_data.hpp"
#include "partnership_chain.hpp"
#include "partnership_routes.hpp"
#include "rate_limiter.hpp"
#include "run_rate.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <csignal>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>

using json = nlohmann::json;

namespace {

std::string g_bootId;
std::string g_startedAt;
const std::string g_authSecret = "cricpulse-auth-token-secret-2026";

int64_t currentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string formatCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min << ":"
        << std::setw(2) << tm.tm_sec;
    return oss.str();
}

std::string readFileContent(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

std::string computeFrontendRevision() {
    std::string html = readFileContent("web/index.html");
    std::string css = readFileContent("web/style.css");
    std::string js = readFileContent("web/app.js");
    return cricpulse::computeSha256Hex(html + css + js).substr(0, 16);
}

void setCommonHeaders(httplib::Response& res, const std::string& contentType) {
    res.set_header("Content-Type", contentType);
    res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    res.set_header("Pragma", "no-cache");
    res.set_header("Expires", "0");
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

httplib::Server* g_serverPtr = nullptr;

void handleSignal(int) {
    if (g_serverPtr) {
        g_serverPtr->stop();
    }
}

} // namespace

int main(int argc, char* argv[]) {
    // Generate boot id and startup timestamp
    int64_t startMs = currentTimeMs();
    std::mt19937_64 rng(static_cast<uint64_t>(startMs));
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream bootOss;
    bootOss << "boot-" << std::hex << dist(rng) << "-" << dist(rng);
    g_bootId = bootOss.str();
    g_startedAt = formatCurrentTime();

    int port = 8080;
    const char* envPort = std::getenv("PORT");
    if (envPort && *envPort) {
        try {
            port = std::stoi(envPort);
        } catch (...) {}
    }
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {}
    }

    httplib::Server svr;
    g_serverPtr = &svr;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // Initialize application state
    auto match = cricpulse::getSampleMatchFixture();
    std::mutex stateMutex;
    cricpulse::PollRateLimiter pollLimiter(10000); // 10 second cooldown

    // Seed default rate-limiter scenario: initial vote 8s ago, rejected attempt 3s ago
    pollLimiter.seedVote("fan-seeded-user", startMs - 8000);
    pollLimiter.recordVoteAttempt("fan-seeded-user", startMs - 3000);

    // Seed default player session: issued 45s ago with 30s TTL (expired 15s ago)
    cricpulse::PlayerSession seededSession = cricpulse::createPlayerSession(
        "player-18", "Virat Kohli", "captain", startMs - 45000, 30000, g_authSecret
    );
    std::string seededToken = cricpulse::serializeSessionToken(seededSession);

    // CORS preflight
    svr.Options(R"((?:.*))", [](const httplib::Request&, httplib::Response& res) {
        setCommonHeaders(res, "text/plain");
        res.status = 204;
    });

    // Health endpoint
    svr.Get(R"((?:.*)/api/health)", [](const httplib::Request&, httplib::Response& res) {
        json j;
        j["boot_id"] = g_bootId;
        j["started_at"] = g_startedAt;
        j["ui_revision"] = computeFrontendRevision();
        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Match overview endpoint
    svr.Get(R"((?:.*)/api/match)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        int64_t now = currentTimeMs();

        // Compute live analytics using core domain functions
        double currentRunRate = cricpulse::calculateCurrentRunRate(match.totalRuns, match.legalBalls);
        auto bestStretch = cricpulse::calculateBestSixOverStretch(match.overRuns);

        // Optimal route between Rohit Sharma (0) and Ravindra Jadeja (4)
        auto optimalRoute = cricpulse::findOptimalPartnershipRoute(match.partnershipGraph, 0, 4);

        // Longest partnership chain starting from Rohit Sharma (0)
        auto longestChain = cricpulse::findLongestPartnershipChain(match.partnershipGraph, 0);

        // Evaluate session status for seeded or header token
        std::string tokenToEval = seededToken;
        std::string authHdr = req.get_header_value("Authorization");
        if (authHdr.rfind("Bearer ", 0) == 0) {
            tokenToEval = authHdr.substr(7);
        } else if (req.has_param("token")) {
            tokenToEval = req.get_param_value("token");
        }

        auto activeSession = cricpulse::deserializeSessionToken(tokenToEval);
        bool sessionExpired = cricpulse::isSessionExpired(activeSession, now);
        bool sessionValid = cricpulse::isSessionValid(activeSession, now, g_authSecret);

        // Fan poll status
        std::string fanId = req.has_param("fanId") ? req.get_param_value("fanId") : "fan-seeded-user";
        auto pollStatus = pollLimiter.checkVoteStatus(fanId, now);

        json j;
        j["matchId"] = match.matchId;
        j["matchTitle"] = match.matchTitle;
        j["matchStatus"] = match.matchStatus;
        j["battingTeam"] = match.battingTeam;
        j["bowlingTeam"] = match.bowlingTeam;
        j["totalRuns"] = match.totalRuns;
        j["wickets"] = match.wickets;
        j["legalBalls"] = match.legalBalls;
        j["oversFormatted"] = std::to_string(match.legalBalls / 6) + "." + std::to_string(match.legalBalls % 6);
        j["currentRunRate"] = currentRunRate;

        // Sliding window stretch result
        j["bestStretch"] = {
            {"startOver", bestStretch.startOver},
            {"endOver", bestStretch.endOver},
            {"totalRuns", bestStretch.totalRuns}
        };

        // Over runs breakdown
        j["overRuns"] = match.overRuns;

        // Ball by ball feed
        json feedArr = json::array();
        for (const auto& b : match.ballFeed) {
            feedArr.push_back({
                {"over", b.over},
                {"ball", b.ball},
                {"batter", b.batter},
                {"bowler", b.bowler},
                {"runs", b.runs},
                {"isWicket", b.isWicket},
                {"commentary", b.commentary}
            });
        }
        j["ballFeed"] = feedArr;

        // Players list
        json playersArr = json::array();
        for (const auto& p : match.players) {
            playersArr.push_back({
                {"id", p.id},
                {"name", p.name},
                {"role", p.role},
                {"team", p.team}
            });
        }
        j["players"] = playersArr;

        // Partnership graph
        json linksArr = json::array();
        for (size_t u = 0; u < match.partnershipGraph.size(); ++u) {
            for (const auto& edge : match.partnershipGraph[u]) {
                if (static_cast<int>(u) < edge.target) {
                    linksArr.push_back({
                        {"source", u},
                        {"target", edge.target},
                        {"weight", edge.weight}
                    });
                }
            }
        }
        j["graphLinks"] = linksArr;

        // Optimal route
        j["optimalRoute"] = {
            {"startPlayer", 0},
            {"endPlayer", 4},
            {"playerPath", optimalRoute.playerPath},
            {"totalCost", optimalRoute.totalCost}
        };

        // Longest chain
        j["longestChain"] = {
            {"startPlayer", 0},
            {"playerPath", longestChain},
            {"length", longestChain.size()}
        };

        // Poll options
        json pollArr = json::array();
        for (const auto& opt : match.pollOptions) {
            pollArr.push_back({
                {"id", opt.id},
                {"name", opt.name},
                {"votes", opt.votes}
            });
        }
        j["pollOptions"] = pollArr;
        j["pollStatus"] = {
            {"fanId", fanId},
            {"allowed", pollStatus.allowed},
            {"remainingCooldownMs", pollStatus.remainingCooldownMs},
            {"nextAvailableMs", pollStatus.nextAvailableMs}
        };

        // Session status
        j["auth"] = {
            {"token", tokenToEval},
            {"playerId", activeSession.playerId},
            {"playerName", activeSession.playerName},
            {"role", activeSession.role},
            {"issuedAtMs", activeSession.issuedAtMs},
            {"expiresAtMs", activeSession.expiresAtMs},
            {"isExpired", sessionExpired},
            {"isValid", sessionValid}
        };

        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Best six over stretch endpoint
    svr.Get(R"((?:.*)/api/analytics/best-stretch)", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        auto result = cricpulse::calculateBestSixOverStretch(match.overRuns);
        json j = {
            {"startOver", result.startOver},
            {"endOver", result.endOver},
            {"totalRuns", result.totalRuns}
        };
        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Current run rate calculation endpoint
    svr.Get(R"((?:.*)/api/analytics/run-rate)", [&](const httplib::Request& req, httplib::Response& res) {
        int runs = match.totalRuns;
        int balls = match.legalBalls;
        if (req.has_param("runs")) {
            try { runs = std::stoi(req.get_param_value("runs")); } catch (...) {}
        }
        if (req.has_param("balls")) {
            try { balls = std::stoi(req.get_param_value("balls")); } catch (...) {}
        }
        double rate = cricpulse::calculateCurrentRunRate(runs, balls);
        json j = {
            {"totalRuns", runs},
            {"legalBalls", balls},
            {"runRate", rate}
        };
        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Player login endpoint
    svr.Post(R"((?:.*)/api/auth/login)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string playerId = "player-18";
        std::string playerName = "Virat Kohli";
        std::string role = "captain";

        try {
            auto body = json::parse(req.body);
            if (body.contains("playerId")) playerId = body["playerId"];
            if (body.contains("playerName")) playerName = body["playerName"];
            if (body.contains("role")) role = body["role"];
        } catch (...) {}

        int64_t now = currentTimeMs();
        int64_t durationMs = 30000; // 30 second session TTL
        auto session = cricpulse::createPlayerSession(playerId, playerName, role, now, durationMs, g_authSecret);
        std::string token = cricpulse::serializeSessionToken(session);

        json j = {
            {"token", token},
            {"playerId", session.playerId},
            {"playerName", session.playerName},
            {"role", session.role},
            {"issuedAtMs", session.issuedAtMs},
            {"expiresAtMs", session.expiresAtMs},
            {"isExpired", cricpulse::isSessionExpired(session, now)},
            {"isValid", cricpulse::isSessionValid(session, now, g_authSecret)}
        };

        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Session validation endpoint
    svr.Get(R"((?:.*)/api/auth/status)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string token = seededToken;
        std::string authHdr = req.get_header_value("Authorization");
        if (authHdr.rfind("Bearer ", 0) == 0) {
            token = authHdr.substr(7);
        } else if (req.has_param("token")) {
            token = req.get_param_value("token");
        }

        int64_t now = currentTimeMs();
        auto session = cricpulse::deserializeSessionToken(token);
        bool isExpired = cricpulse::isSessionExpired(session, now);
        bool isValid = cricpulse::isSessionValid(session, now, g_authSecret);

        json j = {
            {"token", token},
            {"playerId", session.playerId},
            {"playerName", session.playerName},
            {"role", session.role},
            {"issuedAtMs", session.issuedAtMs},
            {"expiresAtMs", session.expiresAtMs},
            {"isExpired", isExpired},
            {"isValid", isValid}
        };

        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Fan poll voting endpoint
    svr.Post(R"((?:.*)/api/poll/vote)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        std::string fanId = "fan-live-user";
        std::string optionId = "opt-1";

        try {
            auto body = json::parse(req.body);
            if (body.contains("fanId")) fanId = body["fanId"];
            if (body.contains("optionId")) optionId = body["optionId"];
        } catch (...) {}

        int64_t now = currentTimeMs();
        auto status = pollLimiter.recordVoteAttempt(fanId, now);

        if (status.allowed) {
            for (auto& opt : match.pollOptions) {
                if (opt.id == optionId) {
                    opt.votes++;
                    break;
                }
            }
            json j = {
                {"success", true},
                {"fanId", fanId},
                {"remainingCooldownMs", status.remainingCooldownMs},
                {"nextAvailableMs", status.nextAvailableMs}
            };
            setCommonHeaders(res, "application/json");
            res.set_content(j.dump(), "application/json");
        } else {
            json j = {
                {"success", false},
                {"fanId", fanId},
                {"remainingCooldownMs", status.remainingCooldownMs},
                {"nextAvailableMs", status.nextAvailableMs},
                {"error", "Cooldown period active. Please wait before submitting another vote."}
            };
            setCommonHeaders(res, "application/json");
            res.status = 429;
            res.set_content(j.dump(), "application/json");
        }
    });

    // Fan poll status query endpoint
    svr.Get(R"((?:.*)/api/poll/status)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string fanId = req.has_param("fanId") ? req.get_param_value("fanId") : "fan-seeded-user";
        int64_t now = currentTimeMs();
        auto status = pollLimiter.checkVoteStatus(fanId, now);

        json j = {
            {"fanId", fanId},
            {"allowed", status.allowed},
            {"remainingCooldownMs", status.remainingCooldownMs},
            {"nextAvailableMs", status.nextAvailableMs}
        };

        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Partnership route calculation endpoint
    svr.Get(R"((?:.*)/api/partnership/route)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        int start = 0;
        int end = 4;
        if (req.has_param("start")) {
            try { start = std::stoi(req.get_param_value("start")); } catch (...) {}
        }
        if (req.has_param("end")) {
            try { end = std::stoi(req.get_param_value("end")); } catch (...) {}
        }

        auto route = cricpulse::findOptimalPartnershipRoute(match.partnershipGraph, start, end);
        json j = {
            {"startPlayer", start},
            {"endPlayer", end},
            {"playerPath", route.playerPath},
            {"totalCost", route.totalCost}
        };

        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Recursive partnership chain calculation endpoint
    svr.Get(R"((?:.*)/api/partnership/chain)", [&](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(stateMutex);
        int start = 0;
        if (req.has_param("start")) {
            try { start = std::stoi(req.get_param_value("start")); } catch (...) {}
        }

        auto chain = cricpulse::findLongestPartnershipChain(match.partnershipGraph, start);
        json j = {
            {"startPlayer", start},
            {"playerPath", chain},
            {"length", chain.size()}
        };

        setCommonHeaders(res, "application/json");
        res.set_content(j.dump(), "application/json");
    });

    // Static assets
    svr.Get(R"((?:.*)/style\.css)", [](const httplib::Request&, httplib::Response& res) {
        std::string content = readFileContent("web/style.css");
        setCommonHeaders(res, "text/css");
        res.set_content(content, "text/css");
    });

    svr.Get(R"((?:.*)/app\.js)", [](const httplib::Request&, httplib::Response& res) {
        std::string content = readFileContent("web/app.js");
        setCommonHeaders(res, "application/javascript");
        res.set_content(content, "application/javascript");
    });

    // Default static HTML route
    svr.Get(R"((?:.*))", [](const httplib::Request& req, httplib::Response& res) {
        // If request path points to an existing file in web
        std::string path = req.path;
        if (path.rfind("/style.css") != std::string::npos) {
            std::string content = readFileContent("web/style.css");
            setCommonHeaders(res, "text/css");
            res.set_content(content, "text/css");
            return;
        }
        if (path.rfind("/app.js") != std::string::npos) {
            std::string content = readFileContent("web/app.js");
            setCommonHeaders(res, "application/javascript");
            res.set_content(content, "application/javascript");
            return;
        }
        std::string content = readFileContent("web/index.html");
        setCommonHeaders(res, "text/html; charset=utf-8");
        res.set_content(content, "text/html; charset=utf-8");
    });

    std::cerr << "[CricPulse] Server listening on 0.0.0.0:" << port << " (boot_id: " << g_bootId << ")" << std::endl;
    svr.listen("0.0.0.0", port);

    return 0;
}
