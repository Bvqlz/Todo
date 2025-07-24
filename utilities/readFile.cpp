#include "readFile.h"

namespace utilities
{
    std::string readFile(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (file) {
            std::stringstream ss;
            ss << file.rdbuf();
            CROW_LOG_INFO << "Successfully read file: " << filepath;
            return ss.str();
        }
        CROW_LOG_ERROR << "Failed to read file: " << filepath << ". File might not exist or path is incorrect.";
        return ""; // Return empty string if file not found
    }
}