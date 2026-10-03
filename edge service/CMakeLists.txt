cmake_minimum_required(VERSION 3.10)

project(ai_infra)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(Threads REQUIRED)
link_directories(
	/usr/local/cuda-10.2/targets/aarch64-linux/lib)
add_executable(server 
	src/main.cpp
	src/Server.cpp
	src/TaskQueue.cpp
	src/ThreadPool.cpp
	src/BatchScheduler.cpp
	src/BatchQueue.cpp
	src/InferenceEngine.cpp
	)
add_executable(
	client
src/client.cpp)

target_include_directories(server PRIVATE
	${PROJECT_SOURCE_DIR}/include
	/usr/include/aarch64-linux-gnu
	/usr/local/cuda-10.2/targets/aarch64-linux/include
	)
target_link_libraries(server PRIVATE 
	Threads::Threads
	nvinfer
	nvonnxparser
	cudart)

