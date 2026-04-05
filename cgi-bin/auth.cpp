// auth.cpp – PA1: User Registration & Login (CGI)
//
// GET  /cgi-bin/auth              → login form
// GET  /cgi-bin/auth?tab=register → register form
// GET  /cgi-bin/auth?action=logout→ destroy session, redirect to login
// POST /cgi-bin/auth  action=login    → authenticate
// POST /cgi-bin/auth  action=register → create account
//
// Password storage: SHA-256( email + ":" + password )
// Sessions: random 64-hex-char token stored in sessions table

#include "../common.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>

// ── helpers ───────────────────────────────────────────────────────────────────

// Emit Set-Cookie + Location and exit (used after successful login/register).
static void redirect_with_session(const std::string& sid,
                                   const std::string& dest) {
    std::cout << "Set-Cookie: " << SESSION_COOKIE << "=" << sid
              << "; Path=/; Max-Age=" << (SESSION_HOURS * 3600)
              << "; HttpOnly\r\n"
              << "Location: " << dest << "\r\n\r\n";
    std::exit(0);
}

// Clear the session cookie and redirect.
static void redirect_clear_session(const std::string& dest) {
    std::cout << "Set-Cookie: " << SESSION_COOKIE
              << "=; Path=/; Max-Age=0; HttpOnly\r\n"
              << "Location: " << dest << "\r\n\r\n";
    std::exit(0);
}

// Render the auth page (login or register form).
static void render_form(const std::string& tab,
                         const std::string& error_msg,
                         const std::string& prefill_email = "") {
    bool show_register = (tab == "register");

    html_header(show_register ? "Register" : "Login");

    std::cout << "<h1>" << (show_register ? "Create an Account" : "Sign In") << "</h1>\n";

    // Tab bar
    std::cout << "<div class=\"tabs\">"
                 "<a href=\"/cgi-bin/auth\""
              << (!show_register ? " class=\"on\"" : "") << ">Login</a>"
                 "<a href=\"/cgi-bin/auth?tab=register\""
              << (show_register ? " class=\"on\"" : "") << ">Register</a>"
                 "</div>\n";

    std::cout << "<div class=\"tabp\">\n";

    if (!error_msg.empty()) print_error(error_msg);

    if (show_register) {
        // ── Register form ──
        std::cout << "<form method=\"post\" action=\"/cgi-bin/auth\">\n"
                     "<input type=\"hidden\" name=\"action\" value=\"register\">\n"
                     "<label>Email Address</label>\n"
                     "<input type=\"email\" name=\"email\" required "
                     "placeholder=\"you@example.com\" value=\""
                  << html_escape(prefill_email) << "\">\n"
                     "<label>Password <small>(min 8 characters)</small></label>\n"
                     "<input type=\"password\" name=\"password\" required minlength=\"8\">\n"
                     "<label>Confirm Password</label>\n"
                     "<input type=\"password\" name=\"confirm\" required minlength=\"8\">\n"
                     "<div class=\"btns\"><button type=\"submit\" class=\"btn\">Create Account</button></div>\n"
                     "</form>\n";
    } else {
        // ── Login form ──
        std::cout << "<form method=\"post\" action=\"/cgi-bin/auth\">\n"
                     "<input type=\"hidden\" name=\"action\" value=\"login\">\n"
                     "<label>Email Address</label>\n"
                     "<input type=\"email\" name=\"email\" required "
                     "placeholder=\"you@example.com\" value=\""
                  << html_escape(prefill_email) << "\">\n"
                     "<label>Password</label>\n"
                     "<input type=\"password\" name=\"password\" required>\n"
                     "<div class=\"btns\"><button type=\"submit\" class=\"btn\">Sign In</button></div>\n"
                     "</form>\n"
                     "<p style=\"margin-top:14px;font-size:13px;color:#718096\">"
                     "Don&rsquo;t have an account? "
                     "<a href=\"/cgi-bin/auth?tab=register\">Register here</a>.</p>\n";
    }

    std::cout << "</div>\n";
    html_footer();
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    const char* method = std::getenv("REQUEST_METHOD");
    const char* qs_env = std::getenv("QUERY_STRING");

    auto cookies = parse_cookies();
    auto query   = parse_query_string(qs_env ? qs_env : "");

    // ── Logout ────────────────────────────────────────────────────────────────
    if (query["action"] == "logout") {
        MYSQL* conn = db_connect();
        if (conn) {
            auto it = cookies.find(SESSION_COOKIE);
            if (it != cookies.end() && !it->second.empty()) {
                std::string del = "DELETE FROM sessions WHERE session_id='"
                                  + db_escape(conn, it->second) + "'";
                mysql_query(conn, del.c_str());
            }
            mysql_close(conn);
        }
        redirect_clear_session("/cgi-bin/auth");
    }

    // ── If already logged in, go to auctions ─────────────────────────────────
    {
        MYSQL* conn = db_connect();
        if (conn) {
            std::string em;
            int uid = get_logged_in_user(conn, cookies, em);
            mysql_close(conn);
            if (uid > 0) redirect("/cgi-bin/auctions");
        }
    }

    // ── POST – process form submission ────────────────────────────────────────
    if (method && std::strcmp(method, "POST") == 0) {
        auto post   = parse_post_data();
        std::string action  = post["action"];
        std::string email   = post["email"];
        std::string password= post["password"];

        // Trim leading/trailing whitespace from email
        while (!email.empty() && (email.front() == ' ' || email.front() == '\t'))
            email.erase(email.begin());
        while (!email.empty() && (email.back() == ' ' || email.back() == '\t'))
            email.pop_back();

        MYSQL* conn = db_connect();
        if (!conn) {
            render_form(action == "register" ? "register" : "",
                        "Database connection failed. Please try again.");
            return 1;
        }

        // ── Register ──────────────────────────────────────────────────────────
        if (action == "register") {
            std::string confirm = post["confirm"];

            if (email.empty() || password.empty()) {
                mysql_close(conn);
                render_form("register", "Email and password are required.", email);
                return 0;
            }
            if (password.size() < 8) {
                mysql_close(conn);
                render_form("register", "Password must be at least 8 characters.", email);
                return 0;
            }
            if (password != confirm) {
                mysql_close(conn);
                render_form("register", "Passwords do not match.", email);
                return 0;
            }

            // Check for existing account
            std::string chk = "SELECT id FROM users WHERE email='"
                              + db_escape(conn, email) + "'";
            mysql_query(conn, chk.c_str());
            MYSQL_RES* res = mysql_store_result(conn);
            bool exists = res && mysql_num_rows(res) > 0;
            if (res) mysql_free_result(res);

            if (exists) {
                mysql_close(conn);
                render_form("register",
                            "An account with that email already exists.", email);
                return 0;
            }

            // Hash: SHA256( email + ":" + password )
            std::string hash = sha256_hex(email + ":" + password);
            std::string salt = generate_session_id(); // Use random hex as salt
            std::string ins = "INSERT INTO users (email, password_hash, salt) VALUES ('"
                              + db_escape(conn, email) + "','"
                              + db_escape(conn, hash) + "','"
                              + db_escape(conn, salt) + "')";
            if (mysql_query(conn, ins.c_str())) {
                mysql_close(conn);
                render_form("register", "Registration failed. Please try again.", email);
                return 0;
            }
            int new_uid = static_cast<int>(mysql_insert_id(conn));

            // Create session
            std::string sid = generate_session_id();
            std::string ses = "INSERT INTO sessions (session_id, user_id, expires_at) VALUES ('"
                              + db_escape(conn, sid) + "',"
                              + std::to_string(new_uid)
                              + ", DATE_ADD(NOW(), INTERVAL "
                              + std::to_string(SESSION_HOURS) + " HOUR))";
            mysql_query(conn, ses.c_str());
            mysql_close(conn);
            redirect_with_session(sid, "/cgi-bin/auctions");
        }

        // ── Login ─────────────────────────────────────────────────────────────
        if (email.empty() || password.empty()) {
            mysql_close(conn);
            render_form("", "Email and password are required.", email);
            return 0;
        }

        std::string q = "SELECT id FROM users WHERE email='"
                        + db_escape(conn, email)
                        + "' AND password_hash='"
                        + db_escape(conn, sha256_hex(email + ":" + password)) + "'";
        mysql_query(conn, q.c_str());
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row  = res ? mysql_fetch_row(res) : nullptr;

        if (!row) {
            if (res) mysql_free_result(res);
            mysql_close(conn);
            render_form("", "Invalid email or password.", email);
            return 0;
        }

        int uid = std::atoi(row[0]);
        mysql_free_result(res);

        // Create session
        std::string sid = generate_session_id();
        std::string ses = "INSERT INTO sessions (session_id, user_id, expires_at) VALUES ('"
                          + db_escape(conn, sid) + "',"
                          + std::to_string(uid)
                          + ", DATE_ADD(NOW(), INTERVAL "
                          + std::to_string(SESSION_HOURS) + " HOUR))";
        mysql_query(conn, ses.c_str());
        mysql_close(conn);
        redirect_with_session(sid, "/cgi-bin/auctions");
    }

    // ── GET – show form ───────────────────────────────────────────────────────
    render_form(query["tab"], "");
    return 0;
}
