#include <gtest/gtest.h>

#include "config.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace {

class TempConfigFile {
public:
    explicit TempConfigFile(const std::string& content)
        : path_("config_unit_test.yaml") {
        std::ofstream output(path_);
        output << content;
    }

    ~TempConfigFile() {
        std::remove(path_.c_str());
    }

    const std::string& path() const {
        return path_;
    }

private:
    std::string path_;
};

}  // namespace

TEST(ConfigManagerTest, ReturnsTheSameSingletonInstance) {
    ConfigManager& first = ConfigManager::instance();
    ConfigManager& second = ConfigManager::instance();

    EXPECT_EQ(&first, &second);
}

TEST(ConfigManagerTest, UsesDefaultsWhenNoValuesAreProvided) {
    TempConfigFile file("server: {}\n");

    ConfigManager& manager = ConfigManager::instance();
    ASSERT_TRUE(manager.load(file.path()));

    const ServerConfig& config = manager.config();
    EXPECT_EQ(config.listen_address, "0.0.0.0");
    EXPECT_EQ(config.port, 8080);
    EXPECT_EQ(config.web_root, "./web");
    EXPECT_EQ(config.log_level, "info");
}

TEST(ConfigManagerTest, LoadsACompleteConfig) {
    TempConfigFile file(
        "server:\n"
        "  listen_address: 127.0.0.1\n"
        "  port: 9090\n"
        "  web_root: ./public\n"
        "logging:\n"
        "  level: debug\n");

    ConfigManager& manager = ConfigManager::instance();
    ASSERT_TRUE(manager.load(file.path()));

    const ServerConfig& config = manager.config();
    EXPECT_EQ(config.listen_address, "127.0.0.1");
    EXPECT_EQ(config.port, 9090);
    EXPECT_EQ(config.web_root, "./public");
    EXPECT_EQ(config.log_level, "debug");
}

TEST(ConfigManagerTest, LoadsAPartialConfigAndKeepsDefaults) {
    TempConfigFile file(
        "server:\n"
        "  port: 7070\n");

    ConfigManager& manager = ConfigManager::instance();
    ASSERT_TRUE(manager.load(file.path()));

    const ServerConfig& config = manager.config();
    EXPECT_EQ(config.listen_address, "0.0.0.0");
    EXPECT_EQ(config.port, 7070);
    EXPECT_EQ(config.web_root, "./web");
    EXPECT_EQ(config.log_level, "info");
}

TEST(ConfigManagerTest, LoadsDatabaseConfig) {
    TempConfigFile file(
        "server: {}\n"
        "database:\n"
        "  host: 10.0.0.5\n"
        "  port: 3307\n"
        "  user: oj_app\n"
        "  password: secret\n"
        "  name: oj_test\n");

    ConfigManager& manager = ConfigManager::instance();
    ASSERT_TRUE(manager.load(file.path()));

    const DatabaseConfig& database = manager.config().database;
    EXPECT_EQ(database.host, "10.0.0.5");
    EXPECT_EQ(database.port, 3307);
    EXPECT_EQ(database.user, "oj_app");
    EXPECT_EQ(database.password, "secret");
    EXPECT_EQ(database.name, "oj_test");
}

TEST(ConfigManagerTest, ReloadResetsPreviousValues) {
    {
        TempConfigFile file(
            "server:\n"
            "  listen_address: 10.0.0.1\n"
            "  port: 8081\n"
            "  web_root: ./old_web\n");

        ASSERT_TRUE(ConfigManager::instance().load(file.path()));
        EXPECT_EQ(ConfigManager::instance().config().port, 8081);
    }

    {
        TempConfigFile file("server: {}\n");

        ASSERT_TRUE(ConfigManager::instance().load(file.path()));
        EXPECT_EQ(ConfigManager::instance().config().port, 8080);
        EXPECT_EQ(ConfigManager::instance().config().web_root, "./web");
    }
}

TEST(ConfigManagerTest, RejectsPortZero) {
    TempConfigFile file(
        "server:\n"
        "  port: 0\n");

    EXPECT_FALSE(ConfigManager::instance().load(file.path()));
}

TEST(ConfigManagerTest, RejectsPortAboveMaximum) {
    TempConfigFile file(
        "server:\n"
        "  port: 65536\n");

    EXPECT_FALSE(ConfigManager::instance().load(file.path()));
}

TEST(ConfigManagerTest, RejectsMissingServerSection) {
    TempConfigFile file("logging:\n  level: info\n");

    EXPECT_FALSE(ConfigManager::instance().load(file.path()));
}

TEST(ConfigManagerTest, RejectsMalformedYaml) {
    TempConfigFile file("server: [unclosed\n");

    EXPECT_FALSE(ConfigManager::instance().load(file.path()));
}

TEST(ConfigManagerTest, RejectsMissingFile) {
    EXPECT_FALSE(ConfigManager::instance().load("missing_config.yaml"));
}
