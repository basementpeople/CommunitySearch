# CommunitySearch

CommunitySearch 是一个面向图查询与社区搜索实验的 C++ 项目。
项目包含单查询、多查询以及最终对比实验流程，并输出 CSV 结果文件。

## 项目亮点

- 基于 C++17 + CMake 的构建体系
- 单查询实验：`global`、`retrieval`、`greedy`、`single_compare`
- 多查询实验：`batchsearch`、`batchsearch_rough`、`multi_query_compare`、`multi_query_min_compare`
- 提供最终展示用实验套件，便于复现与横向对比
- 统一管理实验输出目录：`data/output/`

## 仓库结构

- `src/graph/`：图结构与核心算法实现
- `src/include/`：头文件与对外接口
- `src/experiments/`：实验调度器与各实验实现
- `data/raw/`：输入数据集
- `data/output/`：实验输出 CSV
- `scripts/`：辅助脚本

## 快速开始

在项目根目录编译：

```bash
cmake -S . -B build
cmake --build build --config Debug
```

运行一个简单实验：

```bash
./build/Debug/csp.exe data/raw/email-Eu-core.txt global 5 123
```

## 核心文档

- 快速上手：`QUICKSTART.md`
- 实验清单与参数说明：`src/experiments/docs/EXPERIMENTS.md`
- Git 分支与提交规范：`GIT_WORKFLOW.md`

## 常见输出位置

- 单查询输出：`data/output/single_query/`
- 多查询输出：`data/output/multi_query/`
- 最终对比输出：`data/output/final/`

## 说明

- 可执行文件名为 `csp`（Windows 下为 `csp.exe`）
- 进阶实验参数请以 `src/experiments/docs/EXPERIMENTS.md` 为准
