#include "auth_middleware.h"

#include "http_utils.h"

#include <cstdint>

bool require_authenticated(const httplib::Request& request,
                           httplib::Response& response,
                           Database& database,
                           SessionManager& sessions,
                           AuthUser& user) {
    std::uint64_t user_id = 0;
    if (!sessions.get(cookie_value(request, "session_id"), user_id)) {
        send_error(response, 401, "NOT_AUTHENTICATED",
                   "not authenticated");
        return false;
    }

    if (!database.find_user_by_id(user_id, user)) {
        send_error(response, 401, "USER_NOT_FOUND", "user not found");
        return false;
    }

    return true;
}

bool require_admin(const httplib::Request& request,
                   httplib::Response& response,
                   Database& database,
                   SessionManager& sessions,
                   AuthUser& user) {
    if (!require_authenticated(request, response, database, sessions, user)) {
        return false;
    }

    if (user.role != "admin") {
        send_error(response, 403, "ADMIN_REQUIRED",
                   "admin permission required");
        return false;
    }

    return true;
}
