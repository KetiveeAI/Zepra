cmake_minimum_required(VERSION 3.10)
project(SearchCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(search_crawler
    src/main.cpp
    src/html_parser.cpp
    src/web_request.cpp
)

target_link_libraries(search_crawler
    ${CURL_LIBRARIES}
    Boost::system
    Boost::filesystem
)

install(TARGETS search_crawler DESTINATION bin)