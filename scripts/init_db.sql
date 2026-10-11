-- OJ 数据库初始化脚本
-- 初始管理员账号：admin
-- 初始管理员密码：admin123

CREATE DATABASE IF NOT EXISTS oj
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE oj;

CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role ENUM('user', 'admin') NOT NULL DEFAULT 'user',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_users_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS problems (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    title VARCHAR(200) NOT NULL,
    description MEDIUMTEXT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS test_cases (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    problem_id BIGINT UNSIGNED NOT NULL,
    input_data MEDIUMTEXT NOT NULL,
    expected_output_data MEDIUMTEXT NOT NULL,
    is_sample TINYINT(1) NOT NULL DEFAULT 0,
    order_index TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uk_test_cases_problem_order (problem_id, order_index),
    KEY idx_test_cases_problem_id (problem_id),
    CONSTRAINT fk_test_cases_problem
        FOREIGN KEY (problem_id)
        REFERENCES problems (id)
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS submissions (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    user_id BIGINT UNSIGNED NOT NULL,
    problem_id BIGINT UNSIGNED NOT NULL,
    code MEDIUMTEXT NOT NULL,
    status ENUM('pending', 'judging', 'done') NOT NULL DEFAULT 'pending',
    verdict ENUM('AC', 'WA', 'TLE', 'RE', 'CE') NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    finished_at DATETIME NULL,
    PRIMARY KEY (id),
    KEY idx_submissions_status (status),
    KEY idx_submissions_problem_id (problem_id),
    KEY idx_submissions_user_id (user_id),
    CONSTRAINT fk_submissions_user
        FOREIGN KEY (user_id)
        REFERENCES users (id)
        ON DELETE CASCADE,
    CONSTRAINT fk_submissions_problem
        FOREIGN KEY (problem_id)
        REFERENCES problems (id)
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO users (username, password_hash, role)
VALUES (
    'admin',
    '$argon2id$v=19$m=65536,t=2,p=1$Zv5TqIucm25akXAjfGudHQ$eKJGuR23vtyRhzsFLFJTu+Ivw5q7WGgTpKw0bYhzBtA',
    'admin'
)
ON DUPLICATE KEY UPDATE username = username;

CREATE USER IF NOT EXISTS 'oj_app'@'localhost'
    IDENTIFIED WITH mysql_native_password BY 'oj_app_pass';
CREATE USER IF NOT EXISTS 'oj_app'@'127.0.0.1'
    IDENTIFIED WITH mysql_native_password BY 'oj_app_pass';
GRANT ALL PRIVILEGES ON oj.* TO 'oj_app'@'localhost';
GRANT ALL PRIVILEGES ON oj.* TO 'oj_app'@'127.0.0.1';
FLUSH PRIVILEGES;
