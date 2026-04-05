#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <map>
#include <mysql/mysql.h>

// ── Database credentials ──────────────────────────────────────────────────────
#define DB_HOST "localhost"
#define DB_USER "auction_user"
#define DB_PASS "auction_pass"
#define DB_NAME "auctiondb"

// ── Session settings ──────────────────────────────────────────────────────────
#define SESSION_COOKIE "auction_session"
#define SESSION_HOURS  24

// ── Database ──────────────────────────────────────────────────────────────────
MYSQL* db_connect();

// ── Request parsing ───────────────────────────────────────────────────────────
std::string url_decode(const std::string& s);
std::map<std::string, std::string> parse_post_data();
std::map<std::string, std::string> parse_query_string(const std::string& qs);
std::map<std::string, std::string> parse_cookies();

// ── Auth helpers ──────────────────────────────────────────────────────────────
std::string sha256_hex(const std::string& input);
std::string generate_session_id();
int  get_logged_in_user(MYSQL* conn,
                        const std::map<std::string, std::string>& cookies,
                        std::string& user_email);

// ── Output helpers ────────────────────────────────────────────────────────────
void        redirect(const std::string& url);
void        html_header(const std::string& title,
                        bool logged_in       = false,
                        const std::string&   email = "");
void        html_footer();
std::string html_escape(const std::string& s);
void        print_error(const std::string& msg);
void        print_success(const std::string& msg);

// ── DB string escaping ────────────────────────────────────────────────────────
std::string db_escape(MYSQL* conn, const std::string& s);

#endif // COMMON_H
