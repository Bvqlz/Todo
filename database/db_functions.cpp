#include "db_functions.h"
#include "task.hpp"


namespace database
{
    std::string getConnection()
    {
        //Probably a way better solution to having to do this.
        std::string name_ev = std::getenv("DBNAME");
        std::string user_ev = std::getenv("USER");
        std::string password_ev = std::getenv("PASSWORD");
        std::string host_ev = std::getenv("HOST");
        std::string port_ev = std::getenv("PORT");

        // password shouldn't be empty. But just in case
        if (password_ev.empty())
        {
            CROW_LOG_ERROR << "DB password env variable was not set. Can't connect to db.";
            return "";
        }

        std::string connectString = "dbname=" + name_ev +
                                    " user=" + user_ev +
                                    " password=" + password_ev +
                                    " host=" + host_ev +
                                    " port=" + port_ev;

        CROW_LOG_INFO << "Using connectString: " << connectString.substr(0, connectString.find("password=")) + "password=*****";
        return connectString;
    }

    void ensure_db()
    {
        try
        {
            pqxx::connection C(getConnection());

            //pqxx::work allows us to do a db transaction
            pqxx::work W(C);

            W.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id SERIAL PRIMARY KEY,"
                    "username VARCHAR(50) UNIQUE NOT NULL,"
                    "password_hash VARCHAR(255) NOT NULL,"
                    "created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP"
                    ");");
            CROW_LOG_INFO << "Ensured 'users' table exists";

            //sql terminology
            // IF NOT EXISTS ensures it doesn't throw an error if the table already exists.
            // id SERIAL PRIMARY KEY: 'SERIAL' makes it auto-incrementing, 'PRIMARY KEY' ensures uniqueness.
            // description VARCHAR(255) NOT NULL: Text field for task description, cannot be empty.
            // completed BOOLEAN DEFAULT FALSE: Boolean field for task status, defaults to false.

            W.exec("DO $$ BEGIN "
                "IF NOT EXISTS (SELECT 1 FROM pg_attribute WHERE attrelid = 'tasks'::regclass AND attname = 'user_id') THEN "
                "ALTER TABLE tasks ADD COLUMN user_id INTEGER;"
                "ALTER TABLE tasks ADD CONSTRAINT fk_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE;"
                "END IF;"
                "END $$;");
            CROW_LOG_INFO << "Ensured 'tasks' table has 'user_id' column and foreign key constraint.";

            //sql terminology
            //SELECT 1 FROM pg_attribute: This queries the PostgreSQL system catalog (pg_attribute table),
            //which stores information about all columns in all tables.
            //the attrelid line filters results to only be the columns of the tasks table
            //we further filter to user_id . If this column does not exist we create it
            //by creating a forign key, fk_user that is user_id is referenced by our other table users
            //cascade will delete all the tasks associated with that user should that user be deleted

            W.exec("CREATE TABLE IF NOT EXISTS tasks ("
                "id SERIAL PRIMARY KEY,"
                "description VARCHAR(256) NOT NULL,"
                "status VARCHAR(15) NOT NULL DEFAULT 'todo'"
                ");");

            W.commit(); // this makes the effects of a transaction definite. Meaning that the changes have been made to the database
            CROW_LOG_INFO << "Database Schema was ensured...";
        }
        catch(const pqxx::sql_error& e)
        {
            CROW_LOG_ERROR << "Database error: " << e.what();
            CROW_LOG_ERROR << "Query was: " << e.query();
            CROW_LOG_ERROR << "SQL state: " << e.sqlstate();
            exit(1);
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Error in connecting to data base schema:  " << e.what();
            exit(1);
        }
    }

    std::vector<Task> getTasks(std::optional<int> userID)
    {
        std::vector<Task> tasks;

        try
        {
            // connection object called C
            pqxx::connection C(getConnection());
            pqxx::work W(C);
            std::string query = ("SELECT id, description, status FROM tasks"); // now a dynamic sql string due to user implementation

            //the reason for not creating placeholders here is because of the optional aspect where if i were to make a public api,
            //then a user id would not be required
            if (userID.has_value()) //again, checks if an optional data type has a value.
            {
                query += " WHERE user_id = " + W.quote(userID.value()); //Mismatching variables, bad practice.
            }
            query += " ORDER BY id ASC;"; // orders id's by ascending order

            for (const auto& row : W.exec(query))
            {
                //this is called uniform initialization. Cleaner than having create an explicit temp Task object
                tasks.push_back(Task
                    {
                        //.as<T>() functions converts json objects to their desired types
                        row["id"].as<int>(),
                        row["description"].as<std::string>(),
                        row["status"].as<std::string>()
                    });
            }
            W.commit();
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Error listing tasks: " << e.what();
        }

        return tasks;
    }

    std::optional<Task> getTask(int tID, std::optional<int> userID) // I believe this isn't quite useful anymore.
    {
        try
        {
            pqxx::connection C(getConnection());
            pqxx::work W(C);
            std::string query = "SELECT id, description, status FROM tasks WHERE id = " + W.quote(tID); //quote function is for safety against sql injections
            if (userID.has_value())
            {
                query += " AND user_id = " + W.quote(userID.value());
            }
            query += ";";

            pqxx::result R = W.exec(query);
            W.commit();

            if (!R.empty())
            {
                auto const& row = R[0];
                return Task
                {
                    row["id"].as<int>(),
                    row["description"].as<std::string>(),
                    row["status"].as<std::string>()
                };
            }
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Error listing task: " << e.what();
            throw; // what does this do?
        }
        return std::nullopt; //im assuming this is a null optional.
    }

    int createTask(const std::string& description, const std::string& Tstatus, int userID)
    {
        try
        {
            pqxx::connection C(getConnection());
            pqxx::work W(C);
            C.prepare("create_task", "INSERT INTO tasks (description, status, user_id) VALUES ($1, $2, $3) RETURNING id;"); // what does returning id mean here?
            pqxx::result R = W.exec(pqxx::prepped{"create_task"}, pqxx::params{description, Tstatus, userID}); // instead of setting the parameters individually we do it together
            W.commit(); // commit when we  change something ? Like writing. No commit needed when we read.

            if (!R.empty())
            {
                return R[0]["id"].as<int>();
            }

        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Could not create task: " << e.what();
            throw;
        }
        return -1; //we return a failure
    }

    bool updateTask(int tID, const std::optional<std::string> &description, std::optional<status> Estatus, int userID)
    {
        try
        {
            pqxx::connection C(getConnection());
            pqxx::work W(C);

            std::string query = "UPDATE tasks SET";
            bool first_field = true; //just used to determine which fields have changed.

            if (description)
            {
                query += " description = " + W.quote(*description);
                first_field = false;
            }
            if (Estatus)
            {
                if (!first_field) query += ", ";
                query += " status = " + W.quote(toString(Estatus.value()));
                first_field = false;
            }

            query += " WHERE id = " + W.quote(tID) + " AND user_id = " + W.quote(userID) + " RETURNING id;";
            pqxx::result R = W.exec(query);
            W.commit();
            return R.affected_rows() > 0;
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Could not update task: " << e.what();
        }
        // need a return false here. Logic must be sound so that this is not always triggered
        return false;
    }

    bool deleteTask(int tID, int userID)
    {
        try
        {
            pqxx::connection C(getConnection());
            pqxx::work W(C);
            C.prepare("delete_task", "DELETE FROM tasks WHERE id = $1 AND user_id = $2;");

            pqxx::result R = W.exec(pqxx::prepped{"delete_task"}, pqxx::params{tID, userID});
            W.commit();
            return R.affected_rows() > 0;
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Could not delete task: " << e.what();
        }
        //same idea applies here
        return false;
    }

    std::optional<int> createUser(const std::string& username, const std::string& password_hash)
    {
        try
        { pqxx::connection C(getConnection());
            pqxx::work W(C);

            C.prepare("create_user", "INSERT INTO users (username, password_hash) VALUES ($1, $2) RETURNING id;");
            pqxx::result R = W.exec(pqxx::prepped{"create_user"}, pqxx::params{username, password_hash});

            W.commit();
            CROW_LOG_INFO << "User '" << username << "' created successfully with ID: " << R[0]["id"].as<int>();


            return R[0]["id"].as<int>();
        }
        catch (const pqxx::unique_violation& e)
        {
            CROW_LOG_ERROR << "Could not create user: " << e.what();
            return std::nullopt;
        }
        return -1; // this user was not created
    }


    std::optional<User> getUserID(int userID)
    {
        try
        {
            pqxx::connection C(getConnection());
            pqxx::work W(C);
            C.prepare("get_userID", "SELECT id, username, password_hash FROM users WHERE id = $1;");
            pqxx::result R = W.exec(pqxx::prepped{"get_userID"}, pqxx::params{userID});
            W.commit();

            if (!R.empty())
            {
                const auto& row = R[0];
                User user;
                user.id = row["id"].as<int>();
                user.username = row["username"].as<std::string>();
                user.password_hash = row["password_hash"].as<std::string>();
                return user;
            }
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Could not get user: " << e.what();
        }
        return std::nullopt;
    }

    std::optional<User> getUsername(const std::string& username)
    {
        try
        {
            pqxx::connection C(getConnection());
            pqxx::work W(C);
            C.prepare("get_username", "SELECT id, username, password_hash FROM users WHERE username = $1;");
            pqxx::result R = W.exec(pqxx::prepped{"get_username"}, pqxx::params{username});
            W.commit();

            if (!R.empty())
            {
                const auto& row = R[0];
                User user;
                user.id = row["id"].as<int>();
                user.username = row["username"].as<std::string>();
                user.password_hash = row["password_hash"].as<std::string>();
                CROW_LOG_INFO << "User found in getUsername: " << user.username;
                return user;
            }
        }
        catch (const std::exception& e)
        {
            CROW_LOG_ERROR << "Could not get user: " << e.what();
        }
        return std::nullopt;
    }

    }








