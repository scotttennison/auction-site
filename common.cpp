// common.cpp – shared utilities for all CGI programs
#include "common.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <openssl/sha.h>
#include <openssl/rand.h>

// ── Database ──────────────────────────────────────────────────────────────────

MYSQL* db_connect() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) return nullptr;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME,
                            0, nullptr, 0)) {
        mysql_close(conn);
        return nullptr;
    }
    // Ensure UTF-8 connection
    mysql_set_character_set(conn, "utf8mb4");
    return conn;
}

std::string db_escape(MYSQL* conn, const std::string& s) {
    std::vector<char> buf(s.size() * 2 + 1);
    mysql_real_escape_string(conn, buf.data(), s.c_str(), s.size());
    return std::string(buf.data());
}

// ── Request parsing ───────────────────────────────────────────────────────────

std::string url_decode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int val = 0;
            sscanf(s.c_str() + i + 1, "%2x", &val);
            out += static_cast<char>(val);
            i += 2;
        } else if (s[i] == '+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

std::map<std::string, std::string> parse_query_string(const std::string& qs) {
    std::map<std::string, std::string> params;
    std::istringstream ss(qs);
    std::string token;
    while (std::getline(ss, token, '&')) {
        auto pos = token.find('=');
        if (pos != std::string::npos) {
            params[url_decode(token.substr(0, pos))] =
                url_decode(token.substr(pos + 1));
        }
    }
    return params;
}

std::map<std::string, std::string> parse_post_data() {
    const char* len_str = std::getenv("CONTENT_LENGTH");
    if (!len_str) return {};
    int len = std::atoi(len_str);
    if (len <= 0) return {};
    std::string body(static_cast<size_t>(len), '\0');
    std::cin.read(&body[0], len);
    return parse_query_string(body);
}

std::map<std::string, std::string> parse_cookies() {
    std::map<std::string, std::string> cookies;
    const char* raw = std::getenv("HTTP_COOKIE");
    if (!raw) return cookies;
    std::string rawstr(raw);
    std::istringstream ss(rawstr);
    std::string token;
    while (std::getline(ss, token, ';')) {
        // trim leading whitespace
        size_t start = token.find_first_not_of(' ');
        if (start == std::string::npos) continue;
        token = token.substr(start);
        auto pos = token.find('=');
        if (pos != std::string::npos)
            cookies[token.substr(0, pos)] = token.substr(pos + 1);
    }
    return cookies;
}

// ── Auth helpers ──────────────────────────────────────────────────────────────

std::string sha256_hex(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.size(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    return oss.str();
}

std::string generate_session_id() {
    unsigned char buf[32];
    RAND_bytes(buf, sizeof(buf));
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buf[i]);
    return oss.str();
}

// Returns user_id (>0) on success, 0 on failure; sets user_email on success.
int get_logged_in_user(MYSQL* conn,
                       const std::map<std::string, std::string>& cookies,
                       std::string& user_email) {
    auto it = cookies.find(SESSION_COOKIE);
    if (it == cookies.end() || it->second.empty()) return 0;

    std::string sid = it->second;
    std::string q =
        "SELECT u.id, u.email "
        "FROM sessions s JOIN users u ON s.user_id = u.id "
        "WHERE s.session_id = '" + db_escape(conn, sid) + "' "
        "AND s.expires_at > NOW()";

    if (mysql_query(conn, q.c_str())) return 0;
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) { mysql_free_result(res); return 0; }

    int uid = row[0] ? std::atoi(row[0]) : 0;
    user_email = row[1] ? row[1] : "";
    mysql_free_result(res);
    return uid;
}

// ── Output helpers ────────────────────────────────────────────────────────────

void redirect(const std::string& url) {
    std::cout << "Location: " << url << "\r\n\r\n";
    std::exit(0);
}

std::string html_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:  out += c;
        }
    }
    return out;
}

static const char* CSS = R"CSS(
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,sans-serif;background:#f0f2f5;color:#2d3748}
.topnav{background:#1b263b;padding:12px 24px;display:flex;align-items:center;gap:18px}
.topnav .brand{color:#fff;font-size:19px;font-weight:bold;margin-right:6px}
.topnav a{color:#a0aec0;text-decoration:none;font-size:14px}
.topnav a:hover{color:#fff}
.sp{flex:1}
.usr{color:#718096;font-size:13px}
.wrap{max-width:960px;margin:30px auto;padding:26px;background:#fff;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.09)}
h1{color:#1b263b;margin-bottom:20px;font-size:25px}
h2{color:#2d3748;margin:24px 0 10px;font-size:18px;border-bottom:2px solid #e2e8f0;padding-bottom:6px}
h3{color:#4a5568;margin:14px 0 6px;font-size:15px}
.err{background:#fff5f5;border:1px solid #feb2b2;color:#c53030;padding:11px 15px;border-radius:5px;margin:12px 0}
.ok{background:#f0fff4;border:1px solid #9ae6b4;color:#276749;padding:11px 15px;border-radius:5px;margin:12px 0}
.warn{color:#c05621;font-weight:bold}
label{display:block;margin-top:13px;font-size:13px;font-weight:600;color:#4a5568}
input,textarea,select{width:100%;padding:9px 11px;margin-top:4px;border:1px solid #cbd5e0;border-radius:5px;font-size:14px;color:#2d3748}
textarea{height:90px;resize:vertical}
.btns{margin-top:18px}
.btn{display:inline-block;padding:9px 22px;background:#1b263b;color:#fff;border:none;border-radius:5px;cursor:pointer;font-size:14px;text-decoration:none}
.btn:hover{background:#415a77}
.btn-sm{padding:5px 13px;font-size:12px}
.btn-g{background:#276749}.btn-g:hover{background:#2f855a}
.tabs{display:flex;border-bottom:2px solid #1b263b}
.tabs a{padding:9px 22px;text-decoration:none;color:#718096;border-radius:5px 5px 0 0;font-size:14px}
.tabs a.on,.tabs a:hover{background:#1b263b;color:#fff}
.tabp{border:1px solid #e2e8f0;border-top:none;padding:22px;border-radius:0 5px 5px 5px;margin-bottom:20px}
table{width:100%;border-collapse:collapse;font-size:14px}
th{background:#1b263b;color:#fff;padding:10px 12px;text-align:left}
td{padding:10px 12px;border-bottom:1px solid #e2e8f0;vertical-align:middle}
tr:hover td{background:#f7fafc}
.none{color:#718096;font-style:italic;padding:10px 0;display:block}
.badge{display:inline-block;padding:2px 8px;border-radius:12px;font-size:12px;font-weight:bold}
.badge-green{background:#c6f6d5;color:#276749}
.badge-red{background:#fed7d7;color:#c53030}
.badge-blue{background:#bee3f8;color:#2b6cb0}
)CSS";

void html_header(const std::string& title, bool logged_in,
                 const std::string& email) {
    std::cout << "Content-Type: text/html; charset=UTF-8\r\n\r\n";
    std::cout << "<!DOCTYPE html>\n"
                 "<html lang=\"en\">\n"
                 "<head>\n"
                 "<meta charset=\"UTF-8\">\n"
                 "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
                 "<title>" << html_escape(title) << " – Computech</title>\n"
                 "<style>" << CSS << "</style>\n"
                 "</head>\n<body>\n";

    // Navigation bar
    std::cout << "<nav class=\"topnav\" style=\"justify-content:space-between; align-items:center; padding:8px 24px;\">"
                 "<a href=\"/cgi-bin/auctions\" style=\"display:flex; align-items:center; text-decoration:none;\">"
                 "<img src=\"/computech_logo_transparent.png\" alt=\"Computech\" style=\"height:50px; width:auto; object-fit:contain;\">"
                 "</a>"
                 "<div style=\"display:flex; gap:20px; align-items:center;\">"
                 "<a href=\"/cgi-bin/auctions\">Browse Auctions</a>";
    if (logged_in) {
        std::cout << "<a href=\"/cgi-bin/bid_sell\">Bid / Sell</a>"
                     "<a href=\"/cgi-bin/transactions\">My Transactions</a>"
                     "<span class=\"usr\">" << html_escape(email) << "</span>"
                     "<a href=\"/cgi-bin/auth?action=logout\">Logout</a>";
    } else {
        std::cout << "<a href=\"/cgi-bin/auth\">Login</a>"
                     "<a href=\"/cgi-bin/auth?tab=register\">Register</a>";
    }
    std::cout << "</div></nav>\n<div class=\"wrap\">\n";
}

void html_footer() {
    std::cout << "\n</div>\n</body>\n</html>\n";
}

void print_error(const std::string& msg) {
    std::cout << "<div class=\"err\">" << html_escape(msg) << "</div>\n";
}

void print_success(const std::string& msg) {
    std::cout << "<div class=\"ok\">" << html_escape(msg) << "</div>\n";
}
