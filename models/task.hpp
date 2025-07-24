#pragma once
#include <string>

enum class status
{
    Todo,
    InProgress,
    Completed
};

std::string toString(status eStat); //a enumerated stat

status toStatus(const std::string &sStat); //a string stat