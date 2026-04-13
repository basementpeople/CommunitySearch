#!/bin/bash

# 编译各个源文件为目标文件
g++ -std=c++17 -O3 -march=native -finline-functions -funroll-loops -c Graph.cpp
g++ -std=c++17 -O3 -march=native -finline-functions -funroll-loops -c TreeIndex.cpp
g++ -std=c++17 -O3 -march=native -finline-functions -funroll-loops -c SharingIndex.cpp
g++ -std=c++17 -O3 -march=native -finline-functions -funroll-loops -c VertexScore.cpp

# 链接阶段：所有参数必须跟在同一行的 g++ 命令中
g++ -std=c++17 -O3 -march=native -finline-functions -funroll-loops -flto \
main.cpp Graph.o TreeIndex.o SharingIndex.o VertexScore.o \
-o a.out

echo "done"

start=$(date +%s)  # 获取开始时间
./a.out  # 执行你的命令
end=$(date +%s)    # 获取结束时间
echo "Execution time: $((end - start)) seconds"