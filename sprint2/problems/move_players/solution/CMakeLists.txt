cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)
add_definitions(-DBOOST_BEAST_USE_STD_STRING_VIEW)
# find_package(Boost 1.78.0 REQUIRED)
# if(Boost_FOUND)
  # include_directories(${Boost_INCLUDE_DIRS})
# endif()

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler/request_handler.cpp
	src/request_handler/request_handler.h
	src/request_handler/api.cpp
	src/request_handler/api.h
	src/request_handler/file.cpp
	src/request_handler/file.h
	src/request_handler/base.cpp
	src/request_handler/base.h
	src/log.cpp
	src/log.h
	src/app.cpp
	src/app.h
	src/dog.cpp
	src/dog.h
)
#target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost) 
