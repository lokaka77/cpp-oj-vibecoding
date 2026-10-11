#include <gtest/gtest.h>

#include "logging.h"

#include <ostream>
#include <sstream>
#include <string>

namespace {

class StreamCapture {
public:
    explicit StreamCapture(std::ostream& stream)
        : stream_(stream),
          old_buffer_(stream.rdbuf()) {
        stream_.rdbuf(buffer_.rdbuf());
    }

    ~StreamCapture() {
        stream_.rdbuf(old_buffer_);
    }

    std::string str() const {
        return buffer_.str();
    }

private:
    std::ostream& stream_;
    std::streambuf* old_buffer_;
    std::ostringstream buffer_;
};

class LogLevelGuard {
public:
    explicit LogLevelGuard(LogLevel level)
        : old_level_(g_log_level) {
        g_log_level = level;
    }

    ~LogLevelGuard() {
        g_log_level = old_level_;
    }

private:
    LogLevel old_level_;
};

}  // namespace

TEST(LoggingTest, ParsesKnownLevels) {
    EXPECT_EQ(parse_log_level("debug"), LogLevel::Debug);
    EXPECT_EQ(parse_log_level("info"), LogLevel::Info);
    EXPECT_EQ(parse_log_level("warning"), LogLevel::Warning);
    EXPECT_EQ(parse_log_level("warn"), LogLevel::Warning);
    EXPECT_EQ(parse_log_level("error"), LogLevel::Error);
}

TEST(LoggingTest, UnknownLevelDefaultsToInfo) {
    EXPECT_EQ(parse_log_level("verbose"), LogLevel::Info);
    EXPECT_EQ(parse_log_level(""), LogLevel::Info);
}

TEST(LoggingTest, InfoWritesToStandardOutput) {
    LogLevelGuard guard(LogLevel::Info);
    StreamCapture output(std::cout);

    log_info("server started");

    const std::string text = output.str();
    EXPECT_NE(text.find("[INFO]"), std::string::npos);
    EXPECT_NE(text.find("server started"), std::string::npos);
}

TEST(LoggingTest, DebugIsSuppressedAtInfoLevel) {
    LogLevelGuard guard(LogLevel::Info);
    StreamCapture output(std::cout);

    log_debug("hidden detail");

    EXPECT_EQ(output.str(), "");
}

TEST(LoggingTest, DebugIsWrittenAtDebugLevel) {
    LogLevelGuard guard(LogLevel::Debug);
    StreamCapture output(std::cout);

    log_debug("debug detail");

    const std::string text = output.str();
    EXPECT_NE(text.find("[DEBUG]"), std::string::npos);
    EXPECT_NE(text.find("debug detail"), std::string::npos);
}

TEST(LoggingTest, WarningWritesToStandardError) {
    LogLevelGuard guard(LogLevel::Info);
    StreamCapture output(std::cerr);

    log_warning("disk usage is high");

    const std::string text = output.str();
    EXPECT_NE(text.find("[WARN]"), std::string::npos);
    EXPECT_NE(text.find("disk usage is high"), std::string::npos);
}

TEST(LoggingTest, ErrorWritesToStandardError) {
    LogLevelGuard guard(LogLevel::Info);
    StreamCapture output(std::cerr);

    log_error("connection failed");

    const std::string text = output.str();
    EXPECT_NE(text.find("[ERROR]"), std::string::npos);
    EXPECT_NE(text.find("connection failed"), std::string::npos);
}
