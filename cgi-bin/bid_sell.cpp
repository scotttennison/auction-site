// bid_sell.cpp – PA3: Bid on an Item or List an Item for Sale (CGI)
//
// GET  /cgi-bin/bid_sell              → choice landing page
// GET  /cgi-bin/bid_sell?action=bid   → bid form (all active items)
// GET  /cgi-bin/bid_sell?action=bid&item_id=N → bid form pre-selected
// GET  /cgi-bin/bid_sell?action=sell  → sell form
// POST /cgi-bin/bid_sell  action=bid  → submit a bid
// POST /cgi-bin/bid_sell  action=sell → list an item for sale
//
// Rules:
//   • Bid must be ≥ starting bid and strictly > current highest bid.
//   • Users cannot bid on their own items.
//   • Auction duration is exactly 168 hours (7 days) from the seller's start time.

#include "../common.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <mysql/mysql.h>

// ── Bid form ──────────────────────────────────────────────────────────────────

static void render_bid_form(MYSQL* conn, int uid,
                             const std::string& user_email,
                             int preselect_id,
                             const std::string& error_msg,
                             const std::string& success_msg) {
    html_header("Place a Bid", true, user_email);
    std::cout << "<h1>Place a Bid</h1>\n";

    if (!error_msg.empty())   print_error(error_msg);
    if (!success_msg.empty()) print_success(success_msg);

    // Fetch active items not owned by this user
    std::string q =
        "SELECT i.id, i.description, i.end_time,"
        "  COALESCE(MAX(b.bid_amount), i.starting_bid) AS cur_bid "
        "FROM items i LEFT JOIN bids b ON i.id = b.item_id "
        "WHERE i.end_time > NOW() AND i.seller_id <> " + std::to_string(uid) +
        " GROUP BY i.id ORDER BY i.end_time ASC";

    mysql_query(conn, q.c_str());
    MYSQL_RES* res = mysql_store_result(conn);

    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        std::cout << "<span class=\"none\">There are no items available to bid on right now.</span>\n"
                     "<p style=\"margin-top:14px\">"
                     "<a href=\"/cgi-bin/bid_sell\" class=\"btn\">Back</a></p>\n";
        html_footer();
        return;
    }

    std::cout << "<form method=\"post\" action=\"/cgi-bin/bid_sell\">\n"
                 "<input type=\"hidden\" name=\"action\" value=\"bid\">\n"
                 "<label>Select an item</label>\n"
                 "<select name=\"item_id\" required>\n"
                 "<option value=\"\">-- choose an item --</option>\n";

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        int id = row[0] ? std::atoi(row[0]) : 0;
        std::string desc   = row[1] ? row[1] : "";
        std::string end_at = row[2] ? row[2] : "";
        std::string cur    = row[3] ? row[3] : "0.00";
        std::ostringstream label;
        label << "#" << id << " – " << desc
              << "  (current: $" << cur << ", ends: " << end_at << ")";
        std::cout << "<option value=\"" << id << "\""
                  << (id == preselect_id ? " selected" : "") << ">"
                  << html_escape(label.str()) << "</option>\n";
    }
    mysql_free_result(res);

    std::cout << "</select>\n"
                 "<label>Your Maximum Bid ($)</label>\n"
                 "<input type=\"number\" name=\"bid_amount\" min=\"0.01\" step=\"0.01\" required "
                 "placeholder=\"e.g. 25.00\">\n"
                 "<div class=\"btns\">"
                 "<button type=\"submit\" class=\"btn btn-g\">Submit Bid</button>&nbsp;"
                 "<a href=\"/cgi-bin/bid_sell\" class=\"btn\">Cancel</a>"
                 "</div>\n"
                 "</form>\n";

    html_footer();
}

// ── Sell form ─────────────────────────────────────────────────────────────────

static void render_sell_form(const std::string& user_email,
                              const std::string& error_msg,
                              const std::string& success_msg) {
    html_header("Sell an Item", true, user_email);
    std::cout << "<h1>List an Item for Sale</h1>\n";

    if (!error_msg.empty())   print_error(error_msg);
    if (!success_msg.empty()) print_success(success_msg);

    std::cout << "<form method=\"post\" action=\"/cgi-bin/bid_sell\">\n"
                 "<input type=\"hidden\" name=\"action\" value=\"sell\">\n"
                 "<label>Item Description</label>\n"
                 "<textarea name=\"description\" required "
                 "placeholder=\"Describe the item you are selling...\"></textarea>\n"
                 "<label>Starting Bid ($)</label>\n"
                 "<input type=\"number\" name=\"starting_bid\" min=\"0.01\" step=\"0.01\" "
                 "required placeholder=\"e.g. 1.00\">\n"
                 "<label>Auction Start Date &amp; Time</label>\n"
                 "<input type=\"datetime-local\" name=\"start_time\" required>\n"
                 "<p style=\"font-size:12px;color:#718096;margin-top:6px\">"
                 "&#128337; Auction will run for exactly 168 hours (7 days) from the start time.</p>\n"
                 "<div class=\"btns\">"
                 "<button type=\"submit\" class=\"btn btn-g\">List Item</button>&nbsp;"
                 "<a href=\"/cgi-bin/bid_sell\" class=\"btn\">Cancel</a>"
                 "</div>\n"
                 "</form>\n";

    html_footer();
}

// ── Landing page ──────────────────────────────────────────────────────────────

static void render_landing(const std::string& user_email) {
    html_header("Bid or Sell", true, user_email);
    std::cout << "<h1>What would you like to do?</h1>\n"
                 "<div style=\"display:flex;gap:24px;margin-top:20px\">\n"

                 "<div style=\"flex:1;border:1px solid #e2e8f0;border-radius:8px;padding:24px\">\n"
                 "<h2 style=\"border:none;margin-top:0\">&#128176; Place a Bid</h2>\n"
                 "<p style=\"color:#4a5568;font-size:14px;margin:10px 0 18px\">"
                 "Browse active auctions and submit your highest bid on an item.</p>\n"
                 "<a href=\"/cgi-bin/bid_sell?action=bid\" class=\"btn\">Go to Bidding</a>\n"
                 "</div>\n"

                 "<div style=\"flex:1;border:1px solid #e2e8f0;border-radius:8px;padding:24px\">\n"
                 "<h2 style=\"border:none;margin-top:0\">&#127981; Sell an Item</h2>\n"
                 "<p style=\"color:#4a5568;font-size:14px;margin:10px 0 18px\">"
                 "List an item at auction and let buyers compete for it over 7 days.</p>\n"
                 "<a href=\"/cgi-bin/bid_sell?action=sell\" class=\"btn btn-g\">List an Item</a>\n"
                 "</div>\n"

                 "</div>\n";
    html_footer();
}

// ── Process bid POST ──────────────────────────────────────────────────────────

static void process_bid(MYSQL* conn, int uid, const std::string& user_email,
                         const std::map<std::string, std::string>& post) {
    std::string item_id_str  = post.count("item_id")    ? post.at("item_id")    : "";
    std::string bid_amt_str  = post.count("bid_amount") ? post.at("bid_amount") : "";

    if (item_id_str.empty() || bid_amt_str.empty()) {
        render_bid_form(conn, uid, user_email, 0, "Please select an item and enter a bid amount.", "");
        return;
    }

    // Validate that item_id is a positive integer
    int item_id = std::atoi(item_id_str.c_str());
    if (item_id <= 0) {
        render_bid_form(conn, uid, user_email, 0, "Invalid item selected.", "");
        return;
    }

    double bid_amount = std::atof(bid_amt_str.c_str());
    if (bid_amount <= 0.0) {
        render_bid_form(conn, uid, user_email, item_id, "Bid amount must be greater than zero.", "");
        return;
    }

    // Fetch item details
    std::string q = "SELECT seller_id, starting_bid, end_time, "
                    "  COALESCE((SELECT MAX(bid_amount) FROM bids WHERE item_id = i.id), 0) AS high_bid "
                    "FROM items i WHERE i.id = " + std::to_string(item_id);
    if (mysql_query(conn, q.c_str())) {
        render_bid_form(conn, uid, user_email, item_id, "Database error.", "");
        return;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW  row = res ? mysql_fetch_row(res) : nullptr;
    if (!row) {
        if (res) mysql_free_result(res);
        render_bid_form(conn, uid, user_email, item_id, "Item not found.", "");
        return;
    }

    int    seller_id   = row[0] ? std::atoi(row[0]) : 0;
    double starting_bid= row[1] ? std::atof(row[1]) : 0.0;
    std::string end_time = row[2] ? row[2] : "";
    double high_bid    = row[3] ? std::atof(row[3]) : 0.0;
    mysql_free_result(res);

    // Validate: not own item
    if (seller_id == uid) {
        render_bid_form(conn, uid, user_email, item_id, "You cannot bid on your own item.", "");
        return;
    }

    // Validate: auction still active  (re-check via query returned end_time)
    {
        std::string chk = "SELECT 1 FROM items WHERE id = " + std::to_string(item_id)
                          + " AND end_time > NOW()";
        mysql_query(conn, chk.c_str());
        MYSQL_RES* r = mysql_store_result(conn);
        bool active = r && mysql_num_rows(r) > 0;
        if (r) mysql_free_result(r);
        if (!active) {
            render_bid_form(conn, uid, user_email, item_id, "This auction has already closed.", "");
            return;
        }
    }

    // Validate: bid ≥ starting_bid and > current high bid
    double min_bid = (high_bid > 0.0) ? high_bid : starting_bid - 0.005;
    if (bid_amount <= min_bid) {
        std::ostringstream msg;
        msg << std::fixed << std::setprecision(2);
        if (high_bid > 0.0)
            msg << "Your bid must be greater than the current high bid of $" << high_bid << ".";
        else
            msg << "Your bid must be at least the starting bid of $" << starting_bid << ".";
        render_bid_form(conn, uid, user_email, item_id, msg.str(), "");
        return;
    }

    // Insert bid
    std::ostringstream ins;
    ins << std::fixed << std::setprecision(2);
    ins << "INSERT INTO bids (item_id, bidder_id, bid_amount) VALUES ("
        << item_id << ", " << uid << ", " << bid_amount << ")";
    if (mysql_query(conn, ins.str().c_str())) {
        render_bid_form(conn, uid, user_email, item_id, "Failed to record your bid. Please try again.", "");
        return;
    }

    std::ostringstream ok_msg;
    ok_msg << std::fixed << std::setprecision(2);
    ok_msg << "Your bid of $" << bid_amount << " has been placed successfully!";
    render_bid_form(conn, uid, user_email, 0, "", ok_msg.str());
    (void)end_time;
}

// ── Process sell POST ─────────────────────────────────────────────────────────

static void process_sell(MYSQL* conn, int uid, const std::string& user_email,
                          const std::map<std::string, std::string>& post) {
    std::string desc        = post.count("description")  ? post.at("description")  : "";
    std::string start_str   = post.count("start_time")   ? post.at("start_time")   : "";
    std::string bid_str     = post.count("starting_bid") ? post.at("starting_bid") : "";

    if (desc.empty() || start_str.empty() || bid_str.empty()) {
        render_sell_form(user_email, "All fields are required.", "");
        return;
    }

    double starting_bid = std::atof(bid_str.c_str());
    if (starting_bid <= 0.0) {
        render_sell_form(user_email, "Starting bid must be greater than zero.", "");
        return;
    }

    // Convert datetime-local "YYYY-MM-DDTHH:MM" → "YYYY-MM-DD HH:MM:00"
    std::string start_mysql = start_str;
    auto t_pos = start_mysql.find('T');
    if (t_pos != std::string::npos) start_mysql[t_pos] = ' ';
    if (start_mysql.size() == 16) start_mysql += ":00";  // add seconds if missing

    // Compute end_time = start_time + 168 hours
    std::string ins_q;
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "INSERT INTO items (seller_id, description, starting_bid, start_time, end_time)"
               " VALUES ("
            << uid << ", '"
            << db_escape(conn, desc) << "', "
            << starting_bid << ", '"
            << db_escape(conn, start_mysql) << "',"
               " DATE_ADD('" << db_escape(conn, start_mysql) << "', INTERVAL 168 HOUR))";
        ins_q = oss.str();
    }

    if (mysql_query(conn, ins_q.c_str())) {
        render_sell_form(user_email, "Failed to list item. Check your start date/time.", "");
        return;
    }

    render_sell_form(user_email, "", "Your item has been listed successfully!");
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    const char* method = std::getenv("REQUEST_METHOD");
    const char* qs_env = std::getenv("QUERY_STRING");

    auto cookies = parse_cookies();
    auto query   = parse_query_string(qs_env ? qs_env : "");

    MYSQL* conn = db_connect();
    if (!conn) {
        html_header("Error");
        print_error("Database connection failed.");
        html_footer();
        return 1;
    }

    std::string user_email;
    int uid = get_logged_in_user(conn, cookies, user_email);
    if (uid <= 0) {
        mysql_close(conn);
        redirect("/cgi-bin/auth");
    }

    if (method && std::strcmp(method, "POST") == 0) {
        auto post   = parse_post_data();
        std::string action = post.count("action") ? post.at("action") : "";
        if (action == "bid")
            process_bid(conn, uid, user_email, post);
        else if (action == "sell")
            process_sell(conn, uid, user_email, post);
        else {
            html_header("Bid / Sell", true, user_email);
            print_error("Unknown action.");
            html_footer();
        }
    } else {
        std::string action = query.count("action") ? query.at("action") : "";
        if (action == "bid") {
            int preselect = query.count("item_id") ? std::atoi(query.at("item_id").c_str()) : 0;
            render_bid_form(conn, uid, user_email, preselect, "", "");
        } else if (action == "sell") {
            render_sell_form(user_email, "", "");
        } else {
            render_landing(user_email);
        }
    }

    mysql_close(conn);
    return 0;
}
