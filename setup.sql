-- Auction Site Database Schema
-- PA1-PA4 Programming Assignments

CREATE DATABASE IF NOT EXISTS auctiondb
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE auctiondb;

-- ─── Users ───────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS users (
    id            INT UNSIGNED    NOT NULL AUTO_INCREMENT,
    email         VARCHAR(255)    NOT NULL,
    password_hash VARCHAR(64)     NOT NULL,   -- SHA-256 hex digest
    salt          VARCHAR(64)     NOT NULL,   -- 32-byte random hex salt
    created_at    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_email (email)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ─── Items ───────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS items (
    id            INT UNSIGNED    NOT NULL AUTO_INCREMENT,
    seller_id     INT UNSIGNED    NOT NULL,
    description   TEXT            NOT NULL,
    starting_bid  DECIMAL(12, 2)  NOT NULL,
    start_time    DATETIME        NOT NULL,
    end_time      DATETIME        NOT NULL,   -- start_time + 168 hours
    status        ENUM('active', 'sold', 'expired') NOT NULL DEFAULT 'active',
    PRIMARY KEY (id),
    KEY idx_seller   (seller_id),
    KEY idx_status   (status),
    KEY idx_end_time (end_time),
    CONSTRAINT fk_items_seller FOREIGN KEY (seller_id) REFERENCES users (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ─── Bids ────────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS bids (
    id          INT UNSIGNED    NOT NULL AUTO_INCREMENT,
    item_id     INT UNSIGNED    NOT NULL,
    bidder_id   INT UNSIGNED    NOT NULL,
    bid_amount  DECIMAL(12, 2)  NOT NULL,
    bid_time    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_item   (item_id),
    KEY idx_bidder (bidder_id),
    CONSTRAINT fk_bids_item   FOREIGN KEY (item_id)   REFERENCES items (id),
    CONSTRAINT fk_bids_bidder FOREIGN KEY (bidder_id) REFERENCES users (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
