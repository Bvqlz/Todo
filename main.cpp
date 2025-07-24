#include <sodium/core.h>
#include "crow/middlewares/cookie_parser.h"
#include "auth_routes.h"
#include "crow_routes.h"
#include "db_functions.h"


int main()
{
    if (sodium_init() == -1)
    {
        CROW_LOG_CRITICAL << "sodium_init failed";
        return 1;
    }
    CROW_LOG_INFO << "sodium loaded correctly";

    crow::App<crow::CookieParser> app;
    database::ensure_db();

    taskRoutes(app);
    authRoutes(app);

    app.port(18080)
        .multithreaded()
        .run();

    return 0;
}
