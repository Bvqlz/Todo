#include "task.hpp"
#include <stdexcept>

std::string toString(status eStat) // enum status
{
    switch (eStat)
    {
        case status::Todo:
            return "todo";
        case status::InProgress:
            return "inprogress";
        case status::Completed:
            return "completed";
        default:
            return "unknown";
    }
}

status toStatus(const std::string &sStatus) // string status
{
    // if-else block for comparisons
    if (sStatus == "todo")
    {
        return status::Todo;
    }
    else if (sStatus == "inprogress")
    {
        return status::InProgress;
    }
    else if (sStatus == "completed")
    {
        return status::Completed;
    }
    else
    {
        throw std::runtime_error("Invalid status string: " + sStatus);
    }
}
