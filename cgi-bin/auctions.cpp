// auctions.cpp – PA4: Display All Active Auctions (CGI)
//
// GET /cgi-bin/auctions
//
// Shows all auctions with end_time > NOW(), ordered by soonest ending first.
// Closed auctions are never shown.
// Logged-in users see a "Bid" button on every item except their own listings.

#include "../common.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <map>
#include <mysql/mysql.h>

// Format remaining time as a human-readable string.
// diff_secs is (end_time - NOW()) in seconds, retrieved from MySQL.
static std::string format_remaining(long long secs) {
    if (secs <= 0) return "Ending soon";
    long long days  = secs / 86400;
    long long hours = (secs % 86400) / 3600;
    long long mins  = (secs % 3600)  / 60;
    std::ostringstream oss;
    if (days  > 0) oss << days  << "d ";
    if (hours > 0) oss << hours << "h ";
    oss << mins << "m";
    return oss.str();
}

int main() {
    const char* qs_env = std::getenv("QUERY_STRING");
    auto cookies = parse_cookies();

    MYSQL* conn = db_connect();
    if (!conn) {
        html_header("Browse Auctions");
        print_error("Database connection failed.");
        html_footer();
        return 1;
    }

    // Check session (optional – no redirect if not logged in)
    std::string user_email;
    int uid = get_logged_in_user(conn, cookies, user_email);
    bool logged_in = (uid > 0);

    // Fetch all active auctions, soonest-ending first
    std::string q =
        "SELECT i.id, i.description, i.starting_bid,"
        "  COALESCE(MAX(b.bid_amount), i.starting_bid) AS cur_bid,"
        "  COUNT(b.id)                                 AS bid_cnt,"
        "  i.end_time,"
        "  TIMESTAMPDIFF(SECOND, NOW(), i.end_time)    AS secs_left,"
        "  u.email                                     AS seller_email,"
        "  i.seller_id "
        "FROM items i "
        "LEFT JOIN bids b ON b.item_id = i.id "
        "JOIN users u ON u.id = i.seller_id "
        "WHERE i.end_time > NOW() "
        "GROUP BY i.id "
        "ORDER BY i.end_time ASC";

    if (mysql_query(conn, q.c_str())) {
        html_header("Browse Auctions", logged_in, user_email);
        print_error("Failed to load auctions.");
        html_footer();
        mysql_close(conn);
        return 1;
    }

    MYSQL_RES* res = mysql_store_result(conn);

    html_header("Browse Auctions", logged_in, user_email);

    std::cout << "<h1>Active Auctions</h1>\n";

    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        mysql_close(conn);
        std::cout << "<span class=\"none\">No auctions are currently active. "
                     "<a href=\"/cgi-bin/bid_sell?action=sell\">List an item</a> to get started.</span>\n";
        html_footer();
        return 0;
    }

    // Table header
    std::cout << "<table>\n"
                 "<thead><tr>"
                 "<th>#</th>"
                 "<th>Description</th>"
                 "<th>Current Bid</th>"
                 "<th>Bids</th>"
                 "<th>Seller</th>"
                 "<th>Ends At</th>"
                 "<th>Time Left</th>";
    if (logged_in)
        std::cout << "<th>Action</th>";
    std::cout << "</tr></thead>\n<tbody>\n";

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        int    item_id   = row[0] ? std::atoi(row[0]) : 0;
        int    seller_id = row[8] ? std::atoi(row[8]) : 0;
        long long secs   = row[6] ? std::atoll(row[6]) : 0;
        bool   own_item  = logged_in && (seller_id == uid);

        std::cout << "<tr>"
                     "<td>" << item_id << "</td>"
                     "<td>" << html_escape(row[1] ? row[1] : "") << "</td>"
                     "<td><strong>$" << (row[3] ? row[3] : "0.00") << "</strong></td>"
                     "<td>" << (row[4] ? row[4] : "0") << "</td>"
                     "<td>" << html_escape(row[7] ? row[7] : "") << "</td>"
                     "<td>" << (row[5] ? row[5] : "") << "</td>"
                     "<td>" << format_remaining(secs) << "</td>";

        if (logged_in) {
            std::cout << "<td>";
            if (own_item) {
                std::cout << "<span style=\"color:#718096;font-size:13px\">Your listing</span>";
            } else {
                std::cout << "<a href=\"/cgi-bin/bid_sell?action=bid&amp;item_id="
                          << item_id << "\" class=\"btn btn-sm btn-g\">Bid</a>";
            }
            std::cout << "</td>";
        }

        std::cout << "</tr>\n";
    }

    mysql_free_result(res);
    mysql_close(conn);

    std::cout << "</tbody></table>\n";

    if (!logged_in) {
        std::cout << "<p style=\"margin-top:16px;font-size:13px;color:#718096\">"
                     "<a href=\"/cgi-bin/auth\">Log in</a> or "
                     "<a href=\"/cgi-bin/auth?tab=register\">register</a> "
                     "to place bids.</p>\n";
    }

    html_footer();
    return 0;
}
