# CommunitySearch 快速上手

本文面向第一次接触本项目的同学，帮助你快速完成三件事：
- 理解项目在做什么
- 成功编译
- 跑通常用实验

---

## 1. 项目是什么

`CommunitySearch` 是一个图查询/社区搜索实验项目。  
核心包含：
- 基础图结构与查询算法（如 `globalsearch`）
- 基于索引的查询（`TreeIndex`、`SharingIndex`）
- 单查询与多查询实验入口（通过命令行参数切换）

常看目录：
- `src/graph/`：核心算法实现（如 `SharingIndex.cpp`、`TreeIndex.cpp`）
- `src/include/`：头文件接口
- `src/experiments/`：实验调度与实验实现
- `src/experiments/docs/EXPERIMENTS.md`：实验名、参数与输出说明
- `data/raw/`：输入图数据
- `data/output/`：实验输出 CSV

---

## 2. 环境要求

- C++ 编译器（Windows 下常用 MSVC）
- CMake（建议 3.16+）
- 可用命令行（PowerShell / CMD / Terminal）

---

## 3. 编译（Windows）

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build --config Debug
```

可执行文件通常在：
- `build/Debug/csp.exe`

如果你想编译 Release：

```bash
cmake --build build --config Release
```

---

## 4. 运行方式

通用格式（见 `main.cpp`）：

```bash
csp.exe <datasetPath> <experimentName> <randomQueryCount> <randomSeed> <queryNodeCount> [similarityThreshold] [useLegacyQueryBuilder] [shift_1] [shift_2] [begin_pick_count]
```

最常用示例（在项目根目录执行）：

```bash
./build/Debug/csp.exe
./build/Debug/csp.exe data/raw/email-Eu-core.txt global 5 123
./build/Debug/csp.exe data/raw/email-Eu-core.txt retrieval 5 123
./build/Debug/csp.exe data/raw/email-Eu-core.txt greedy 5 123
./build/Debug/csp.exe data/raw/email-Eu-core.txt single_compare 5 123
./build/Debug/csp.exe data/raw/email-Eu-core.txt batchsearch 20 123 6
./build/Debug/csp.exe data/raw/email-Eu-core.txt batchsearch_rough 20 123 6
./build/Debug/csp.exe data/raw/email-Eu-core.txt multi_query_compare 20 123 6
./build/Debug/csp.exe data/raw/email-Eu-core.txt multi_query_min_compare 20 123 6 0.15
```

说明：
- 当前可用的最小化对比实验是 `multi_query_min_compare`
- `batchmin_compare` 已从分发入口移除（会回退为未知实验）

---

## 5. 输出文件在哪里

实验输出默认在：
- `data/output/single_query/`
- `data/output/multi_query/`

例如：
- `data/output/single_query/global_result.csv`
- `data/output/multi_query/multi_query_compare_result.csv`
- `data/output/multi_query/multi_query_min_compare_result.csv`

---

## 6. 常见问题

- **运行 `git log` 后回不到命令行**  
  按 `q` 退出分页器。

- **编译报 `LNK1168`（无法写入 csp.exe）**  
  关闭正在运行的 `csp.exe` 或终端占用，再重新编译。

- **实验名不生效，提示 Unknown experiment**  
  先查看 `src/experiments/docs/EXPERIMENTS.md` 的实验清单，并确认拼写。

---

## 7. 建议阅读顺序

1. `src/experiments/docs/EXPERIMENTS.md`（先会跑）
2. `src/experiments/core/ExperimentRunner.cpp`（看调度）
3. `src/graph/TreeIndex.cpp` 与 `src/graph/SharingIndex.cpp`（看核心算法）

如果你只想先验证环境，建议先跑：

```bash
./build/Debug/csp.exe data/raw/email-Eu-core.txt global 5 123
```

