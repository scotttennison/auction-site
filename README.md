# Computech – Antique Electronics Auction Platform

A CGI-based auction site for buying and selling classic electronics, built with C++17 and MySQL.

## Features

- **Browse Auctions**: View all active antique electronics listings
- **User Registration & Login**: Create accounts and manage sessions
- **Bidding System**: Place bids on items in real-time
- **Sell Items**: List your own antique electronics for auction
- **Transaction History**: Track your bids and sales
- **Transparent Pricing**: Starting bids and current bid amounts clearly displayed

## Project Structure

```
├── common.h              # Shared header with database and utility functions
├── common.cpp            # Implementation of shared utilities
├── cgi-bin/
│   ├── auth.cpp         # User registration, login, logout
│   ├── auctions.cpp     # Browse and view auction listings
│   ├── bid_sell.cpp     # Place bids and create new listings
│   └── transactions.cpp # View transaction history
├── db_setup.sql         # Initial database schema
├── setup.sql            # Alternative database setup
├── Makefile             # Build configuration
└── .vscode/             # VS Code configuration
```

## Tech Stack

- **Language**: C++17
- **Web Server**: Apache (CGI)
- **Database**: MySQL 9.6
- **Security**: OpenSSL (SHA-256 password hashing)
- **Build**: GNU Make with g++

## Building

### Dependencies (macOS with Homebrew)

```bash
brew install mysql openssl@3
```

### Build Commands

```bash
cd "Auction Site"
make clean
make all CXXFLAGS='-std=c++17 -Wall -Wextra -O2 \
  -I/opt/homebrew/Cellar/mysql/9.6.0_1/include \
  -I/opt/homebrew/Cellar/openssl@3/3.6.1/include' \
  LDFLAGS='-L/opt/homebrew/Cellar/mysql/9.6.0_1/lib \
  -L/opt/homebrew/lib -lmysqlclient -lz -lzstd -lssl -lcrypto -lresolv'
```

Or simply:
```bash
make
```

This generates four CGI binaries: `auth`, `auctions`, `bid_sell`, and `transactions`.

## Database Setup

### Create User & Database

```bash
mysql -u root << 'EOF'
CREATE USER 'auction_user'@'localhost' IDENTIFIED BY 'auction_pass';
CREATE DATABASE auctiondb;
GRANT ALL PRIVILEGES ON auctiondb.* TO 'auction_user'@'localhost';
FLUSH PRIVILEGES;
EOF
```

### Import Schema

```bash
mysql -u auction_user -p'auction_pass' auctiondb < db_setup.sql
```

## Deployment

### Install to Apache CGI Directory

```bash
sudo cp auth transactions bid_sell auctions /Library/WebServer/CGI-Executables/
sudo chmod 755 /Library/WebServer/CGI-Executables/{auth,transactions,bid_sell,auctions}
```

### Configure Apache

Ensure your Apache config includes:
```apache
ScriptAlias /cgi-bin/ "/Library/WebServer/CGI-Executables/"
```

### Start Apache

```bash
sudo apachectl start
```

## Usage

### Local Access

Visit http://localhost/cgi-bin/auctions in your browser.

### Remote Access

Use ngrok to share over the internet:
```bash
ngrok http 80
```

Then share the generated URL with others.

## Site URLs

- **Browse Auctions**: `http://localhost/cgi-bin/auctions`
- **Register/Login**: `http://localhost/cgi-bin/auth`
- **Bid & Sell**: `http://localhost/cgi-bin/bid_sell`
- **My Transactions**: `http://localhost/cgi-bin/transactions`

## Sample Data

The database includes 50+ pre-loaded antique electronics items including:
- Vintage radios & tube televisions
- Classic computers (Apple II, Commodore 64, IBM 5150)
- Retro gaming consoles (Atari, NES, Sega Genesis)
- Old telephones, phonographs & cameras
- And more rare collectibles

## Code Features

### Security

- Passwords hashed with SHA-256
- Session tokens generated with cryptographic randomness
- SQL injection protection via parameterized queries
- XSS protection with HTML escaping

### Database

- UTF-8 character encoding
- Proper foreign key constraints
- Automatic timestamp tracking
- Transaction logging

### UI

- Responsive design with flexbox layout
- Clean navigation bar with logo
- Error/success message displays
- Tabbed interfaces for forms

## Development

### VS Code

IntelliSense is configured in `.vscode/c_cpp_properties.json` for proper header resolution.

### Adding Features

1. Edit source files in `cgi-bin/`
2. Run `make clean && make all` (see Build Commands above)
3. Redeploy binaries to `/Library/WebServer/CGI-Executables/`

## License

This project is provided as-is for educational and personal use.

## Contact

Created by Scott Tennison (scotttennison on GitHub)
