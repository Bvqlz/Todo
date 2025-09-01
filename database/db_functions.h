#pragma once
#include <pqxx/pqxx>
#include "crow.h"
#include "task.hpp"
#include "user.h"

struct Task
{
    int id;
    std::string description;
    std::string Tstatus;
};


namespace database
{
    std::string getConnection();
    void ensure_db();
    std::vector<Task> getTasks(std::optional<int> userID = std::nullopt);
    std::optional<Task> getTask(int tID, std::optional<int> userID = std::nullopt);
    int createTask(const std::string& description, const std::string& Tstatus, int userID);
    bool updateTask(int tID, const std::optional<std::string> &description, std::optional<status> Estatus, int userID);
    bool deleteTask(int tID, int userID);


    std::optional<int> createUser(const std::string& username, const std::string& password_hash);
    std::optional<User> getUsername(const std::string& username);
    std::optional<User> getUserID(int userID);
}
