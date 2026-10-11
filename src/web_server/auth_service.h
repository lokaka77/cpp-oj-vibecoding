#pragma once

#include <httplib.h>

#include "database.h"
#include "session.h"

void register_auth_routes(httplib::Server& server,
                          Database& database,
                          SessionManager& sessions);
