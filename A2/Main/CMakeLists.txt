cmake_minimum_required(VERSION 3.10)
project(A2)


set(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(BufferMgr/headers)
INCLUDE_DIRECTORIES(Catalog/headers)
INCLUDE_DIRECTORIES(DatabaseTable/headers)
INCLUDE_DIRECTORIES(Record/headers)

add_subdirectory(BufferMgr)
add_subdirectory(Catalog)
add_subdirectory(DatabaseTable)
add_subdirectory(Record)