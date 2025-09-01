#pragma once
#include "crow.h"
#include "crow/middlewares/cookie_parser.h"

void authRoutes(crow::App<crow::CookieParser>& app);

