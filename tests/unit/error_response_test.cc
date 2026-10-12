#include <gtest/gtest.h>

#include "http_utils.h"

#include <nlohmann/json.hpp>

TEST(ErrorResponseTest, BuildsUnifiedErrorBody) {
    httplib::Response response;

    send_error(response, 401, "NOT_AUTHENTICATED", "not authenticated");

    EXPECT_EQ(response.status, 401);
    nlohmann::json body = nlohmann::json::parse(response.body);
    EXPECT_EQ(body["error"]["code"], "NOT_AUTHENTICATED");
    EXPECT_EQ(body["error"]["message"], "not authenticated");
    EXPECT_FALSE(body["error"].contains("details"));
}

TEST(ErrorResponseTest, AddsOptionalDetails) {
    httplib::Response response;

    send_error(response, 500, "INTERNAL_ERROR", "failed",
               {{"detail", "database unavailable"}});

    nlohmann::json body = nlohmann::json::parse(response.body);
    EXPECT_EQ(body["error"]["code"], "INTERNAL_ERROR");
    EXPECT_EQ(body["error"]["details"]["detail"], "database unavailable");
}
