#include <gtest/gtest.h>

#include "database.h"

#include <mysql/mysql.h>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

namespace {

struct MysqlDeleter {
    void operator()(MYSQL* connection) const {
        if (connection != nullptr) {
            mysql_close(connection);
        }
    }
};

std::string unique_suffix() {
    return std::to_string(static_cast<std::uint64_t>(std::time(nullptr)));
}

}  // namespace

class DatabaseIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        DatabaseConfig config;
        config.host = "127.0.0.1";
        config.port = 3306;
        config.user = "oj_app";
        config.password = "oj_app_pass";
        config.name = "oj";

        if (!database_.connect(config)) {
            GTEST_SKIP() << "MySQL is not available";
        }

        raw_.reset(mysql_init(nullptr));
        if (!raw_) {
            GTEST_SKIP() << "Unable to initialize MySQL connection";
        }

        my_bool ssl_enforce = 0;
        my_bool ssl_verify = 0;
        mysql_options(raw_.get(), MYSQL_OPT_SSL_ENFORCE, &ssl_enforce);
        mysql_options(raw_.get(), MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &ssl_verify);

        if (mysql_real_connect(raw_.get(),
                               config.host.c_str(),
                               config.user.c_str(),
                               config.password.c_str(),
                               config.name.c_str(),
                               config.port,
                               nullptr,
                               0) == nullptr) {
            raw_.reset();
            GTEST_SKIP() << "Unable to connect to MySQL test database";
        }
    }

    void TearDown() override {
        for (auto it = cleanup_.rbegin(); it != cleanup_.rend(); ++it) {
            mysql_query(raw_.get(), it->c_str());
        }
        cleanup_.clear();
        raw_.reset();
    }

    bool execute(const std::string& sql) {
        return mysql_query(raw_.get(), sql.c_str()) == 0;
    }

    void defer_cleanup(const std::string& sql) {
        cleanup_.push_back(sql);
    }

    Database database_;
    std::unique_ptr<MYSQL, MysqlDeleter> raw_;
    std::vector<std::string> cleanup_;
};

TEST_F(DatabaseIntegrationTest, InsertsAndFindsUser) {
    const std::string username = "it_user_" + unique_suffix();
    defer_cleanup("DELETE FROM users WHERE username='" + username + "'");

    AuthUser user;
    ASSERT_TRUE(database_.insert_user(username, "test_hash", "user"));
    ASSERT_TRUE(database_.find_user_by_username(username, user));

    EXPECT_EQ(user.username, username);
    EXPECT_EQ(user.password_hash, "test_hash");
    EXPECT_EQ(user.role, "user");

    AuthUser by_id;
    ASSERT_TRUE(database_.find_user_by_id(user.id, by_id));
    EXPECT_EQ(by_id.username, username);
}

TEST_F(DatabaseIntegrationTest, ListsProblemAndVisibleSamples) {
    const std::string title = "IT Problem " + unique_suffix();
    ASSERT_TRUE(execute(
        "INSERT INTO problems (title, description) VALUES ('" + title +
        "', 'integration test description')"));

    const std::uint64_t problem_id = mysql_insert_id(raw_.get());
    defer_cleanup("DELETE FROM problems WHERE id=" +
                  std::to_string(problem_id));

    ASSERT_TRUE(execute(
        "INSERT INTO test_cases "
        "(problem_id, input_data, expected_output_data, is_sample, order_index) "
        "VALUES (" + std::to_string(problem_id) + ", '1 2\\n', '3\\n', 1, 1)"));
    ASSERT_TRUE(execute(
        "INSERT INTO test_cases "
        "(problem_id, input_data, expected_output_data, is_sample, order_index) "
        "VALUES (" + std::to_string(problem_id) + ", '10 20\\n', '30\\n', 1, 2)"));
    ASSERT_TRUE(execute(
        "INSERT INTO test_cases "
        "(problem_id, input_data, expected_output_data, is_sample, order_index) "
        "VALUES (" + std::to_string(problem_id) + ", '-1 1\\n', '0\\n', 0, 3)"));

    std::vector<ProblemSummary> problems;
    ASSERT_TRUE(database_.list_problems(problems));

    bool found = false;
    for (const ProblemSummary& problem : problems) {
        if (problem.id == problem_id) {
            EXPECT_EQ(problem.title, title);
            found = true;
        }
    }
    EXPECT_TRUE(found);

    Problem detail;
    ASSERT_TRUE(database_.find_problem_by_id(problem_id, detail));
    EXPECT_EQ(detail.title, title);
    EXPECT_EQ(detail.description, "integration test description");

    std::vector<TestCase> samples;
    ASSERT_TRUE(database_.list_sample_test_cases(problem_id, samples));
    ASSERT_EQ(samples.size(), 2);
    EXPECT_TRUE(samples[0].is_sample);
    EXPECT_EQ(samples[0].order_index, 1);
    EXPECT_EQ(samples[1].order_index, 2);
}

TEST_F(DatabaseIntegrationTest, MissingProblemReturnsFalse) {
    Problem problem;
    EXPECT_FALSE(database_.find_problem_by_id(999999999, problem));
}

TEST_F(DatabaseIntegrationTest, ClaimsAndFinishesPendingSubmission) {
    const std::string title = "IT Judge Problem " + unique_suffix();
    ASSERT_TRUE(execute(
        "INSERT INTO problems (title, description) VALUES ('" + title +
        "', 'judge queue integration test')"));

    const std::uint64_t problem_id = mysql_insert_id(raw_.get());
    defer_cleanup("DELETE FROM problems WHERE id=" +
                  std::to_string(problem_id));

    const std::string code = "int main() { return 0; }";
    std::uint64_t submission_id = 0;
    ASSERT_TRUE(database_.insert_pending_submission(
        1, problem_id, code, submission_id));

    JudgeTask task;
    ASSERT_TRUE(database_.claim_pending_submission(task));
    EXPECT_EQ(task.submission_id, submission_id);
    EXPECT_EQ(task.problem_id, problem_id);
    EXPECT_EQ(task.code, code);

    std::string status;
    std::string verdict;
    ASSERT_TRUE(database_.get_submission_result(submission_id, status, verdict));
    EXPECT_EQ(status, "judging");
    EXPECT_TRUE(verdict.empty());

    std::string error;
    ASSERT_TRUE(database_.finish_submission(submission_id, "AC", error));
    ASSERT_TRUE(database_.get_submission_result(submission_id, status, verdict));
    EXPECT_EQ(status, "done");
    EXPECT_EQ(verdict, "AC");
}

TEST_F(DatabaseIntegrationTest, WritesAllSupportedVerdicts) {
    const std::string title = "IT Verdict Problem " + unique_suffix();
    ASSERT_TRUE(execute(
        "INSERT INTO problems (title, description) VALUES ('" + title +
        "', 'verdict integration test')"));

    const std::uint64_t problem_id = mysql_insert_id(raw_.get());
    defer_cleanup("DELETE FROM problems WHERE id=" +
                  std::to_string(problem_id));

    for (const std::string& expected_verdict :
         {"AC", "WA", "TLE", "RE", "CE"}) {
        std::uint64_t submission_id = 0;
        ASSERT_TRUE(database_.insert_pending_submission(
            1, problem_id, "int main() { return 0; }", submission_id));

        std::string error;
        ASSERT_TRUE(database_.finish_submission(
            submission_id, expected_verdict, error));

        std::string status;
        std::string stored_verdict;
        ASSERT_TRUE(database_.get_submission_result(
            submission_id, status, stored_verdict));
        EXPECT_EQ(status, "done");
        EXPECT_EQ(stored_verdict, expected_verdict);
    }
}

TEST_F(DatabaseIntegrationTest, ListsSolvedProblemIdsForUser) {
    const std::string username = "it_solved_" + unique_suffix();
    const std::string title = "IT Solved Problem " + unique_suffix();

    ASSERT_TRUE(database_.insert_user(username, "hash", "user"));
    defer_cleanup("DELETE FROM users WHERE username='" + username + "'");

    AuthUser user;
    ASSERT_TRUE(database_.find_user_by_username(username, user));

    ASSERT_TRUE(execute(
        "INSERT INTO problems (title, description) VALUES ('" + title +
        "', 'solved list integration test')"));
    const std::uint64_t problem_id = mysql_insert_id(raw_.get());
    defer_cleanup("DELETE FROM problems WHERE id=" +
                  std::to_string(problem_id));

    std::uint64_t submission_id = 0;
    ASSERT_TRUE(database_.insert_pending_submission(
        user.id, problem_id, "int main() { return 0; }", submission_id));

    std::string error;
    ASSERT_TRUE(database_.finish_submission(submission_id, "AC", error));

    std::vector<std::uint64_t> solved_ids;
    ASSERT_TRUE(database_.list_solved_problem_ids(user.id, solved_ids));
    EXPECT_NE(std::find(solved_ids.begin(), solved_ids.end(), problem_id),
              solved_ids.end());
}

TEST_F(DatabaseIntegrationTest, ListsAndDeletesRegularUser) {
    const std::string username = "it_delete_" + unique_suffix();
    defer_cleanup("DELETE FROM users WHERE username='" + username + "'");

    ASSERT_TRUE(database_.insert_user(username, "hash", "user"));

    AuthUser user;
    ASSERT_TRUE(database_.find_user_by_username(username, user));

    std::vector<UserSummary> users;
    ASSERT_TRUE(database_.list_users(users));

    bool found = false;
    for (const UserSummary& summary : users) {
        if (summary.id == user.id) {
            EXPECT_EQ(summary.username, username);
            EXPECT_EQ(summary.role, "user");
            EXPECT_FALSE(summary.created_at.empty());
            found = true;
        }
    }
    EXPECT_TRUE(found);

    ASSERT_TRUE(database_.delete_user(user.id));

    AuthUser deleted;
    EXPECT_FALSE(database_.find_user_by_id(user.id, deleted));
}

TEST_F(DatabaseIntegrationTest, ProtectsAdminAccountFromDeletion) {
    EXPECT_FALSE(database_.delete_user(1));
}
