cmake_minimum_required(VERSION 3.16)
project(ProyectoParalelo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(OpenMP REQUIRED)

include_directories(include/fast-cpp-csv-parser)

add_executable(proyecto 
    src/main.cpp 
    src/sftp_client.cpp 
    src/api_client.cpp
)
target_link_libraries(proyecto PRIVATE CURL::libcurl OpenMP::OpenMP_CXX)