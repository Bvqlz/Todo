#include "crow_routes.h"
#include "readFile.h"
#include "db_functions.h"
#include "AuthHandle.h"



void taskRoutes(crow::App<crow::CookieParser>& app)
{

    CROW_ROUTE(app, "/")
     ([]()
    {
        std::string html = utilities::readFile("templates/page3.html");
         if (html.empty())
         {
            CROW_LOG_ERROR << "HTML file was not found.";
             return crow::response(crow::status::NOT_FOUND, "Page was not found");
         }

         return crow::response(crow::status::OK, "text/html", html);

    });

    //this is psudo middleware
    auto check_auth = [&](const crow::request& req) -> std::optional<int> // arrow pointing to optional indicates the return type
    {
        auto& cookie_ctx = app.get_context<crow::CookieParser>(req); // Get the cookie object
        std::string sessionID = cookie_ctx.get_cookie("sessionID"); // find the cookie value from the sessionID key.
        if (sessionID.empty())
        {
            return std::nullopt;
        }

        std::optional<int> userID = AuthHandle::loadSession(sessionID); // return the id associated with this sessionID
        return userID;
    };

    // Endpoint to list all tasks
    CROW_ROUTE(app, "/tasks")
    ([&](const crow::request& req)
    {
        std::optional<int> userID = check_auth(req);
        if (!userID.has_value())
        {
            return crow::response(crow::status::UNAUTHORIZED, "Authentication required.");
        }

        crow::json::wvalue response_json;
        crow::json::wvalue::list tasks_array;

        try
        {
            std::vector<Task> user_tasks = database::getTasks(userID.value());
            for (const auto &task: user_tasks)
            {
                crow::json::wvalue task_json;
                task_json["id"] = task.id;
                task_json["description"] = task.description;
                task_json["status"] = task.Tstatus;
                tasks_array.push_back(std::move(task_json)); // move the object direct? Believe this avoids having to copy
            }
        }
        catch (const std::exception &e)
        {
            CROW_LOG_ERROR << "Error listing tasks from DB in route handler: " << e.what();
            crow::json::wvalue error_json;
            error_json["error"] = "Database error retrieving tasks";
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, error_json);
        }

        response_json["tasks"] = std::move(tasks_array); // sets the all the elements in an array called tasks.
        return crow::response(crow::status::OK, response_json);
    });

    // Endpoint to retrieve a single task by ID
    CROW_ROUTE(app, "/tasks/<int>")
    ([&](const crow::request& req, int tID)
    {
        std::optional<int> userID = check_auth(req);
        if (!userID.has_value())
        {
            return crow::response(crow::status::UNAUTHORIZED, "Authentication required.");
        }

        try
        {
            std::optional<Task> task = database::getTask(tID, userID.value());

            if (task.has_value())
            {
                // we use arrow notation because our task is an optional data type.
                crow::json::wvalue task_json;
                task_json["id"] = task->id;
                task_json["description"] = task->description;
                task_json["status"] = task->Tstatus;
                return crow::response(crow::status::OK, task_json);
            }
        }
        catch (const std::exception &e)
        {
            CROW_LOG_ERROR << "Error listing task from DB: " << e.what();
            crow::json::wvalue error_json;
            error_json["error"] = "Database error retrieving task";
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, error_json);
        }

        crow::json::wvalue error_json;
        error_json["message"] = "Database error retrieving task";
        return crow::response(crow::status::NOT_FOUND, error_json);

    });

    // Endpoint to create a new task
    CROW_ROUTE(app, "/tasks")
        .methods("POST"_method) // sends data to table
    ([&](const crow::request& req)
    {

        std::optional<int> userID = check_auth(req);
        if (!userID.has_value())
        {
            return crow::response(crow::status::UNAUTHORIZED, "Authentication required.");
        }


        auto json_body = crow::json::load(req.body);
        if (!json_body)
        {
            crow::json::wvalue error_json;
            error_json["message"] = "Invalid json in request body";
            return crow::response(crow::status::BAD_REQUEST, error_json); // this a 400 code
        }

        std::string description = json_body["description"].s();
        std::string status = json_body["status"].s(); // default is set if left blank

        try
        {
            int new_id = database::createTask(description, status, userID.value());
            crow::json::wvalue ntask_json;
            ntask_json["id "] = new_id;
            ntask_json["description"] = description;
            ntask_json["status"] = status;
            return crow::response(crow::status::OK, ntask_json);
        }
        catch (const std::exception &e)
        {
            CROW_LOG_ERROR << "Error creating task:  " << e.what();
            crow::json::wvalue error_json;
            error_json["error"] = "Database error creating task";
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, error_json);
        }
    });

    // Endpoint to update an existing task by ID
    CROW_ROUTE(app, "/tasks/<int>")
        .methods("PUT"_method)
    ([&](const crow::request& req, int tID)
    {
        std::optional<int> userID = check_auth(req);
        if (!userID.has_value())
        {
            return crow::response(crow::status::UNAUTHORIZED, "Authentication required.");
        }

        auto json_body = crow::json::load(req.body);
        if (!json_body)
        {
            crow::json::wvalue error_json;
            error_json["message"] = "Invalid json in request body";
            return crow::response(crow::status::BAD_REQUEST, error_json); // this a 400 code
        }

        std::optional<std::string> description;
            if (json_body.count("description") && json_body["description"].t() == crow::json::type::String)
            {
                description = json_body["description"].s();
            }

            std::optional<status> Estatus;
            if (json_body.count("status") && json_body["status"].t() == crow::json::type::String) {
                try
                {
                    std::string Jstatus = json_body["status"].s();
                    Estatus = toStatus(Jstatus);

                }
                catch (const std::runtime_error& e)
                {
                    crow::json::wvalue error_json;
                    error_json["message"] = e.what();
                    return crow::response(crow::status::BAD_REQUEST, error_json);
                }
            }

            if (!description && !Estatus) // Check if at least one field is provided for update
            {
                crow::json::wvalue error_json;
                error_json["message"] = "No valid fields to update were provided (expected 'description' or 'status' as string)";
                return crow::response(crow::status::BAD_REQUEST, error_json);
            }

        try
        {
            if (bool update = database::updateTask(tID , description, Estatus, userID.value()))
            {
                std::optional<Task> utask = database::getTask(tID, userID.value());
                crow::json::wvalue utask_json;
                utask_json["id"] = utask->id; // once the updateTask is called we are just getting our task, and displaying it back to the frontend
                utask_json["description"] = utask->description;
                utask_json["status"] = utask->Tstatus;
                return crow::response(crow::status::OK, utask_json);
            }
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Error updating task: " << e.what();
            crow::json::wvalue error_json;
            error_json["error"] = "Database error updating task";
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, error_json);
        }

        crow::json::wvalue error_json;
        error_json["message"] = "Task not found";
        return crow::response(crow::status::NOT_FOUND, error_json);
    });

    // Endpoint to delete a task by ID
    CROW_ROUTE(app, "/tasks/<int>")
        .methods("DELETE"_method)
    ([&](const crow::request& req, int task_id)
    {
        std::optional<int> userID = check_auth(req);
        if (!userID.has_value())
        {
            return crow::response(crow::status::UNAUTHORIZED, "Authentication required.");
        }

        try
        {
            bool deleted = database::deleteTask(task_id, userID.value());
            if (deleted)
            {
                return crow::response(crow::status::NO_CONTENT); // indicates successful deletion
            }
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Error deleting task: " << e.what();
            crow::json::wvalue error_json;
            error_json["error"] = "Database error deleting task";
            return crow::response(crow::status::INTERNAL_SERVER_ERROR, error_json);
        }

        crow::json::wvalue error_json;
        error_json["message"] = "Task not found";
        return crow::response(crow::status::NOT_FOUND, error_json);
    });

}
