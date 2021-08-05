# Mini-HTPPServer

## Introduction
It is a HTTP project in C++ done as a mini-project for the **DytechLab**.

## Instructions
1. Set a limit on open file handles:
```
ulimit -n 32768
```

2. Go to the directory where server file is located and run following:
```
./server [host] [port]
```

3. Open browser and go to: 
```
http://[host]:[port]
```

If you want to recompile the source code, you can use following command:
```
g++ -o main main.cpp -std=c++2a -pthread
```

## Futures
1. Supports 2 simple web pages
2. Supports `GET` and `POST` static requests

## Benchmark
Benchmark is set based on performing $100000$ requests on $10000$ **concurrent connections**.

Benchmark tool used is `Apache Benchmark`. Follow the [link](https://www.datadoghq.com/blog/apachebench/) to install for the linux ubuntu.

**NOTE**: This benchmark is based on testing on `localhost`.

Software that is used for testing:
![](https://i.imgur.com/iPaDxPV.png)


Latest Updates:
05. 08. 2021
1. Multithread option:

![](https://i.imgur.com/HU7Qcgb.png)

2: Single Thread Option:

![](https://i.imgur.com/djNNIaZ.png)
