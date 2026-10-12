#include "database.h"

#include "logging.h"

#include <mysql/mysql.h>

#include <cstring>
#include <memory>
#include <utility>

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

bool query_test_cases(MYSQL* connection,
                      std::uint64_t problem_id,
                      bool only_samples,
                      std::vector<TestCase>& test_cases) {
    const char* sql_with_samples =
        "SELECT id, problem_id, input_data, expected_output_data, "
        "is_sample, order_index "
        "FROM test_cases WHERE problem_id = ? AND is_sample = 1 "
        "ORDER BY order_index ASC";
    const char* sql_all =
        "SELECT id, problem_id, input_data, expected_output_data, "
        "is_sample, order_index "
        "FROM test_cases WHERE problem_id = ? "
        "ORDER BY order_index ASC";

    StatementPtr statement =
        make_statement(connection, only_samples ? sql_with_samples : sql_all);
    if (!statement) {
        return false;
    }

    MYSQL_BIND parameter{};
    parameter.buffer_type = MYSQL_TYPE_LONGLONG;
    parameter.buffer = &problem_id;
    if (mysql_stmt_bind_param(statement.get(), &parameter) != 0 ||
        mysql_stmt_execute(statement.get()) != 0) {
        return false;
    }

    unsigned long input_length = 0;
    unsigned long expected_length = 0;
    std::vector<char> input(1024 * 1024);
    std::vector<char> expected(1024 * 1024);
    signed char is_sample = 0;
    unsigned char order_index = 0;
    std::uint64_t id = 0;
    std::uint64_t fetched_problem_id = 0;

    MYSQL_BIND result[6]{};
    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &id;

    result[1].buffer_type = MYSQL_TYPE_LONGLONG;
    result[1].buffer = &fetched_problem_id;

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = input.data();
    result[2].buffer_length = input.size();
    result[2].length = &input_length;

    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = expected.data();
    result[3].buffer_length = expected.size();
    result[3].length = &expected_length;

    result[4].buffer_type = MYSQL_TYPE_TINY;
    result[4].buffer = &is_sample;

    result[5].buffer_type = MYSQL_TYPE_TINY;
    result[5].buffer = &order_index;
    result[5].is_unsigned = true;

    if (mysql_stmt_bind_result(statement.get(), result) != 0) {
        return false;
    }

    int fetch_result = 0;
    while ((fetch_result = mysql_stmt_fetch(statement.get())) == 0 ||
           fetch_result == MYSQL_DATA_TRUNCATED) {
        TestCase test_case;
        test_case.id = id;
        test_case.problem_id = fetched_problem_id;
        test_case.input_data.assign(input.data(), input_length);
        test_case.expected_output_data.assign(expected.data(), expected_length);
        test_case.is_sample = is_sample != 0;
        test_case.order_index = order_index;
        test_cases.push_back(std::move(test_case));
    }

    return fetch_result == MYSQL_NO_DATA;
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

bool Database::list_problems(std::vector<ProblemSummary>& problems) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "SELECT id, title FROM problems ORDER BY id ASC");
    if (!statement || mysql_stmt_execute(statement.get()) != 0) {
        return false;
    }

    unsigned long title_length = 0;
    char title[201]{};
    MYSQL_BIND result[2]{};
    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = title;
    result[1].buffer_length = sizeof(title);
    result[1].length = &title_length;

    std::uint64_t id = 0;
    result[0].buffer = &id;

    if (mysql_stmt_bind_result(statement.get(), result) != 0) {
        return false;
    }

    int fetch_result = 0;
    while ((fetch_result = mysql_stmt_fetch(statement.get())) == 0 ||
           fetch_result == MYSQL_DATA_TRUNCATED) {
        ProblemSummary problem;
        problem.id = id;
        problem.title.assign(title, title_length);
        problems.push_back(std::move(problem));
    }

    return fetch_result == MYSQL_NO_DATA;
}

bool Database::find_problem_by_id(std::uint64_t id, Problem& problem) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "SELECT id, title, description FROM problems WHERE id = ?");
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

    unsigned long title_length = 0;
    unsigned long description_length = 0;
    char title[201]{};
    std::vector<char> description(16 * 1024 * 1024);

    MYSQL_BIND result[3]{};
    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &problem.id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = title;
    result[1].buffer_length = sizeof(title);
    result[1].length = &title_length;

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = description.data();
    result[2].buffer_length = description.size();
    result[2].length = &description_length;

    if (mysql_stmt_bind_result(statement.get(), result) != 0) {
        return false;
    }

    int fetch_result = mysql_stmt_fetch(statement.get());
    if (fetch_result == MYSQL_NO_DATA) {
        return false;
    }
    if (fetch_result != 0 && fetch_result != MYSQL_DATA_TRUNCATED) {
        return false;
    }

    problem.title.assign(title, title_length);
    problem.description.assign(description.data(), description_length);
    return true;
}

bool Database::list_sample_test_cases(std::uint64_t problem_id,
                                      std::vector<TestCase>& test_cases) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    return query_test_cases(static_cast<MYSQL*>(mysql_), problem_id, true,
                            test_cases);
}

bool Database::list_all_test_cases(std::uint64_t problem_id,
                                   std::vector<TestCase>& test_cases) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    return query_test_cases(static_cast<MYSQL*>(mysql_), problem_id, false,
                            test_cases);
}

bool Database::insert_submission(std::uint64_t user_id,
                                 std::uint64_t problem_id,
                                 const std::string& code,
                                 const std::string& verdict,
                                 std::uint64_t& submission_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "INSERT INTO submissions "
        "(user_id, problem_id, code, status, verdict, finished_at) "
        "VALUES (?, ?, ?, 'done', ?, NOW())");
    if (!statement) {
        return false;
    }

    unsigned long code_length = code.size();
    unsigned long verdict_length = verdict.size();

    MYSQL_BIND parameters[4]{};
    parameters[0].buffer_type = MYSQL_TYPE_LONGLONG;
    parameters[0].buffer = &user_id;

    parameters[1].buffer_type = MYSQL_TYPE_LONGLONG;
    parameters[1].buffer = &problem_id;

    parameters[2].buffer_type = MYSQL_TYPE_STRING;
    parameters[2].buffer = const_cast<char*>(code.c_str());
    parameters[2].buffer_length = code.size();
    parameters[2].length = &code_length;

    parameters[3].buffer_type = MYSQL_TYPE_STRING;
    parameters[3].buffer = const_cast<char*>(verdict.c_str());
    parameters[3].buffer_length = verdict.size();
    parameters[3].length = &verdict_length;

    if (mysql_stmt_bind_param(statement.get(), parameters) != 0 ||
        mysql_stmt_execute(statement.get()) != 0) {
        return false;
    }

    submission_id = mysql_stmt_insert_id(statement.get());
    return true;
}

bool Database::insert_problem(const std::string& title,
                              const std::string& description,
                              std::uint64_t& problem_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "INSERT INTO problems (title, description) VALUES (?, ?)");
    if (!statement) {
        return false;
    }

    unsigned long title_length = title.size();
    unsigned long description_length = description.size();

    MYSQL_BIND parameters[2]{};
    parameters[0].buffer_type = MYSQL_TYPE_STRING;
    parameters[0].buffer = const_cast<char*>(title.c_str());
    parameters[0].buffer_length = title.size();
    parameters[0].length = &title_length;

    parameters[1].buffer_type = MYSQL_TYPE_STRING;
    parameters[1].buffer = const_cast<char*>(description.c_str());
    parameters[1].buffer_length = description.size();
    parameters[1].length = &description_length;

    if (mysql_stmt_bind_param(statement.get(), parameters) != 0 ||
        mysql_stmt_execute(statement.get()) != 0) {
        return false;
    }

    problem_id = mysql_stmt_insert_id(statement.get());
    return true;
}

bool Database::update_problem(std::uint64_t problem_id,
                              const std::string& title,
                              const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "UPDATE problems SET title = ?, description = ? WHERE id = ?");
    if (!statement) {
        return false;
    }

    unsigned long title_length = title.size();
    unsigned long description_length = description.size();

    MYSQL_BIND parameters[3]{};
    parameters[0].buffer_type = MYSQL_TYPE_STRING;
    parameters[0].buffer = const_cast<char*>(title.c_str());
    parameters[0].buffer_length = title.size();
    parameters[0].length = &title_length;

    parameters[1].buffer_type = MYSQL_TYPE_STRING;
    parameters[1].buffer = const_cast<char*>(description.c_str());
    parameters[1].buffer_length = description.size();
    parameters[1].length = &description_length;

    parameters[2].buffer_type = MYSQL_TYPE_LONGLONG;
    parameters[2].buffer = &problem_id;

    return mysql_stmt_bind_param(statement.get(), parameters) == 0 &&
           mysql_stmt_execute(statement.get()) == 0;
}

bool Database::delete_problem(std::uint64_t problem_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_ == nullptr) {
        return false;
    }

    auto* connection = static_cast<MYSQL*>(mysql_);
    StatementPtr statement = make_statement(
        connection,
        "DELETE FROM problems WHERE id = ?");
    if (!statement) {
        return false;
    }

    MYSQL_BIND parameter{};
    parameter.buffer_type = MYSQL_TYPE_LONGLONG;
    parameter.buffer = &problem_id;

    return mysql_stmt_bind_param(statement.get(), &parameter) == 0 &&
           mysql_stmt_execute(statement.get()) == 0;
}
