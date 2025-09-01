#include "auth_routes.h"
#include "AuthHandle.h"
#include "db_functions.h"
#include "user.h"
#include <sodium.h>

void authRoutes(crow::App<crow::CookieParser>& app)
{
    CROW_ROUTE(app, "/register")
            .methods("POST"_method) // POST method sends data to a server. If successive requests, can create the same order multiple times
    ([](const crow::request& req)
    {
        auto json = crow::json::load(req.body);
        //if json is empty or we do not have an instance in the username or password field.
        //we use .count() which return a 0 or a 1 if a json object has a key.
        if (!json || !json.count("username") || !json.count("password"))
        {
            return crow::response(crow::status::BAD_REQUEST, "Missing username or password");
        }

        //.s() and related functions return the value a json node as a string.
        std::string username = json["username"].s();
        std::string plain_password = json["password"].s();
        //.has_value() is used due to optional data type. Checks if object has a value.
        if (database::getUsername(username).has_value())
        {
            //in this case it's used to check if the table does not already have this user
            return crow::response(crow::status::CONFLICT, "Username already exists");
        }

        //this will help us create the hash
        char password_hash_buf[crypto_pwhash_STRBYTES];
        if (crypto_pwhash_str(password_hash_buf,
                        plain_password.c_str(),
                        plain_password.length(),
                        crypto_pwhash_OPSLIMIT_MODERATE,
                        crypto_pwhash_MEMLIMIT_MODERATE) != 0)
        {
            CROW_LOG_ERROR << "Failed to generate password";
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, "Failed to hash password.");
        }

        std::string password_hash_str(password_hash_buf);
        std::optional<int> userID = database::createUser(username, password_hash_str);
        if (userID.has_value()) //If user creation was a success, then we should have a value.
        {
            crow::json::wvalue mJson;
            mJson["message"] = "User registered successfully.";
            mJson["userID"] = userID.value(); //This will return the actual value that the object has. We only do this once has_value confirms to prevent errors.
            crow::response res(crow::status::CREATED); // Create a response object
            res.write(mJson.dump()); // Write the JSON string to the response body
            res.set_header("Content-Type", "application/json"); // Set the Content-Type header
            return res; // Return the response object
        }
        else
        {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, "User registration failed.");
        }
    });

    CROW_ROUTE(app, "/login")
            .methods("POST"_method)
    ([&app](const crow::request& req, crow::response &res) // we capture a reference to the app to use cookies
    {
        CROW_LOG_INFO << "Received a login request";
        CROW_LOG_INFO << "Request body: " << req.body;

        auto& cookie_ctx = app.get_context<crow::CookieParser>(req); //Create an object where cookies can be accessed
        std::string existingSession = cookie_ctx.get_cookie("sessionID"); //This retrieves the cookie value from our request under the sessionID key.

        if (!existingSession.empty()) //only works if a value is found
        {
            AuthHandle::deleteSession(existingSession); // will delete the cookie from the map that was created.
            //this line will clear our broswer cookie
            cookie_ctx.set_cookie("sessionID", "").path("/").max_age(0).httponly(); //this set the cookie's value to an empty string. Also made its age as zero.
            CROW_LOG_INFO << "Cleared existing session for login request: " << existingSession;

        }

        try
            {
                auto json = crow::json::load(req.body);
                if (!json || !json.count("username") || !json.count("password"))
                {
                    CROW_LOG_ERROR << "Invalid JSON in login request body.";
                    //creating a response requires a code, a body and an end.
                    res.code = crow::status::BAD_REQUEST;
                    crow::json::wvalue error_json;
                    error_json["message"] = "Missing username or password.";
                    res.write(error_json.dump());
                    res.set_header("Content-Type", "application/json");
                    res.end();
                    return;
                }

                std::string username = json["username"].s();
                std::string plain_password = json["password"].s();
                CROW_LOG_INFO << "Login attempt for username: " << username;

                std::optional<User> user;
                try
                {
                    user = database::getUsername(username); // Get user by username
                }
                catch (const std::exception& e)
                {
                    CROW_LOG_ERROR << "Database error getting user by username during login: " << e.what();
                    res.code = crow::status::INTERNAL_SERVER_ERROR;
                    crow::json::wvalue error_json;
                    error_json["message"] = "Database error during login";
                    res.write(error_json.dump());
                    res.set_header("Content-Type", "application/json");
                    res.end();
                    return;
                }

                if (!user.has_value()) //if our user object is empty then we can say that either was incorrect
                {
                    res.code = crow::status::UNAUTHORIZED;
                    crow::json::wvalue error_json;
                    error_json["message"] = "Invalid username or password.";
                    res.write(error_json.dump());
                    res.set_header("Content-Type", "application/json");
                    res.end();
                    return;
                }
                CROW_LOG_INFO << "User '" << username << "' found. Verifying password...";

                // Verify password using Libsodium
                if (crypto_pwhash_str_verify(user->password_hash.c_str(), // we use arrow notation for optional data types instead of dot notation.
                plain_password.c_str(), plain_password.length()) != 0)
                {
                    CROW_LOG_INFO << "Password verification failed for user: " << username;
                    res.code = crow::status::UNAUTHORIZED;
                    crow::json::wvalue error_json;
                    error_json["message"] = "Invalid username or password.";
                    res.write(error_json.dump());
                    res.set_header("Content-Type", "application/json");
                    res.end();
                    return;
                }

                //once authentication was successful, we can create and set a new session
                std::string newSession = AuthHandle::genSessionID();
                AuthHandle::storeSession(newSession, user->id);
                CROW_LOG_INFO << "New session created for user " << user->id << ": " << newSession;

                //this creates the session cookie using middleware
                cookie_ctx.set_cookie("sessionID", newSession)
                        .path("/")
                        .max_age(3600)
                        .httponly();

                CROW_LOG_INFO << "Session cookie 'session_id' set for user " << user->id;

                res.code = crow::status::OK;
                crow::json::wvalue success_response;
                success_response["message"] = "Login successful!"; // Consistent message with HTML
                res.write(success_response.dump()); // Write JSON to response body
                res.set_header("Content-Type", "application/json"); // Set content type
                res.end();
                return;
            }
        catch (const std::exception& e)
        {
            CROW_LOG_CRITICAL << "Unhandled exception in /login route: " << e.what();
            res.code = crow::status::INTERNAL_SERVER_ERROR;
            crow::json::wvalue error_json;
            error_json["message"] = "An unexpected error occurred";
            res.write(error_json.dump());
            res.set_header("Content-Type", "application/json");
            res.end();
            return;
        }
    });

    CROW_ROUTE(app, "/logout")
            .methods("POST"_method)
    ([&app](const crow::request& req, crow::response &res)
    {
        auto& cookie_ctx = app.get_context<crow::CookieParser>(req);
        std::string sessionID = cookie_ctx.get_cookie("sessionID");
        if (!sessionID.empty())
        {
            AuthHandle::deleteSession(sessionID); //once again, it erases the cookie from our map
        }

        //clear the cookie in the browser by setting an expired cookie
        cookie_ctx.set_cookie("sessionID", "").path("/").max_age(0).httponly();

        res.code = crow::status::OK;
        crow::json::wvalue success_response;
        success_response["message"] = "Logged out successfully!"; // Consistent message
        res.write(success_response.dump());
        res.set_header("Content-Type", "application/json");
        res.end();
        return;
    });

    CROW_ROUTE(app, "/me")
            .methods("GET"_method) //A GET method requests data.
    ([&app](const crow::request& req)
    {
        auto& cookie_ctx = app.get_context<crow::CookieParser>(req);
        std::string sessionID = cookie_ctx.get_cookie("sessionID");
        if (sessionID.empty())
        {
            return crow::response(crow::status::UNAUTHORIZED, "Not logged in");
        }

        std::optional<int> userID = AuthHandle::loadSession(sessionID);
        if (!userID.has_value())
        {
            //we create an expired cookie
            cookie_ctx.set_cookie("sessionID", "").path("/").max_age(0).httponly();
            return crow::response(crow::status::UNAUTHORIZED, "Session expired or invalid. Log in again.");
        }

        std::optional<User> user = database::getUserID(userID.value());
        if (!user.has_value())
        {
            //if this session points to a nonexistant user, we delete the session
            AuthHandle::deleteSession(sessionID);
            cookie_ctx.set_cookie("sessionID", "").path("/").max_age(0).httponly();
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, "User not found for session.");
        }

        crow::json::wvalue userJson;
        userJson["id"] = user->id;
        userJson["username"] = user->username;
        return crow::response(crow::status::OK, userJson);
    });
}