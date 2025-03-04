cmake_minimum_required(VERSION 3.14)

project(l1test)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
FetchContent_MakeAvailable(googletest)

add_executable(${PROJECT_NAME}
        OpenDbTest.cpp
        OpenDbWithCorruptJournalFileTest.cpp
        OpenDbWithEmptyJournalFileTest.cpp
        OpenDbWithEmptyJournalFolderTest.cpp
        OpenExistingDbTest.cpp
        OpenDbWithBackupTest.cpp
        DbInRuntimeTest.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        gmock_main
)

find_package(PkgConfig REQUIRED)
pkg_search_module(SQLITE REQUIRED sqlite3)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SQLITE_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
