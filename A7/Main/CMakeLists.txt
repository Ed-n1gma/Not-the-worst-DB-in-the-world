cmake_minimum_required(VERSION 3.10)
project(A6)


set(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(BufferMgr/headers)
INCLUDE_DIRECTORIES(Catalog/headers)
INCLUDE_DIRECTORIES(DatabaseTable/headers)
INCLUDE_DIRECTORIES(Record/headers)
INCLUDE_DIRECTORIES(RelOps/headers)
INCLUDE_DIRECTORIES(SQL/headers)
INCLUDE_DIRECTORIES(Execution/headers)

add_subdirectory(BufferMgr)
add_subdirectory(Catalog)
add_subdirectory(DatabaseTable)
add_subdirectory(Record)
add_subdirectory(RelOps)
add_subdirectory(SQL)
add_subdirectory(Execution)

# cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=True ..