# ──────────────────────────────────────────────────────────────────────────────
# Makefile – AuctionHub CGI programs (C++17)
#
# Build all targets:   make
# Install to cgi-bin:  make install   (requires write access to CGI_DIR)
# Clean build output:  make clean
#
# Dependencies:
#   • libmysqlclient-dev  (Ubuntu/Debian: sudo apt install libmysqlclient-dev)
#   • libssl-dev          (Ubuntu/Debian: sudo apt install libssl-dev)
#   • g++ ≥ 7
# ──────────────────────────────────────────────────────────────────────────────

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 $(shell mysql_config --cflags)
LDFLAGS  := $(shell mysql_config --libs) -lssl -lcrypto

# Apache CGI directory – adjust for your server setup
CGI_DIR  := /usr/lib/cgi-bin

COMMON_SRC := common.cpp
COMMON_HDR := common.h

TARGETS := auth transactions bid_sell auctions

# ──────────────────────────────────────────────────────────────────────────────

.PHONY: all install clean

all: $(TARGETS)

auth: cgi-bin/auth.cpp $(COMMON_SRC) $(COMMON_HDR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(COMMON_SRC) $(LDFLAGS)

transactions: cgi-bin/transactions.cpp $(COMMON_SRC) $(COMMON_HDR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(COMMON_SRC) $(LDFLAGS)

bid_sell: cgi-bin/bid_sell.cpp $(COMMON_SRC) $(COMMON_HDR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(COMMON_SRC) $(LDFLAGS)

auctions: cgi-bin/auctions.cpp $(COMMON_SRC) $(COMMON_HDR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(COMMON_SRC) $(LDFLAGS)

# Copy compiled binaries to the web server's CGI directory
install: all
	install -m 755 $(TARGETS) $(CGI_DIR)/

clean:
	rm -f $(TARGETS)
