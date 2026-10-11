#include "database.h"

#include "logging.h"

#include <mysql/mysql.h>

#include <cstring>
#include <memory>

namespace {

using StatementPtr =
    std::unique_ptr<MYSQL_STMT, void (*)(MYSQL_STMT*)>;

StatementPtr make_statement(MYSQL* connection, const char* sql) {
    MYSQL_STMT* statement = mysql_stmt_init(connection);
    if (statement == nullptr ||
        mysql_stmt_prepare(statement, sql, std::strlen(sql)) != 0) {
        if (statement != nullptr) {
            mysql_stmt_close(statement);
        }
        return StatementPtr(nullptr, [](MYSQL_STMT*) {});
    }
    return StatementPtr(statement, [](MYSQL_STMT* value) {
        mysql_stmt_close(value);
    });
}

bool fetch_user(MYSQL_STMT* statement, AuthUser& user) {
    unsigned long lengths[4]{};
    char username[51]{};
    char password_hash[255]{};
    char role[16]{};

    MYSQL_BIND result[4]{};
    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &user.id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = username;
    result[1].buffer_length = sizeof(username);
    result[1].length = &lengths[1];

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = password_hash;
    result[2].buffer_length = sizeof(password_hash);
    result[2].length = &lengths[2];

    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = role;
    result[3].buffer_length = sizeof(role);
    result[3].length = &lengths[3];

    if (mysql_stmt_bind_result(statement, result) != 0) {
        return false;
    }

    int fetch_result = mysql_stmt_fetch(statement);
    if (fetch_result == MYSQL_NO_DATA) {
        return false;
    }
    if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
        return false;
    }

    user.username.assign(username, lengths[1]);
    user.password_hash.assign(password_hash, lengths[2]);
    user.role.assign(role, lengths[3]);
    return true;
}

}  // namespace

Database::~Database() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ != nullptr) {
        mysql_close(static_cast<MYSQL*>(mysql_));
    }
}

bool Database::connect(const DatabaseConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    MYSQL* connection = mysql_init(nullptr);
    if (connection == nullptr) {
        return false;
    }

    my_bool ssl_enforce = 0;
    my_bool ssl_verify_server_cert = 0;
    mysql_options(connection, MYSQL_OPT_SSL_ENFORCE, &ssl_enforce);
    mysql_options(connection, MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                  &ssl_verify_server_cert);

    if (mysql_real_connect(connection,
                           config.host.c_str(),
                           config.user.c_str(),
                           config.password.c_str(),
                           config.name.c_str(),
                           config.port,
                           nullptr,
                           0) == nullptr) {
        log_error(std::string("MySQL connect failed: ") +
                  mysql_error(connection));
        mysql_close(connection);
        return false;
    }

    if (mysql_ != nullptr) {
        mysql_close(static_cast<MYSQL*>(mysql_));
    }
    mysql_ = connection;
    return true;
}

bool Database::find_user_by_username(const std::string& username,
                                     AuthUser& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "SELECT id, username, password_hash, role "
        "FROM users WHERE username = ?");
    if (!statement) {
        return false;
    }

    unsigned long username_length = username.size();
    MYSQL_BIND parameter{};
    parameter.buffer_type = MYSQL_TYPE_STRING;
    parameter.buffer = const_cast<char*>(username.c_str());
    parameter.buffer_length = username.size();
    parameter.length = &username_length;

    if (mysql_stmt_bind_param(statement.get(), &parameter) != 0 ||
        mysql_stmt_execute(statement.get()) != 0) {
        return false;
    }

    return fetch_user(statement.get(), user);
}

bool Database::find_user_by_id(std::uint64_t id, AuthUser& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "SELECT id, username, password_hash, role "
        "FROM users WHERE id = ?");
    if (!statement) {
        return false;
    }

    MYSQL_BIND parameter{};
    parameter.buffer_type = MYSQL_TYPE_LONGLONG;
    parameter.buffer = &id;

    if (mysql_stmt_bind_param(statement.get(), &parameter) != 0 ||
        mysql_stmt_execute(statement.get()) != 0) {
        return false;
    }

    return fetch_user(statement.get(), user);
}

bool Database::insert_user(const std::string& username,
                           const std::string& password_hash,
                           const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)");
    if (!statement) {
        return false;
    }

    unsigned long username_length = username.size();
    unsigned long password_length = password_hash.size();
    unsigned long role_length = role.size();

    MYSQL_BIND parameters[3]{};
    parameters[0].buffer_type = MYSQL_TYPE_STRING;
    parameters[0].buffer = const_cast<char*>(username.c_str());
    parameters[0].buffer_length = username.size();
    parameters[0].length = &username_length;

    parameters[1].buffer_type = MYSQL_TYPE_STRING;
    parameters[1].buffer = const_cast<char*>(password_hash.c_str());
    parameters[1].buffer_length = password_hash.size();
    parameters[1].length = &password_length;

    parameters[2].buffer_type = MYSQL_TYPE_STRING;
    parameters[2].buffer = const_cast<char*>(role.c_str());
    parameters[2].buffer_length = role.size();
    parameters[2].length = &role_length;

    return mysql_stmt_bind_param(statement.get(), parameters) == 0 &&
           mysql_stmt_execute(statement.get()) == 0;
}
