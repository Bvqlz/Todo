cmake_minimum_required(VERSION 3.31)
project(Todo)

set(CMAKE_CXX_STANDARD 20)

add_executable(Todo
        main.cpp
        database/db_functions.cpp
        models/task.cpp 
        routes/crow_routes.cpp
        utilities/readFile.cpp
        auth/auth_routes.cpp
        auth/AuthHandle.cpp
)

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

target_link_libraries(Todo PRIVATE Crow::Crow asio::asio ws2_32 mswsock)
target_link_libraries(Todo PRIVATE libpqxx::pqxx)
target_link_libraries(Todo PRIVATE unofficial-sodium::sodium)

target_include_directories(Todo PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/database
        ${CMAKE_SOURCE_DIR}/models
        ${CMAKE_SOURCE_DIR}/routes
        ${CMAKE_SOURCE_DIR}/utilities
        ${CMAKE_SOURCE_DIR}/auth
        ${CMAKE_SOURCE_DIR}/frontend
)
