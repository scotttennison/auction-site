// transactions.cpp – PA2: Display User Transactions (CGI)
//
// GET /cgi-bin/transactions
// Requires a valid session; redirects to /cgi-bin/auth otherwise.
//
// Displays four sections:
//   1. Selling       – active & closed auctions the user listed
//   2. Purchases     – closed auctions the user won (highest bidder)
//   3. Current Bids  – active auctions the user is bidding on
//   4. Didn't Win    – closed auctions user bid on but lost

#include "../common.h"
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <mysql/mysql.h>

// ── section helpers ───────────────────────────────────────────────────────────

// Print a table row for an item.  Columns are caller-defined.
static void open_table(const std::initializer_list<const char*>& headers) {
    std::cout << "<table><thead><tr>";
    for (auto h : headers) std::cout << "<th>" << h << "</th>";
    std::cout << "</tr></thead><tbody>\n";
}
static void close_table() { std::cout << "</tbody></table>\n"; }

// ── Selling ───────────────────────────────────────────────────────────────────

static void section_selling(MYSQL* conn, int uid) {
    std::cout << "<h2>Selling</h2>\n";

    // ── Active listings ──
    std::cout << "<h3>Active Listings</h3>\n";
    std::string q =
        "SELECT i.id, i.description, i.starting_bid, i.start_time, i.end_time,"
        "  COALESCE(MAX(b.bid_amount), i.starting_bid) AS cur_bid,"
        "  COUNT(b.id) AS bid_cnt "
        "FROM items i LEFT JOIN bids b ON i.id = b.item_id "
        "WHERE i.seller_id = " + std::to_string(uid) + " AND i.end_time > NOW() "
        "GROUP BY i.id ORDER BY i.end_time ASC";

    if (mysql_query(conn, q.c_str())) { std::cout << "<p class=\"err\">Query error.</p>"; return; }
    MYSQL_RES* res = mysql_store_result(conn);

    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        std::cout << "<span class=\"none\">No active listings.</span>\n";
    } else {
        open_table({"#", "Description", "Starting Bid", "Current Bid",
                    "Bids", "Ends At"});
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            std::cout << "<tr>"
                         "<td>" << (row[0] ? row[0] : "") << "</td>"
                         "<td>" << html_escape(row[1] ? row[1] : "") << "</td>"
                         "<td>$" << (row[2] ? row[2] : "0.00") << "</td>"
                         "<td>$" << (row[5] ? row[5] : "0.00") << "</td>"
                         "<td>" << (row[6] ? row[6] : "0") << "</td>"
                         "<td>" << (row[4] ? row[4] : "") << "</td>"
                         "</tr>\n";
        }
        close_table();
        mysql_free_result(res);
    }

    // ── Closed / sold listings ──
    std::cout << "<h3>Closed Listings</h3>\n";
    std::string q2 =
        "SELECT i.id, i.description, i.starting_bid, i.end_time,"
        "  COALESCE(MAX(b.bid_amount), 0) AS final_bid,"
        "  COUNT(b.id) AS bid_cnt "
        "FROM items i LEFT JOIN bids b ON i.id = b.item_id "
        "WHERE i.seller_id = " + std::to_string(uid) + " AND i.end_time <= NOW() "
        "GROUP BY i.id ORDER BY i.end_time DESC";

    if (mysql_query(conn, q2.c_str())) { std::cout << "<p class=\"err\">Query error.</p>"; return; }
    MYSQL_RES* res2 = mysql_store_result(conn);

    if (!res2 || mysql_num_rows(res2) == 0) {
        if (res2) mysql_free_result(res2);
        std::cout << "<span class=\"none\">No closed listings.</span>\n";
    } else {
        open_table({"#", "Description", "Starting Bid", "Final / Winning Bid",
                    "Bids", "Closed At", "Status"});
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res2))) {
            double final_bid = row[4] ? std::atof(row[4]) : 0.0;
            int bid_cnt = row[5] ? std::atoi(row[5]) : 0;
            const char* status = bid_cnt > 0 ? "Sold" : "Unsold";
            const char* badge  = bid_cnt > 0
                ? "<span class=\"badge badge-green\">Sold</span>"
                : "<span class=\"badge badge-red\">Unsold</span>";
            std::cout << "<tr>"
                         "<td>" << (row[0] ? row[0] : "") << "</td>"
                         "<td>" << html_escape(row[1] ? row[1] : "") << "</td>"
                         "<td>$" << (row[2] ? row[2] : "0.00") << "</td>"
                         "<td>";
            if (bid_cnt > 0)
                std::cout << "$" << row[4];
            else
                std::cout << "—";
            std::cout << "</td>"
                         "<td>" << bid_cnt << "</td>"
                         "<td>" << (row[3] ? row[3] : "") << "</td>"
                         "<td>" << badge << "</td>"
                         "</tr>\n";
            (void)status;
            (void)final_bid;
        }
        close_table();
        mysql_free_result(res2);
    }
}

// ── Purchases ─────────────────────────────────────────────────────────────────

static void section_purchases(MYSQL* conn, int uid) {
    std::cout << "<h2>Purchases</h2>\n";

    // Items where: auction closed, user's highest bid == item's highest bid
    std::string q =
        "SELECT i.id, i.description, i.end_time,"
        "  MAX(b.bid_amount) AS win_bid,"
        "  u.email AS seller "
        "FROM items i "
        "JOIN bids b ON i.id = b.item_id AND b.bidder_id = " + std::to_string(uid) +
        "  AND b.bid_amount = (SELECT MAX(b2.bid_amount) FROM bids b2 WHERE b2.item_id = i.id) "
        "JOIN users u ON i.seller_id = u.id "
        "WHERE i.end_time <= NOW() "
        "GROUP BY i.id ORDER BY i.end_time DESC";

    if (mysql_query(conn, q.c_str())) { std::cout << "<p class=\"err\">Query error.</p>"; return; }
    MYSQL_RES* res = mysql_store_result(conn);

    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        std::cout << "<span class=\"none\">No purchases yet.</span>\n";
        return;
    }

    open_table({"#", "Description", "Winning Bid", "Seller", "Auction Closed"});
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::cout << "<tr>"
                     "<td>" << (row[0] ? row[0] : "") << "</td>"
                     "<td>" << html_escape(row[1] ? row[1] : "") << "</td>"
                     "<td>$" << (row[3] ? row[3] : "0.00") << "</td>"
                     "<td>" << html_escape(row[4] ? row[4] : "") << "</td>"
                     "<td>" << (row[2] ? row[2] : "") << "</td>"
                     "</tr>\n";
    }
    close_table();
    mysql_free_result(res);
}

// ── Current Bids ──────────────────────────────────────────────────────────────

static void section_current_bids(MYSQL* conn, int uid) {
    std::cout << "<h2>Current Bids</h2>\n";

    // Active auctions where user has at least one bid
    std::string q =
        "SELECT i.id, i.description, i.end_time,"
        "  (SELECT MAX(b2.bid_amount) FROM bids b2 WHERE b2.item_id = i.id) AS top_bid,"
        "  (SELECT MAX(b3.bid_amount) FROM bids b3 WHERE b3.item_id = i.id AND b3.bidder_id = "
        + std::to_string(uid) + ") AS my_bid,"
        "  (SELECT b4.bidder_id FROM bids b4 WHERE b4.item_id = i.id "
        "   ORDER BY b4.bid_amount DESC, b4.bid_time ASC LIMIT 1) AS top_bidder "
        "FROM items i "
        "WHERE i.end_time > NOW() "
        "AND EXISTS (SELECT 1 FROM bids bx WHERE bx.item_id = i.id AND bx.bidder_id = "
        + std::to_string(uid) + ") "
        "ORDER BY i.end_time ASC";

    if (mysql_query(conn, q.c_str())) { std::cout << "<p class=\"err\">Query error.</p>"; return; }
    MYSQL_RES* res = mysql_store_result(conn);

    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        std::cout << "<span class=\"none\">You are not currently bidding on any items.</span>\n";
        return;
    }

    open_table({"#", "Description", "Current High Bid", "Your Max Bid",
                "Status", "Ends At", "Action"});
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        int item_id       = row[0] ? std::atoi(row[0]) : 0;
        double top_bid    = row[3] ? std::atof(row[3]) : 0.0;
        double my_bid     = row[4] ? std::atof(row[4]) : 0.0;
        int top_bidder    = row[5] ? std::atoi(row[5]) : 0;
        bool i_am_winning = (top_bidder == uid);

        std::cout << "<tr>"
                     "<td>" << item_id << "</td>"
                     "<td>" << html_escape(row[1] ? row[1] : "") << "</td>"
                     "<td>$" << (row[3] ? row[3] : "0.00") << "</td>"
                     "<td>$" << (row[4] ? row[4] : "0.00") << "</td>"
                     "<td>";
        if (i_am_winning)
            std::cout << "<span class=\"badge badge-green\">Winning</span>";
        else
            std::cout << "<span class=\"badge badge-red\">Outbid</span>"
                         " <span class=\"warn\">&#9888; You have been outbid!</span>";
        std::cout << "</td>"
                     "<td>" << (row[2] ? row[2] : "") << "</td>"
                     "<td>"
                     "<a href=\"/cgi-bin/bid_sell?action=bid&amp;item_id="
                  << item_id << "\" class=\"btn btn-sm\">Increase Bid</a>"
                     "</td>"
                     "</tr>\n";
        (void)top_bid;
        (void)my_bid;
    }
    close_table();
    mysql_free_result(res);
}

// ── Didn't Win ────────────────────────────────────────────────────────────────

static void section_didnt_win(MYSQL* conn, int uid) {
    std::cout << "<h2>Didn&rsquo;t Win</h2>\n";

    // Closed auctions user bid on but someone else had the higher bid
    std::string q =
        "SELECT i.id, i.description, i.end_time,"
        "  (SELECT MAX(b2.bid_amount) FROM bids b2 WHERE b2.item_id = i.id) AS win_bid "
        "FROM items i "
        "WHERE i.end_time <= NOW() "
        "AND EXISTS (SELECT 1 FROM bids bx WHERE bx.item_id = i.id AND bx.bidder_id = "
        + std::to_string(uid) + ") "
        "AND (SELECT b3.bidder_id FROM bids b3 WHERE b3.item_id = i.id "
        "     ORDER BY b3.bid_amount DESC, b3.bid_time ASC LIMIT 1) <> "
        + std::to_string(uid) +
        " ORDER BY i.end_time DESC";

    if (mysql_query(conn, q.c_str())) { std::cout << "<p class=\"err\">Query error.</p>"; return; }
    MYSQL_RES* res = mysql_store_result(conn);

    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        std::cout << "<span class=\"none\">No lost auctions.</span>\n";
        return;
    }

    open_table({"#", "Description", "Winning Bid", "Auction Closed"});
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::cout << "<tr>"
                     "<td>" << (row[0] ? row[0] : "") << "</td>"
                     "<td>" << html_escape(row[1] ? row[1] : "") << "</td>"
                     "<td>$" << (row[3] ? row[3] : "0.00") << "</td>"
                     "<td>" << (row[2] ? row[2] : "") << "</td>"
                     "</tr>\n";
    }
    close_table();
    mysql_free_result(res);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    auto cookies = parse_cookies();

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

    html_header("My Transactions", true, user_email);
    std::cout << "<h1>My Transactions</h1>\n";

    section_selling(conn, uid);
    section_purchases(conn, uid);
    section_current_bids(conn, uid);
    section_didnt_win(conn, uid);

    mysql_close(conn);
    html_footer();
    return 0;
}
