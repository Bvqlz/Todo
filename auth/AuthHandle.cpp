#include "AuthHandle.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace AuthHandle
{
    //static members?
    std::unordered_map<std::string, int> sessions;
    std::mutex sessions_mutex;

    std::string genSessionID()
    {
        //see if you can find a better library to create uuid's
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        std::stringstream ss;
        //limit is 16 bytes for 128 bit session id.
        for (int i = 0; i < 16; ++i)
        {
            //this line right here creates the 128 session id through iteration
            //hex makes the output format into hexadecimal
            //setw adds padding and setfill adds a char 0 inbetween the gaps actively changing the id.
            ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
        }
        return ss.str(); //assuming that this returns the id
    }

    void storeSession(const std::string& sessionID, const int userID)
    {
        //mutex creates a lock for a thread so that this happens only once at a time.
        //prevents multiple threads trying to access this at the same time.
        //this line in particular calls the session_mutex and checks if it is locked. If not it locks it until done
        //if it is locked when called this thread will be paused until this is unlocked
        std::lock_guard<std::mutex> lock(sessions_mutex);
        sessions[sessionID] = userID; //our sessionID becomes our key and our userID is the associated value
        CROW_LOG_INFO << "Session " << sessionID << " stored for user: " << userID;
    }

    std::optional<int> loadSession(const std::string& sessionID) //apperantly this gets us the user id?
    {
        //lock the thread again
        //remember that lock guard does the locking and unlocking for us
        std::lock_guard<std::mutex> lock(sessions_mutex);
        //create an iterator to find the sessionID that we were given
        auto it = sessions.find(sessionID);
        if (it != sessions.end()) // as long as the iterator does not reach the end of the map
        {
            return it->second; //we return the userID associated with the sessionID
        }

        return std::nullopt; // if we did not find it, we return an optional null.
    }

    void deleteSession(const std::string& sessionID)
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        //this erase function returns the number of instances that were deleted
        //if none were found, we return a zero
        if (sessions.erase(sessionID) > 0)
        {
            CROW_LOG_INFO << "Session " << sessionID << " deleted";
        }
        else
        {
            CROW_LOG_INFO << "Session " << sessionID << " not found";
        }
    }
}