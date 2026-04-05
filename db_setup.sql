-- Computech Database Setup
-- Run: mysql -u root -p < db_setup.sql

CREATE DATABASE IF NOT EXISTS auction_db;
USE auction_db;

CREATE TABLE IF NOT EXISTS users (
    id            INT AUTO_INCREMENT PRIMARY KEY,
    email         VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(64)  NOT NULL,
    created_at    TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id    INT         NOT NULL,
    expires_at DATETIME    NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS items (
    id           INT AUTO_INCREMENT PRIMARY KEY,
    seller_id    INT            NOT NULL,
    description  TEXT           NOT NULL,
    starting_bid DECIMAL(10,2)  NOT NULL,
    start_time   DATETIME       NOT NULL,
    end_time     DATETIME       NOT NULL,
    FOREIGN KEY (seller_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS bids (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    item_id    INT           NOT NULL,
    bidder_id  INT           NOT NULL,
    bid_amount DECIMAL(10,2) NOT NULL,
    bid_time   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (item_id)   REFERENCES items(id) ON DELETE CASCADE,
    FOREIGN KEY (bidder_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Create a dedicated DB user (adjust password as needed)
-- CREATE USER IF NOT EXISTS 'auction_user'@'localhost' IDENTIFIED BY 'auction_pass';
-- GRANT ALL PRIVILEGES ON auction_db.* TO 'auction_user'@'localhost';
-- FLUSH PRIVILEGES;
