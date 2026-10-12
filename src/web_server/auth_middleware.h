#pragma once

#include <httplib.h>

#include "database.h"
#include "session.h"

bool require_authenticated(const httplib::Request& request,
                           httplib::Response& response,
                           Database& database,
                           SessionManager& sessions,
                           AuthUser& user);

bool require_admin(const httplib::Request& request,
                   httplib::Response& response,
                   Database& database,
                   SessionManager& sessions,
                   AuthUser& user);
