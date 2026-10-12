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
        send_json(response, 401, {{"error", "not authenticated"}});
        return false;
    }

    if (!database.find_user_by_id(user_id, user)) {
        send_json(response, 401, {{"error", "user not found"}});
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
        send_json(response, 403,
                  {{"error", "admin permission required"}});
        return false;
    }

    return true;
}
