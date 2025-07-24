#pragma once
#include "crow.h"
#include "crow/middlewares/cookie_parser.h"

void taskRoutes(crow::App<crow::CookieParser>& app);
