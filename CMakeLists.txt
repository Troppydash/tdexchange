cmake_minimum_required(VERSION 3.5)
project(tdexchange)

# need cpp 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
# so that we use a statically linked libstdc++
set(CMAKE_CXX_FLAGS "-static-libstdc++")

# link header only libraries
# https://github.com/jcelerier/websocketpp, branch develop
include_directories("~/tools/websocketpp")
# https://github.com/nlohmann/json.git
include_directories("~/tools/nlohmann_json/include")
# https://github.com/bshoshany/thread-pool
include_directories("~/tools/thread-pool/include")
# https://github.com/yhirose/cpp-httplib
include_directories("~/tools/cpp-httplib")

# link boost
# sudo apt install libboost-all-dev
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system) 
include_directories(${Boost_INCLUDE_DIRS})

# https://github.com/fmtlib/fmt
# https://fmt.dev/latest/usage.html, build and install
find_package(fmt REQUIRED)
include_directories("~/tools/fmt/include")  # we shouldn't need this?

# copy the public folder
file(COPY ${PROJECT_SOURCE_DIR}/tdexchange/public DESTINATION ${PROJECT_SOURCE_DIR}/build)

# add the *.cpp files
file(GLOB CPP_SOURCES "tdexchange/*.cpp")
add_executable(${PROJECT_NAME} ${CPP_SOURCES})

# add header files
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${PROJECT_SOURCE_DIR}/tdexchange
)
# link boost
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${Boost_LIBRARIES}
)
# link fmt
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
