# Experiments Framework Guide

本文档说明 `src/experiments` 目录下实验框架的组织方式、各实验作用，以及统一运行方法。

## 1) 实验框架总览

- **建议按用途看文件（减少“文件太多”的感知）**
  - 核心入口：`src/experiments/core/ExperimentRunner.*`
  - 单查询链路：`src/experiments/core/SingleQueryExperiment.*`
  - 多查询检索/对比：`src/experiments/multi_query/BatchSearchExperiment.*`、`src/experiments/multi_query/MultiQueryCompareExperiment.*`、`src/experiments/multi_query/BatchMinCompareExperiment.*`
  - 历史实验（仅回溯/兼容）：`src/experiments/legacy/BatchExperiment.*`、`src/experiments/legacy/CompareExperiment.*`

- **统一入口**
  - `src/main.cpp`
  - 负责解析命令行参数，构造 `Graph`，调用 `experiments::RunExperiment(...)`。
- **实验调度器**
  - `src/experiments/core/ExperimentRunner.h`
  - `src/experiments/core/ExperimentRunner.cpp`
  - 负责：
    - 根据 `experimentName` 分发到对应实验函数
    - 为实验选择默认输出文件（`data\output\*.csv`）
    - 处理随机查询参数（数量、种子等）
- **具体实验模块**
  - `src/experiments/core/SingleQueryExperiment.*`：单查询算法实验
  - `src/experiments/multi_query/BatchSearchExperiment.*`：多查询 batchsearch 系列实验
  - `src/experiments/multi_query/MultiQueryCompareExperiment.*`：多查询四方法对比实验
  - `src/experiments/legacy/BatchExperiment.*`、`src/experiments/legacy/CompareExperiment.*`：历史/组合实验

## 2) 命令行参数约定

当前主程序参数顺序（见 `main.cpp`）：

1. `datasetPath`：数据集路径（可省略，默认 `data\raw\email-Eu-core.txt`）
2. `experimentName`：实验名
3. `randomQueryCount`：随机查询数量（对部分实验表示“查询组数”）
4. `randomSeed`：随机种子（`0` 表示随机设备生成）
5. `queryNodeCount`：每个查询包含的点数（主要用于 batch/multi-query 实验）
6. `similarityThreshold`：可选，相似度阈值（用于 `multi_query_min_compare`；不传则自动估计）
7. `useLegacyQueryBuilder`：可选，是否启用 legacy 风格查询构造（`0/1`，默认 `0`）
8. `shift_1`：可选，legacy 模式下 core 下界扩展参数（默认 `0`）
9. `shift_2`：可选，legacy 模式下 core 上界扩展参数（默认 `0`）
10. `begin_pick_count`：可选，legacy 模式下每组初始选点数（默认 `3`）

示例：

```bash
csp.exe data/raw/email-Eu-core.txt "batchsearch" 20 42 6
```

含义：跑 `batchsearch`，生成 20 组查询，每组 6 个点，种子 42。

## 3) 实验清单与作用

以下实验名均由 `ExperimentRunner.cpp` 分发：

- `global`
  - 单查询 `globalsearch`。
  - 输出：`data\output\single_query\global_result.csv`

- `retrieval`
  - 单查询 `retrievalShellStruct`。
  - 输出：`data\output\single_query\retrieval_result.csv`

- `k-global`
  - 单查询 `kGlobalsearch(query, kCount)`。
  - `kCount` 从命令行第 5 个参数 `queryNodeCount` 读取；若不传则默认使用查询点个数。
  - 输出：`data\output\single_query\k_global_result.csv`

- `greedy`
  - 单查询 `greedyConnection`（通常依赖 retrieval 结果）。
  - 输出：`data\output\single_query\greedy_result.csv`

- `single_compare`
  - 同一查询集下比较 `global / retrieval / greedy`。
  - 输出：
    - 汇总：`data\output\single_query\single_compare_result.csv`
    - 明细：`*_global.csv`、`*_retrieval.csv`、`*_greedy.csv`

- `batchsearch`
  - 多查询下运行 `SharingIndex::batchsearch`。
  - 输出：`data\output\multi_query\batchsearch_result.csv`

- `batchsearch_rough`
  - 多查询下运行 `SharingIndex::batchsearchRough`。
  - 输出：`data\output\multi_query\batchsearch_rough_result.csv`

- `multi_query_compare`
  - 多查询下比较四种方法：
    - `batchsearch`
    - `batchsearch_rough`
    - 循环 `globalsearch`
    - 循环 `retrievalShellStruct`
  - 输出：
    - 汇总：`data\output\multi_query\multi_query_compare_result.csv`
    - 方法明细：
      - `..._batchsearch_detail.csv`
      - `..._batchsearch_rough_detail.csv`
      - `..._globalsearch_loop_detail.csv`
      - `..._retrieval_loop_detail.csv`

- `multi_query_min_compare`
  - 多查询下比较三种 minCSP 方法：
    - `batchMinsearchPrecise`
    - `batchMinsearchFast`
    - 循环 `greedyConnection`
  - 输出：
    - 汇总：`data\output\multi_query\multi_query_min_compare_result.csv`
    - 方法明细：
      - `..._precise_detail.csv`
      - `..._fast_detail.csv`
      - `..._greedy_loop_detail.csv`

- `final_csp_three_compare`
  - 最终展示用三方法对比（统一查询组）：
    - `retrieval_loop`
    - `batchsearch`
    - `batchsearch_rough`
  - 输出（独立目录）：
    - 汇总：`data\output\final\final_csp_three_compare_result.csv`
    - 方法明细：
      - `..._retrieval_loop_detail.csv`
      - `..._batchsearch_detail.csv`
      - `..._batchsearch_rough_detail.csv`

- `final_csp_four_compare`
  - 最终展示用四方法对比（统一查询组）：
    - `global_loop`
    - `retrieval_loop`
    - `batchsearch`
    - `batchsearch_rough`
  - 输出（独立目录）：
    - 汇总：`data\output\final\final_csp_four_compare_result.csv`
    - 方法明细：
      - `..._global_loop_detail.csv`
      - `..._retrieval_loop_detail.csv`
      - `..._batchsearch_detail.csv`
      - `..._batchsearch_rough_detail.csv`

- `final_min_csp_simi_sweep`（min-CSP final 实验 1）
  - 固定一批查询组，将聚类相似度阈值 `simi` 从 **0% 扫到 90%**（步长 **10%**，即 `simi = 0.0,0.1,...,0.9`），对比三种方法：
    - `batchMinsearchPrecise`
    - `batchMinsearchFast`
    - 循环 `greedyConnection`（与 `multi_query_min_compare` 相同实现；**不依赖** `simi`，各行中 greedy 指标相同，仅便于与 Precise/Fast 同表对比）
  - 查询构造与 `multi_query_min_compare` 一致：第 7～10 个参数可指定 legacy 与 `shift_1` / `shift_2` / `begin_pick_count`。
  - 第 6 个参数 `similarityThreshold` 在扫频实验中**不使用**（可填任意占位值，例如 `0`）。
  - 输出：
    - 汇总：`data\output\final\final_min_csp_simi_sweep_result.csv`（每行一个相似度档位，宽表列出三方法的耗时与并集/总和规模，**不含** `AvgResultNodeCount`）
    - 明细（与汇总同前缀）：
      - `..._precise_detail.csv`：Precise 在每个 `simi` 下逐查询的 `TimeSec`（该档整批耗时，各行相同）、`ResultSize`、`K`、`ComponentId`
      - `..._fast_detail.csv`：Fast，列同上
      - `..._greedy_loop_detail.csv`：Greedy 与 `simi` 无关，`SimilarityPct`/`Similarity` 为 `-`，`TimeSec` 为整批 greedy 总耗时

- `batchmin` / `h0` / `h` / `h1` / `h2` / `compare`
  - 历史 batch 与对比实验路径。
  - 输出文件在 `ExperimentRunner.cpp` 的 `defaultCsvPathForExperiment(...)` 中可查。

## 4) 常用运行方式

- **默认运行（单查询 global）**

```bash
csp.exe
```

- **单查询随机化（例如 retrieval，随机 5 点）**

```bash
csp.exe data/raw/email-Eu-core.txt "retrieval" 5 123
```

- **单查询 k-global（随机 5 点，kCount=80）**

```bash
csp.exe data/raw/email-Eu-core.txt "k-global" 5 123 80
```

- **多查询 batchsearch（20 组，每组 6 点）**

```bash
csp.exe data/raw/email-Eu-core.txt "batchsearch" 20 123 6
```

- **多查询 rough 版（同参数）**

```bash
csp.exe data/raw/email-Eu-core.txt "batchsearch_rough" 20 123 6
```

- **多查询四方法对比**

```bash
csp.exe data/raw/email-Eu-core.txt "multi_query_compare" 20 123 6
```

- **多查询最小化四方法对比**

```bash
csp.exe data/raw/email-Eu-core.txt "multi_query_min_compare" 20 123 6
```

- **多查询最小化（手动指定相似度阈值）**

```bash
csp.exe data/raw/email-Eu-core.txt "multi_query_min_compare" 20 123 6 0.15
```

- **多查询最小化（启用 legacy 查询构造）**

```bash
csp.exe data/raw/email-Eu-core.txt "multi_query_min_compare" 20 123 6 0.15 1 1 2 3
```

- **最终展示三方法对比**

```bash
csp.exe data/raw/email-Eu-core.txt "final_csp_three_compare" 20 123 6
```

- **最终展示四方法对比**

```bash
csp.exe data/raw/email-Eu-core.txt "final_csp_four_compare" 20 123 6
```

- **min-CSP final：相似度 0%～90% 扫频（三方法时间与结果规模）**

```bash
csp.exe data/raw/email-Eu-core.txt "final_min_csp_simi_sweep" 20 123 6 0
```

- **同上，启用 legacy 查询构造（参数与 multi_query_min_compare 对齐）**

```bash
csp.exe data/raw/email-Eu-core.txt "final_min_csp_simi_sweep" 20 123 6 0 1 1 2 3
```

## 5) 输出字段解释（多查询对比）

`batchsearch_result.csv` 与 `batchsearch_rough_result.csv` 的关键列：

- `TestNumber`：查询编号（从 `0` 开始）
- `QueryNodes`：该组查询节点（空格分隔）
- `size`：该查询结果的节点数
- `k`：该查询结果对应的最小度（由索引流程记录）

`multi_query_compare_result.csv` 的关键列：

- `TimeSec`：该方法处理整批查询的总时间（秒）
- `UnionNodeCount`：所有查询结果取并集后的节点数
- `TotalResultNodeCount`：所有查询结果大小直接求和（不去重）
- `AvgResultNodeCount`：平均每个查询结果大小（整数平均）
- `DetailCsv`：该方法对应明细文件路径

`multi_query_compare` 的方法明细文件（`*_detail.csv`）关键列：

- `Code`：查询编号（与输入查询组顺序一致）
- `QueryNodes`：该组查询节点（空格分隔）
- `ResultSize`：该方法下该查询结果的节点数
- `K`：该方法下该查询对应的最小度（若方法本身不直接返回，则按实现回填）

`multi_query_min_compare_result.csv` 的关键列：

- `TimeSec`：方法总耗时（秒）
- `StageClusterSec`：第一阶段聚类耗时（仅 Precise/Fast）
- `StageGreedyStepSec`：簇共享 `greedyStep` 耗时（仅 Precise/Fast）
- `StageSteinerSec`：Steiner 树阶段耗时（仅 Precise/Fast）
- `StageSecondClusteringSec`：二次聚类耗时（仅 Fast）
- `StageGreedySimplySec`：`greedyStep_simply` 阶段耗时（仅 Precise/Fast）
- `FirstClusterCount`：第一阶段聚类簇数（仅 Precise/Fast）
- `UnionNodeCount`：所有查询结果并集大小
- `TotalResultNodeCount`：所有查询结果大小求和
- `AvgResultNodeCount`：平均每查询结果大小
- `DetailCsv`：该方法对应明细文件路径

`final_min_csp_simi_sweep_result.csv` 的关键列：

- `SimilarityPct` / `Similarity`：相似度档位（百分比与 `simi` 小数，例如 `50` 与 `0.5`）
- `BatchminPrecise_*` / `BatchminFast_*` / `GreedyLoop_*`：各方法在该档位下的 `TimeSec`、`UnionNodeCount`、`TotalResultNodeCount`（**无**平均每查询列）
- 末行 `SUMMARY`：种子、查询组数、每组点数、扫频说明、查询构造方式，以及三个明细 CSV 的路径字段 `detail_precise` / `detail_fast` / `detail_greedy`

`*_precise_detail.csv` / `*_fast_detail.csv` / `*_greedy_loop_detail.csv` 关键列：

- `SimilarityPct` / `Similarity`：greedy 明细中为 `-`（与 `simi` 无关）
- `TimeSec`：该次运行中**整批**该方法的 wall 时间（秒）；明细中同一 `simi` 下各查询行重复同一 `TimeSec`
- `Code`、 `QueryNodes`、 `ResultSize`、 `K`、 `ComponentId`：与 `multi_query_min_compare` 明细含义一致

## 6) 开发建议

- 新增实验时优先走 `ExperimentRunner` 分发，保证命令行入口一致。
- 输出文件路径建议统一放在 `data\output\`，命名遵循 `<experiment>_result.csv`。
- 若实验复用随机查询，尽量复用同一批 query，以保证横向可比性。
