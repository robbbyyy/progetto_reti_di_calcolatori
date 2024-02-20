cmake_minimum_required(VERSION 3.27)
project(progetto_reti C)

set(CMAKE_C_STANDARD 11)

add_executable(progetto_reti segreteria.c
        server.c
        studente.c
)

find_package(MySQL REQUIRED)
include_directories(${MYSQL_INCLUDE_DIR})

target_link_libraries(NOME_TUO_TARGET ${MYSQL_LIBRARY})

