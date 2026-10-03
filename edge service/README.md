# Jetson Embedded Linux 下的 C++ GPU 推理服务

## 1. 项目简介

本项目基于 NVIDIA Jetson Nano，在 Embedded Linux 环境下使用 C++17 实现一个简单的 GPU 推理服务。

服务端通过 TCP 接收客户端请求，使用 epoll 实现非阻塞网络 IO，并通过任务队列、动态 Batch 调度和线程池将网络请求与 GPU 推理解耦。

推理部分使用 ONNX、TensorRT 和 CUDA，在 Jetson GPU 上完成模型推理。

本项目主要用于实践 Linux C++ 网络编程、并发处理、任务调度以及端侧 GPU 推理部署。

---

## 2. 系统架构

```text
Client
   |
   | TCP
   v
Server / epoll
   |
   v
TaskQueue
   |
   v
BatchScheduler
   |
   v
BatchQueue
   |
   v
ThreadPool
   |
   v
InferenceEngine
   |
   v
TensorRT
   |
   v
CUDA / Jetson GPU
   |
   v
Inference Result
```

---

## 3. 技术栈

* C++17
* Linux
* TCP/IP
* epoll
* 多线程
* Thread Pool
* Mutex / Condition Variable
* ONNX
* TensorRT 8.2
* CUDA 10.2
* NVIDIA Jetson Nano
* CMake

---

## 4. 核心模块

### 4.1 Server

负责 TCP 网络通信。

主要使用：

* socket
* bind
* listen
* accept
* epoll
* non-blocking IO

服务端为每个客户端维护独立的接收缓冲区，并通过换行符划分 TCP 字节流中的消息边界。

---

### 4.2 TaskQueue

负责保存网络层产生的任务。

网络线程负责：

```text
接收请求
    ↓
创建 Task
    ↓
TaskQueue
```

推理线程从 TaskQueue 获取任务，从而实现网络通信与推理处理的解耦。

---

### 4.3 BatchScheduler

负责将多个 Task 组织成 Batch。

当前配置：

```text
Batch Size = 4
Timeout = 10 ms
```

当收集到 Batch Size 个请求时立即形成 Batch。

如果在 Timeout 时间内没有收集到足够的请求，则使用当前已经收集到的请求形成 Batch。

例如：

```text
4 requests
    ↓
Batch size = 4
```

或者：

```text
3 requests
    ↓
等待 Timeout
    ↓
Batch size = 3
```

---

### 4.4 BatchQueue

负责保存已经形成的 Batch。

数据流：

```text
BatchScheduler
      ↓
BatchQueue
      ↓
Worker
```

通过 BatchQueue 将 Batch 生成和 Batch 执行两个阶段进行解耦。

---

### 4.5 ThreadPool

创建多个 Worker 线程并发处理 Batch。

当前测试使用：

```text
Worker Count = 3
```

每个 Worker 创建独立的 TensorRT `IExecutionContext`，并负责从 BatchQueue 获取 Batch 后执行推理。

---

### 4.6 InferenceEngine

负责 TensorRT 推理。

主要流程：

```text
ONNX Model
    ↓
TensorRT Network
    ↓
Optimization Profile
    ↓
TensorRT Engine
    ↓
Execution Context
    ↓
CUDA Memory
    ↓
GPU Inference
```

---

## 5. Dynamic Batch

模型输入采用动态 Batch：

```text
Input:
[batch, 1, 28, 28]

Output:
[batch, 10]
```

TensorRT Optimization Profile：

```text
MIN = 1
OPT = 4
MAX = 8
```

因此 TensorRT Engine 支持 Batch Size 1~8 的动态输入。

---

## 6. TensorRT 部署

项目使用 PyTorch 导出的 ONNX 模型：

```text
mnist_dynamic.onnx
```

ONNX 模型使用动态 Batch 维度：

```text
Input3:
[batch, 1, 28, 28]

Output:
[batch, 10]
```

Jetson Nano 上通过 TensorRT 构建 Engine，并为不同 Worker 创建独立的 Execution Context。

---

## 7. 测试结果

使用客户端发送 20 个请求进行测试。

配置：

```text
Batch Size = 4
Worker Count = 3
```

测试流程：

```text
20 requests
     ↓
5 batches
     ↓
4 requests / batch
     ↓
3 Workers
     ↓
TensorRT GPU inference
     ↓
20 results
```

最终客户端能够正常接收全部 20 个推理结果并退出。

---

## 8. 编译

进入项目目录：

```bash
cd ~/ai_infra
```

创建构建目录：

```bash
mkdir build
cd build
```

执行 CMake：

```bash
cmake ..
```

编译：

```bash
make -j$(nproc)
```

编译成功后生成：

```text
build/server
build/client
```

---

## 9. 运行

### 启动服务端

```bash
./build/server
```

默认监听：

```text
Port: 8080
```

### 启动客户端

在另一个终端运行：

```bash
./build/client
```

客户端发送请求并接收推理结果。

---

## 10. 项目目录

```text
ai_infra/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/
│   ├── Server.hpp
│   ├── Task.hpp
│   ├── TaskQueue.hpp
│   ├── Batch.hpp
│   ├── BatchQueue.hpp
│   ├── BatchScheduler.hpp
│   ├── ThreadPool.hpp
│   └── InferenceEngine.hpp
│
├── src/
│   ├── main.cpp
│   ├── Server.cpp
│   ├── TaskQueue.cpp
│   ├── BatchQueue.cpp
│   ├── BatchScheduler.cpp
│   ├── ThreadPool.cpp
│   ├── InferenceEngine.cpp
│   └── client.cpp
│
├── model/
│   └── mnist_dynamic.onnx
│
└── build/
```

---

## 11. 项目定位

本项目定位为一个运行在 Jetson Nano Embedded Linux 上的 C++ GPU 推理服务工程实践。

项目重点不是 AI 算法本身，而是将以下技术串联起来：

```text
Linux Network Programming
        ↓
Task Scheduling
        ↓
Multi-threading
        ↓
Dynamic Batching
        ↓
TensorRT
        ↓
CUDA
        ↓
Jetson GPU
```

通过该项目实践了从客户端请求进入，到 GPU 推理完成并返回结果的完整工程链路。

---

## 12. 项目局限

当前模型为轻量级测试模型，主要用于验证 TensorRT 动态 Batch 和 GPU 推理流程。

项目目前尚未针对生产环境进行深度性能优化，例如：

* GPU Buffer 复用
* CUDA Stream 异步流水线
* INT8 量化
* 多 GPU 调度
* 分布式推理
* 完善的监控系统
* 生产级服务治理

因此，本项目主要作为 AI 推理服务和 Embedded Linux GPU 部署的工程实践项目。

---

## 13. 开发环境

```text
Hardware:
NVIDIA Jetson Nano Developer Kit B01

OS:
Ubuntu 18.04.6

JetPack:
4.6.1

CUDA:
10.2

TensorRT:
8.2.1

Compiler:
GCC 7.5

CMake:
3.10
```

