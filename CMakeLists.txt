cmake_minimum_required(VERSION 3.5)

project(wii)

set(CMAKE_BUILD_TYPE DEBUG)

find_package(OpenSSL REQUIRED)

set(SOURCES src/main.cpp
			src/memory/bus.cpp
			src/memory/nand.cpp
			src/cpu/starlet.cpp
			src/crypto/aes.cpp
			src/crypto/sha1.cpp)

add_executable(wii ${SOURCES})
target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)

target_include_directories(wii PRIVATE ${CMAKE_SOURCE_DIR})

set_property(TARGET wii PROPERTY CXX_STANDARD 23)