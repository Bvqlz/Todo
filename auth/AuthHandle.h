#pragma once
#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include "crow.h"


namespace AuthHandle
{
    //extern extends the visibility of variables and functions across multiple files
    extern std::unordered_map<std::string, int> sessions;
    extern std::mutex sessions_mutex;

    std::string genSessionID();
    void storeSession(const std::string& sessionID, const int userID);
    //this loads obtains the user ID using the sessionID
    std::optional<int> loadSession(const std::string& sessionID);
    void deleteSession(const std::string& sessionID);
    //this function obtains the session ID from crow request
    //because in crow we are going to be adding headers


}